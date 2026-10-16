#include "calculator_core.hpp"
#include "keymap.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string_view>

namespace {
constexpr std::size_t KeySettleTicks = 250000;
constexpr std::size_t ProgramRunTicks = 5000000;

void press(pmk::CalculatorCore& core, pmk::PhysicalKey key) {
    const auto matrix = pmk::matrixKey(key);
    core.pressMatrixKey(matrix.row, matrix.column);
    core.advanceTicks(50000);
    core.releaseKey();
    core.advanceTicks(KeySettleTicks * 2);
}

void enter(pmk::CalculatorCore& core,
           std::initializer_list<pmk::PhysicalKey> digits) {
    for (const auto digit : digits)
        press(core, digit);
}

void store(pmk::CalculatorCore& core,
           std::initializer_list<pmk::PhysicalKey> value,
           pmk::PhysicalKey registerKey) {
    enter(core, value);
    press(core, pmk::PhysicalKey::Store);
    press(core, registerKey);
}

void import(pmk::CalculatorCore& core,
            std::initializer_list<std::uint8_t> commands) {
    pmk::CalculatorCore::Program program{};
    // PMK text images start with the first command at index 0. During manual
    // entry the display advances immediately and therefore shows 01 after
    // that command has already been stored at address 00.
    std::copy(commands.begin(), commands.end(), program.begin());
    core.importProgram(program);
    core.advanceTicks(KeySettleTicks);
}

void run(pmk::CalculatorCore& core) {
    press(core, pmk::PhysicalKey::Return);
    press(core, pmk::PhysicalKey::RunStop);
    core.advanceTicks(ProgramRunTicks);
}

bool expectPrefix(const char* name, const pmk::CalculatorCore& core,
                  std::string_view expected) {
    const auto actual = core.displayText();
    std::cout << name << ": [" << actual << "]\n";
    if (actual.starts_with(expected))
        return true;
    std::cerr << name << ": expected display prefix [" << expected
              << "], got [" << actual << "]\n";
    return false;
}

bool circleArea() {
    // Elektronika MK-61 Calculator Operating Instructions, Appendix 3,
    // "Calculating the area of a circle", expected S(d=4) = 12.56637.
    pmk::CalculatorCore core;
    import(core, {0x62, 0x22, 0x0e, 0x20, 0x12, 0x04, 0x13, 0x50});
    store(core, {pmk::PhysicalKey::Digit4}, pmk::PhysicalKey::Digit2);
    run(core);
    return expectPrefix("circle area", core, " 12.56637");
}

bool indirectBranchesAndRegisterMutation() {
    // Operating Instructions, Appendix 3, "Using unconditional indirect
    // branching GTO (i)". Besides the result 18, this checks the original
    // pointer-register quirk: R3 decrements, R4 increments, and Ra is stable.
    pmk::CalculatorCore core;
    import(core, {0x04, 0x0e, 0x83, 0x21, 0x02, 0x10, 0x84,
                  0x10, 0x03, 0x12, 0x8a, 0x11, 0x50});
    store(core, {pmk::PhysicalKey::Digit5}, pmk::PhysicalKey::Digit3);
    store(core, {pmk::PhysicalKey::Digit7}, pmk::PhysicalKey::Digit4);
    store(core, {pmk::PhysicalKey::Digit1, pmk::PhysicalKey::Digit2},
          pmk::PhysicalKey::Decimal); // register a
    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Digit3);
    expectPrefix("setup R3", core, " 5.");
    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Digit4);
    expectPrefix("setup R4", core, " 7.");
    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Decimal);
    expectPrefix("setup Ra", core, " 12.");
    run(core);
    if (!expectPrefix("indirect GTO", core, " 18."))
        return false;

    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Digit3);
    const bool r3 = expectPrefix("indirect GTO R3", core, " 00000004.");
    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Digit4);
    const bool r4 = expectPrefix("indirect GTO R4", core, " 00000008.");
    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Decimal);
    const bool ra = expectPrefix("indirect GTO Ra", core, " 00000012.");
    return r3 && r4 && ra;
}

bool conditionalIndirectBranch() {
    // Operating Instructions, Appendix 3, "Using conditional indirect
    // branching x<0 (i)". For x=4 in R1 and DEG mode the documented result
    // is 2836.2007. The program's known result is produced in radians; the
    // English draft's prose saying DEG conflicts with its own program/result.
    pmk::CalculatorCore core;
    core.setAngleMode(pmk::AngleMode::Radians);
    import(core, {0x01, 0x09, 0x4b, 0x09, 0x61, 0x22, 0x12, 0x42,
                  0x61, 0x02, 0x12, 0x16, 0x62, 0x11, 0xcb, 0x61,
                  0x1e, 0x10, 0x50, 0x61, 0x1c, 0x10, 0x50});
    store(core, {pmk::PhysicalKey::Digit4}, pmk::PhysicalKey::Digit1);
    run(core);
    return expectPrefix("conditional indirect x<0", core, " 2836.2007");
}

bool dsnzLoop() {
    // Operating Instructions, Appendix 3, "Looping calculations using
    // commands such as DSNZ0 (F, L0)". This executes four iterations of
    // documented summation, using R0 as the loop counter and R5 as its
    // accumulator. The manual gives no golden numeric result for this case,
    // so the checks below verify termination and the documented registers.
    pmk::CalculatorCore core;
    core.setAngleMode(pmk::AngleMode::Degrees);
    import(core, {0x40, 0x60, 0x03, 0x13, 0x1e, 0x02, 0x12, 0x04,
                  0x10, 0x65, 0x10, 0x45, 0x5d, 0x02, 0x50});
    press(core, pmk::PhysicalKey::Clear);
    press(core, pmk::PhysicalKey::Store);
    press(core, pmk::PhysicalKey::Digit5);
    enter(core, {pmk::PhysicalKey::Digit4});
    run(core);
    const auto result = core.displayText();
    std::cout << "DSNZ0 loop: [" << result << "]\n";
    if (result.find_first_of("0123456789") == std::string::npos) {
        std::cerr << "DSNZ0 loop did not stop with a numeric result\n";
        return false;
    }

    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Digit0);
    // Capture the terminal counter representation produced by the original
    // microcode tables; the manual supplies no separate display fixture.
    const bool counterTerminated =
        expectPrefix("DSNZ0 counter R0", core, " 00000001.");
    press(core, pmk::PhysicalKey::Recall);
    press(core, pmk::PhysicalKey::Digit5);
    const bool accumulatorReturned = core.displayText() == result;
    if (!accumulatorReturned)
        std::cerr << "DSNZ0 displayed result differs from accumulator R5\n";
    return counterTerminated && accumulatorReturned;
}
} // namespace

int main() {
    const bool passed = circleArea() &&
                        indirectBranchesAndRegisterMutation() &&
                        conditionalIndirectBranch() &&
                        dsnzLoop();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
