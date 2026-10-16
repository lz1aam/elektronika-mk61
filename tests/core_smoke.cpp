#include "calculator_core.hpp"
#include "keymap.hpp"
#include "program_text.hpp"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string_view>

namespace {
constexpr bool portableInstructionLayout() {
    struct Field {
        std::uint32_t (pmk::MicroInstruction::*decode)() const;
        unsigned offset;
        unsigned width;
    };
    constexpr std::array<Field, 23> fields = {{
        {&pmk::MicroInstruction::a_r, 0, 1},
        {&pmk::MicroInstruction::a_m, 1, 1},
        {&pmk::MicroInstruction::a_st, 2, 1},
        {&pmk::MicroInstruction::a_nr, 3, 1},
        {&pmk::MicroInstruction::a_10nl, 4, 1},
        {&pmk::MicroInstruction::a_s, 5, 1},
        {&pmk::MicroInstruction::a_4, 6, 1},
        {&pmk::MicroInstruction::b_s, 7, 1},
        {&pmk::MicroInstruction::b_ns, 8, 1},
        {&pmk::MicroInstruction::b_s1, 9, 1},
        {&pmk::MicroInstruction::b_6, 10, 1},
        {&pmk::MicroInstruction::b_1, 11, 1},
        {&pmk::MicroInstruction::g_l, 12, 1},
        {&pmk::MicroInstruction::g_nl, 13, 1},
        {&pmk::MicroInstruction::g_nt, 14, 1},
        {&pmk::MicroInstruction::r0, 15, 3},
        {&pmk::MicroInstruction::r_1, 18, 1},
        {&pmk::MicroInstruction::r_2, 19, 1},
        {&pmk::MicroInstruction::m, 20, 1},
        {&pmk::MicroInstruction::l, 21, 1},
        {&pmk::MicroInstruction::s, 22, 2},
        {&pmk::MicroInstruction::s1, 24, 2},
        {&pmk::MicroInstruction::st, 26, 2},
    }};
    for (unsigned bit = 0; bit < 32; ++bit) {
        const pmk::MicroInstruction instruction{std::uint32_t{1} << bit};
        for (const auto& field : fields) {
            const auto expected = bit >= field.offset &&
                bit < field.offset + field.width
                ? std::uint32_t{1} << (bit - field.offset) : 0U;
            if ((instruction.*field.decode)() != expected)
                return false;
        }
    }
    for (const auto& field : fields) {
        const pmk::MicroInstruction instruction{0xffffffffU};
        if ((instruction.*field.decode)() !=
            (std::uint32_t{1} << field.width) - 1U)
            return false;
    }
    return true;
}
static_assert(portableInstructionLayout());

bool delayMemoryMatchesShiftRegister() {
    pmk::DelayMemory memory;
    std::array<bool, pmk::DelayMemory::BitLength> reference{};
    std::uint32_t random = 0x61U;
    for (unsigned tick = 0; tick < 4 * pmk::DelayMemory::BitLength; ++tick) {
        random = random * 1664525U + 1013904223U;
        const bool input = (random >> 31) != 0;
        if (memory.pretick() != reference[0] || memory.tick(input) != reference[0])
            return false;
        std::move(reference.begin() + 1, reference.end(), reference.begin());
        reference.back() = input;
        if (tick % 131 == 0) {
            const unsigned offset = (tick / 131) % 252;
            memory.setNibbleFromHead(offset, 0x0a);
            for (unsigned bit = 0; bit < 4; ++bit)
                reference[offset * 4 + bit] = ((0x0aU >> bit) & 1U) != 0;
            for (unsigned nibble = 0; nibble < 252; ++nibble) {
                unsigned expected = 0;
                for (unsigned bit = 0; bit < 4; ++bit)
                    expected |= unsigned(reference[nibble * 4 + bit]) << bit;
                if (memory.nibbleFromHead(nibble) != expected)
                    return false;
            }
        }
    }
    memory.reset();
    for (unsigned i = 0; i < 252; ++i)
        if (memory.nibbleFromHead(i) != 0)
            return false;
    return !memory.pretick();
}

std::string run(std::initializer_list<pmk::PhysicalKey> keys) {
    pmk::CalculatorCore core;
    core.advanceTicks(250000);
    for (const auto key : keys) {
        const auto matrix = pmk::matrixKey(key);
        core.pressMatrixKey(matrix.row, matrix.column);
        core.advanceTicks(50000);
        core.releaseKey();
        core.advanceTicks(500000);
    }
    return core.displayText();
}

bool beginsWith(const std::string& actual, std::string_view expected) {
    if (actual.starts_with(expected))
        return true;
    std::cerr << "expected display prefix [" << expected << "], got ["
              << actual << "]\n";
    return false;
}
}

int main() {
    if (!delayMemoryMatchesShiftRegister()) {
        std::cerr << "serial memory differs from reference shift register\n";
        return EXIT_FAILURE;
    }
    pmk::CalculatorCore core;
    core.advanceTicks(250000);
    const auto powerOn = core.displayText();
    const auto& powerOnTube = core.displaySnapshot();
    const bool tubeWasScanned = powerOnTube.completedScans != 0;
    const bool anodeWasExcited = std::any_of(
        powerOnTube.cells.begin(), powerOnTube.cells.end(),
        [](const pmk::hardware::VfdCellState& cell) {
            return std::any_of(cell.phosphor.begin(), cell.phosphor.end(),
                               [](std::uint16_t charge) { return charge != 0; });
        });
    if (!tubeWasScanned || !anodeWasExcited) {
        std::cerr << "microcode did not drive the multiplexed VFD bus\n";
        return EXIT_FAILURE;
    }

    const auto press = [&core](pmk::PhysicalKey key) {
        const auto matrix = pmk::matrixKey(key);
        core.pressMatrixKey(matrix.row, matrix.column);
        core.advanceTicks(50000);
        core.releaseKey();
        core.advanceTicks(500000);
    };

    press(pmk::PhysicalKey::Digit1);
    const auto afterKey = core.displayText();

    std::cout << "power-on: [" << powerOn << "]\n";
    std::cout << "after digit 1: [" << afterKey << "]\n";
    std::cout << "ticks: " << core.tickCount() << "\n";

    if (core.keyContactActive()) {
        std::cerr << "released key contact remained active\n";
        return EXIT_FAILURE;
    }
    if (powerOn == afterKey) {
        std::cerr << "display did not change after a real microcode key event\n";
        return EXIT_FAILURE;
    }

    press(pmk::PhysicalKey::Enter);
    press(pmk::PhysicalKey::Digit2);
    const auto& stack = core.stackDisplaySnapshots();
    const auto stackContains = [](const pmk::hardware::VfdSnapshot& snapshot,
                                  std::uint8_t code) {
        return std::any_of(snapshot.cells.begin(), snapshot.cells.end(),
                           [code](const pmk::hardware::VfdCellState& cell) {
                               return (cell.code & 0x0fU) == code;
                           });
    };
    if (!stackContains(stack[0], 2) || !stackContains(stack[1], 1)) {
        std::cerr << "circulating-memory stack display is not X=2, Y=1\n";
        return EXIT_FAILURE;
    }
    press(pmk::PhysicalKey::Add);
    const auto sum = core.displayText();
    std::cout << "1 enter 2 +: [" << sum << "]\n";
    if (sum.find('3') == std::string::npos) {
        std::cerr << "microcode RPN addition did not produce 3\n";
        return EXIT_FAILURE;
    }

    const auto enter = pmk::PhysicalKey::Enter;
    const auto d2 = pmk::PhysicalKey::Digit2;
    const bool elementaryOperations =
        beginsWith(run({pmk::PhysicalKey::Digit7, enter, d2,
                        pmk::PhysicalKey::Subtract}), " 5.") &&
        beginsWith(run({pmk::PhysicalKey::Digit6, enter,
                        pmk::PhysicalKey::Digit3,
                        pmk::PhysicalKey::Multiply}), " 18.") &&
        beginsWith(run({pmk::PhysicalKey::Digit8, enter, d2,
                        pmk::PhysicalKey::Divide}), " 4.") &&
        beginsWith(run({d2, pmk::PhysicalKey::ChangeSign}), "-2.");
    if (!elementaryOperations)
        return EXIT_FAILURE;

    pmk::CalculatorCore::Program manualCircle{};
    const std::uint8_t circleCommands[] = {
        0x62, 0x22, 0x0e, 0x20, 0x12, 0x04, 0x13, 0x50
    };
    std::copy(std::begin(circleCommands), std::end(circleCommands),
              manualCircle.begin());
    pmk::CalculatorCore programCore;
    programCore.importProgram(manualCircle);
    const auto exported = programCore.exportProgram();
    if (exported != manualCircle) {
        std::cerr << "105-step physical program-memory round trip failed\n";
        return EXIT_FAILURE;
    }
    const std::string exportedText = pmk::exportProgramText(manualCircle);
    const auto parsedText = pmk::importProgramText(exportedText);
    if (parsedText.size != manualCircle.size() ||
        parsedText.bytes != manualCircle) {
        std::cerr << "PMK-Android mnemonic text round trip failed\n";
        return EXIT_FAILURE;
    }
    const auto tableText = pmk::importProgramText(
        "# | 00 | 01\n00 | В/О\t01 | С/П\n");
    if (tableText.size != 2 || tableText.bytes[0] != 0x52 ||
        tableText.bytes[1] != 0x50) {
        std::cerr << "PMK table-style text import failed\n";
        return EXIT_FAILURE;
    }

    programCore.advanceTicks(250000);
    const auto programPress = [&programCore](pmk::PhysicalKey key) {
        const auto matrix = pmk::matrixKey(key);
        programCore.pressMatrixKey(matrix.row, matrix.column);
        programCore.advanceTicks(50000);
        programCore.releaseKey();
        programCore.advanceTicks(500000);
    };
    programPress(pmk::PhysicalKey::Digit4);
    programPress(pmk::PhysicalKey::Store);
    programPress(pmk::PhysicalKey::Digit2);
    programPress(pmk::PhysicalKey::Return);
    programPress(pmk::PhysicalKey::RunStop);
    programCore.advanceTicks(4000000);
    std::cout << "manual circle program: [" << programCore.displayText() << "]\n";
    if (!beginsWith(programCore.displayText(), " 12.56637")) {
        std::cerr << "imported manual program did not execute as documented\n";
        return EXIT_FAILURE;
    }

    for (const auto invalid : {"", "# comments only\n", "00.1 02.2", "99.1"}) {
        try {
            (void)pmk::importProgramText(invalid);
            std::cerr << "invalid/empty program was accepted\n";
            return EXIT_FAILURE;
        } catch (const std::invalid_argument&) {
        }
    }
    auto legacyText = exportedText;
    for (const auto address : {"A1.", "A2.", "A3.", "A4."}) {
        const auto position = legacyText.find(address);
        if (position == std::string::npos)
            return EXIT_FAILURE;
        legacyText.replace(position, 3, "A0.");
    }
    if (pmk::importProgramText(legacyText).bytes != manualCircle)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

