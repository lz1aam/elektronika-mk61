#include "calculator_core.hpp"
#include "keymap.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void settledKey(pmk::CalculatorCore& core, pmk::PhysicalKey key) {
    const auto contact = pmk::matrixKey(key);
    core.pressMatrixKey(contact.row, contact.column);
    core.advanceTicks(50000);
    core.releaseKey();
    core.advanceTicks(500000);
}

pmk::CalculatorCore initialStack() {
    pmk::CalculatorCore core;
    core.advanceTicks(250000);
    for (unsigned digit : {1U, 2U, 3U, 4U, 5U, 6U, 7U, 9U})
        settledKey(core, static_cast<pmk::PhysicalKey>(10U + digit));
    settledKey(core, pmk::PhysicalKey::Enter);
    settledKey(core, pmk::PhysicalKey::Digit9);
    settledKey(core, pmk::PhysicalKey::Multiply);
    for (unsigned i = 0; i < 3; ++i)
        settledKey(core, pmk::PhysicalKey::Enter);
    check(core.displayText() == " 1.1111111 08", "invalid reproduction setup");
    return core;
}

void advanceTo(pmk::CalculatorCore& core, std::uint64_t tick) {
    check(tick >= core.tickCount(), "non-monotonic test contact");
    core.advanceTicks(static_cast<std::size_t>(tick - core.tickCount()));
}

void phaseTrace(const pmk::CalculatorCore& initial, unsigned phase) {
    auto actual = initial;
    auto reference = initial;
    actual.advanceTicks(phase);
    std::uint32_t random = 7;
    const auto contact = pmk::matrixKey(pmk::PhysicalKey::Multiply);
    const auto nextWord = [](std::uint64_t tick) { return (tick + 3U) & ~std::uint64_t{3}; };
    for (unsigned i = 0; i < 30; ++i) {
        // Captured reproducer: seed 7, contacts at arbitrary bit phases.
        random = random * 1664525U + 1013904223U;
        const unsigned held = 4000U + random % 12001U;
        random = random * 1664525U + 1013904223U;
        const unsigned released = 4000U + random % 12001U;
        const auto press = actual.tickCount();
        actual.pressMatrixKey(contact.row, contact.column);
        actual.advanceTicks(held);
        const auto release = actual.tickCount();
        actual.releaseKey();
        actual.advanceTicks(released);

        // A reference that changes physical contacts at the next word boundary
        // must have identical machine state after the contacts have settled.
        advanceTo(reference, nextWord(press));
        reference.pressMatrixKey(contact.row, contact.column);
        advanceTo(reference, nextWord(release));
        reference.releaseKey();
    }
    const auto finish = nextWord(actual.tickCount() + 500000U);
    advanceTo(actual, finish);
    advanceTo(reference, finish);
    check(actual.saveState() == reference.saveState(),
          "sub-word contact phase changed the emulated computation");
    // Manual section 10.2: repeated products exceed 9.9999999e99. Before the
    // fix, the phase-zero captured trace instead produced 3.8742065e-73.
    check(actual.displayText().find("EDD0D") != std::string::npos,
          "rapid multiply produced a false numeric result instead of overflow");
    check(!actual.keyContactActive(), "released contact remained held");
}

void wordLatch() {
    pmk::hardware::KeyboardMatrix keyboard;
    keyboard.sample(1, true, 0);
    keyboard.pressContact(2, 0);
    for (unsigned phase = 1; phase < 4; ++phase)
        check(!keyboard.sample(1, true, phase).k2, "input changed inside a word");
    check(keyboard.sample(1, true, 0).k2, "held contact was not sampled");
    keyboard.releaseContact();
    check(!keyboard.contactActive(), "physical release was delayed");
    for (unsigned phase = 1; phase < 4; ++phase)
        check(keyboard.sample(1, true, phase).k2, "sampled word was torn on release");
    check(!keyboard.sample(1, true, 0).k2, "release was queued beyond the next word");
    keyboard.pressContact(2, 0);
    keyboard.releaseContact();
    check(!keyboard.sample(1, true, 0).k2, "unscanned press was queued");
    keyboard.pressContact(2, 0);
    check(!keyboard.sample(1, false, 0).k2, "busy scan accepted a contact");
    keyboard.releaseContact();
    check(!keyboard.sample(1, true, 0).k2, "busy contact was replayed later");
    keyboard.sample(12, true, 0);
    keyboard.reset();
    check(!keyboard.sample(12, true, 1).k2, "reset retained sampled input");
}
} // namespace

int main() {
    try {
        wordLatch();
        const auto initial = initialStack();
        for (unsigned phase = 0; phase < 4; ++phase)
            phaseTrace(initial, phase);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
