#include "calculator_core.hpp"
#include "keymap.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void roundTrip(pmk::CalculatorCore original) {
    const auto ticks = original.tickCount();
    const auto bytes = original.saveState();
    check(ticks == original.tickCount(), "saving advanced the emulated machine");
    pmk::CalculatorCore restored;
    restored.loadState(bytes);
    check(restored.saveState() == bytes, "state did not round trip exactly");
    for (const auto count : {1U, 3U, 167U, 1008U, 5040U, 17083U}) {
        original.advanceTicks(count);
        restored.advanceTicks(count);
        check(original.saveState() == restored.saveState(), "restored continuation diverged");
    }
    original.releaseKey(); restored.releaseKey();
    original.advanceTicks(50000); restored.advanceTicks(50000);
    check(original.saveState() == restored.saveState(), "released continuation diverged");
}

void key(pmk::CalculatorCore& core, pmk::PhysicalKey key) {
    const auto contact = pmk::matrixKey(key);
    core.pressMatrixKey(contact.row, contact.column);
    core.advanceTicks(50000);
    core.releaseKey();
    core.advanceTicks(250000);
}

void updateChecksum(std::vector<std::uint8_t>& bytes) {
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t i = 20; i < bytes.size(); ++i) {
        crc ^= bytes[i];
        for (unsigned j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xedb88320U : 0);
    }
    crc ^= 0xffffffffU;
    for (unsigned i = 0; i < 4; ++i)
        bytes[16 + i] = static_cast<std::uint8_t>(crc >> (i * 8));
}

void rejected(const std::vector<std::uint8_t>& bad, pmk::CalculatorCore& target) {
    const auto before = target.saveState();
    bool failed = false;
    try { target.loadState(bad); }
    catch (const std::invalid_argument&) { failed = true; }
    check(failed, "invalid state was accepted");
    check(target.saveState() == before, "failed load changed the running machine");
}
}

int main(int argc, char** argv) {
    try {
        pmk::CalculatorCore core;
        const auto resetBytes = core.saveState();
        check(resetBytes[8] == 1 && resetBytes[9] == 0,
              "version is not encoded little-endian");
        roundTrip(core);
        for (const auto phase : {1U, 3U, 4U, 167U, 168U, 335U, 336U, 997U,
                                  1008U, 5039U, 5040U, 12345U}) {
            auto atPhase = core;
            atPhase.advanceTicks(phase);
            roundTrip(atPhase);
        }
        core.advanceTicks(250000);
        for (const auto mode : {pmk::AngleMode::Radians, pmk::AngleMode::Degrees,
                                pmk::AngleMode::Grads}) {
            core.setAngleMode(mode);
            key(core, pmk::PhysicalKey::Digit1);
            key(core, pmk::PhysicalKey::Enter);
            key(core, pmk::PhysicalKey::Digit2);
            core.pressMatrixKey(2, 0); // addition while still held
            core.advanceTicks(517);
            roundTrip(core);
            core.releaseKey();
            core.advanceTicks(500000);
        }
        pmk::CalculatorCore::Program program{};
        // A running loop must resume, not merely restore an idle display.
        program[0] = 0x01; program[1] = 0x51; program[2] = 0x00;
        core.importProgram(program);
        key(core, pmk::PhysicalKey::Return);
        core.pressMatrixKey(3, 0);
        core.advanceTicks(12345);
        roundTrip(core);
        core.releaseKey();
        core.advanceTicks(70000);
        roundTrip(core);

        auto bad = resetBytes;
        bad[0] ^= 1; rejected(bad, core);
        bad = resetBytes; bad[8] = 2; rejected(bad, core);
        bad = resetBytes; bad[10] = 1; rejected(bad, core);
        bad = resetBytes; bad.back() ^= 1; rejected(bad, core);
        bad = resetBytes; bad.push_back(0); rejected(bad, core);
        bad = resetBytes; bad[20] ^= 1; updateChecksum(bad); rejected(bad, core);
        bad = resetBytes; bad[28] = 2; updateChecksum(bad); rejected(bad, core);
        bad = resetBytes; bad[28 + 537] = 42;
        updateChecksum(bad); rejected(bad, core);
        for (std::size_t size = 0; size < resetBytes.size(); size += 17)
            rejected(std::vector<std::uint8_t>(resetBytes.begin(), resetBytes.begin() + size), core);
        rejected(std::vector<std::uint8_t>(pmk::CalculatorCore::MaximumStateBytes + 1), core);

        if (argc == 2) {
            const auto runningBytes = core.saveState();
            std::ofstream file(argv[1], std::ios::binary);
            file.write(reinterpret_cast<const char*>(runningBytes.data()),
                       static_cast<std::streamsize>(runningBytes.size()));
            check(bool(file), "could not write cross-compiler state fixture");
        }
        std::cout << "native state " << resetBytes.size() << " bytes; continuation and rejection passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
