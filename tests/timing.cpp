#include "calculator_core.hpp"
#include "keymap.hpp"
#include "realtime_clock.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>

namespace {
bool sameTube(const pmk::hardware::VfdSnapshot& a,
              const pmk::hardware::VfdSnapshot& b) {
    if (a.completedScans != b.completedScans)
        return false;
    for (unsigned i = 0; i < a.cells.size(); ++i)
        if (a.cells[i].code != b.cells[i].code ||
            a.cells[i].phosphor != b.cells[i].phosphor)
            return false;
    return true;
}

bool same(const pmk::CalculatorCore& a, const pmk::CalculatorCore& b) {
    if (a.tickCount() != b.tickCount() ||
        a.keyContactActive() != b.keyContactActive() ||
        a.displaySegments() != b.displaySegments() ||
        !sameTube(a.displaySnapshot(), b.displaySnapshot()))
        return false;
    for (unsigned i = 0; i < 4; ++i)
        if (!sameTube(a.stackDisplaySnapshots()[i], b.stackDisplaySnapshots()[i]))
            return false;
    auto aCopy = a;
    auto bCopy = b;
    return aCopy.exportProgram() == bCopy.exportProgram();
}

bool batching() {
    pmk::CalculatorCore singles, batches;
    std::uint32_t random = 61;
    const auto advance = [&](std::size_t ticks) {
        for (std::size_t i = 0; i < ticks; ++i)
            singles.advanceTicks(1);
        while (ticks != 0) {
            random = random * 1664525U + 1013904223U;
            const auto count = std::min(ticks, std::size_t(random % 4093 + 1));
            batches.advanceTicks(count);
            ticks -= count;
        }
        return same(singles, batches);
    };
    if (!advance(250000))
        return false;
    for (const auto mode : {pmk::AngleMode::Radians, pmk::AngleMode::Degrees,
                            pmk::AngleMode::Grads}) {
        singles.setAngleMode(mode);
        batches.setAngleMode(mode);
        for (const auto key : {pmk::PhysicalKey::Digit1, pmk::PhysicalKey::Enter,
                               pmk::PhysicalKey::Digit2, pmk::PhysicalKey::Add,
                               pmk::PhysicalKey::F, pmk::PhysicalKey::Digit4}) {
            const auto contact = pmk::matrixKey(key);
            singles.pressMatrixKey(contact.row, contact.column);
            batches.pressMatrixKey(contact.row, contact.column);
            if (!advance(8401))
                return false;
            singles.releaseKey();
            batches.releaseKey();
            if (!advance(60001))
                return false;
        }
    }
    singles.reset();
    batches.reset();
    return advance(10081);
}

bool clockCadence() {
    for (const auto interval : {1ULL, 1000ULL, 1'000'000ULL, 16'666'667ULL,
                                37'000'001ULL, 100'000'000ULL}) {
        pmk::RealtimeClock clock;
        // Tiny intervals need not iterate a full second to cover carry.
        const std::uint64_t duration = interval < 1000 ? 20000 : 1'000'000'000;
        std::uint64_t elapsed = 0, ticks = 0;
        while (elapsed < duration) {
            const auto delta = std::min<std::uint64_t>(interval, duration - elapsed);
            ticks += clock.consumeNanoseconds(delta);
            elapsed += delta;
        }
        if (ticks != duration * pmk::RealtimeClock::TicksPerSecond / 1'000'000'000)
            return false;
    }
    pmk::RealtimeClock clock;
    if (clock.consumeNanoseconds(UINT64_MAX) != 16800)
        return false;
    clock.consumeNanoseconds(1);
    clock.reset();
    return clock.consumeNanoseconds(5952) == 0 &&
           clock.consumeNanoseconds(1) == 1;
}
}

int main() {
    if (!clockCadence() || !batching()) {
        std::cerr << "clock or microtick batching changed calculator state\n";
        return EXIT_FAILURE;
    }
}
