/*
 * Qt-independent EMU145 adapter.
 * Sequencing and keyboard scan are derived from fixelsan/emu145 pmkemu/emulator.cpp.
 */
#include "calculator_core.hpp"

#include "ucommands.inc"
#include "synchro.inc"
#include "mcommands.inc"

#include <algorithm>

namespace pmk {
namespace {
constexpr char kSegments[16] = {
    // PMK-Android Emulator.show_symbols. The bundled digital font maps D to
    // the calculator's Cyrillic-looking segment glyph used in EГГ0Г.
    '0','1','2','3','4','5','6','7','8','9','-','L','C','D','E',' '
};

struct MemoryAddress { unsigned chip; unsigned nibble; };

constexpr std::array<unsigned, 15> kPageOneSwap = {
    10, 11, 6, 7, 2, 3, 4, 5, 0, 1, 14, 13, 12, 8, 9
};
constexpr std::array<MemoryAddress, 15> kMemoryPages = {{
    {1, 41}, {1, 83}, {1, 125}, {1, 167}, {1, 209}, {1, 251},
    {2, 41}, {2, 83}, {2, 125}, {2, 167}, {2, 209}, {2, 251},
    {3, 41}, {4, 41}, {5, 41}
}};

constexpr std::array<MemoryAddress, 15> kStackAddresses = {{
    {1, 34}, {1, 76}, {1, 118}, {1, 160}, {1, 202}, {1, 244},
    {2, 34}, {2, 76}, {2, 118}, {2, 160}, {2, 202}, {2, 244},
    {3, 34}, {4, 34}, {5, 34}
}};
constexpr std::array<unsigned, 5> kStackSwap = {{14, 13, 12, 8, 9}};
constexpr unsigned MemoryPhaseCount = 5;

MemoryAddress programAddress(unsigned address) {
    const unsigned group = address / 7;
    const unsigned withinGroup = address % 7;
    MemoryAddress result = kMemoryPages[kPageOneSwap[group]];
    if (withinGroup != 0)
        result.nibble = result.nibble - 42 + withinGroup * 6;
    return result;
}

void loadRoms(MicrocodeChip& chip,
              const unsigned int (&urom)[68],
              const unsigned char (&srom)[128][9],
              const unsigned int (&mrom)[256]) {
    for (unsigned i = 0; i < 68; ++i)
        chip.ucrom[i].raw = urom[i];
    for (unsigned i = 0; i < 128; ++i)
        for (unsigned j = 0; j < 9; ++j)
            chip.asprom[i][j] = srom[i][j];
    for (unsigned i = 0; i < 256; ++i)
        chip.cmdrom[i] = mrom[i];
}
}

CalculatorCore::CalculatorCore() {
    reset();
}

void CalculatorCore::reset() {
    ik1302_ = MicrocodeChip{};
    ik1303_ = MicrocodeChip{};
    ik1306_ = MicrocodeChip{};
    // Re-load ROMs after value-resetting the chips.
    loadRoms(ik1302_, ik1302_urom, ik1302_srom, ik1302_mrom);
    loadRoms(ik1303_, ik1303_urom, ik1303_srom, ik1303_mrom);
    loadRoms(ik1306_, ik1306_urom, ik1306_srom, ik1306_mrom);
    ik1302_.init();
    ik1303_.init();
    ik1306_.init();
    ir2_1_.reset();
    ir2_2_.reset();
    hardware_.reset();
    stackDisplaySnapshots_ = {};
    setAngleMode(mode_);
    chain_ = false;
    sync_ = false;
    dcycle_ = 0;
    ticks_ = 0;
    memorySyncCounter_ = 0;
}

void CalculatorCore::pressMatrixKey(unsigned row, unsigned column) {
    hardware_.keyboard().pressContact(row, column);
}

void CalculatorCore::setAngleMode(AngleMode mode) {
    mode_ = mode;
    const auto position = mode == AngleMode::Radians
        ? hardware::AngleSelector::Position::Radians
        : mode == AngleMode::Degrees
            ? hardware::AngleSelector::Position::Degrees
            : hardware::AngleSelector::Position::Grads;
    hardware_.angleSelector().setPosition(position);
}

void CalculatorCore::releaseKey() {
    hardware_.keyboard().releaseContact();
}

void CalculatorCore::advanceTicks(std::size_t count) {
    while (count-- != 0)
        tick();
}

void CalculatorCore::advanceRealtimeSlice() {
    advanceTicks(168);
}

void CalculatorCore::tick() {
    unsigned char segment = 0;

    const auto keyboardLines =
        hardware_.keyboard().sample(ik1302_.dcount, ik1302_.strobe(), ik1302_.ucount);
    chain_ = ik1302_.tick(chain_, keyboardLines.k1, keyboardLines.k2,
                          &dcycle_, &sync_, &segment);
    hardware_.display().advanceMicrotick();
    hardware_.display().acceptScan(ik1302_.strobe(), dcycle_, segment, sync_);

    const bool anglePin =
        hardware_.angleSelector().inputLevel(ik1303_.dcount);

    chain_ = ik1303_.tick(chain_, anglePin, false, nullptr, nullptr, nullptr);
    chain_ = ik1306_.tick(chain_, false, false, nullptr, nullptr, nullptr);
    chain_ = ir2_1_.tick(chain_);
    chain_ = ir2_2_.tick(chain_);
    ik1302_.pretick(chain_);
    ++ticks_;
    // PMK-Android reads the circulating registers when IR2_1.microtick is 84.
    // One Java nibble tick is four bit ticks in this core.
    if (ticks_ % DelayMemory::BitLength == 84U * 4U) {
        memorySyncCounter_ = (memorySyncCounter_ + 1U) % MemoryPhaseCount;
        if (memorySyncCounter_ == MemoryPhaseCount - 1U)
            refreshStackDisplaySnapshots();
    }
}

hardware::VfdSnapshot CalculatorCore::stackValueSnapshot(
    unsigned chip, unsigned address) const {
    const auto nibble = [&](unsigned index) {
        return readProgramNibble(chip, index);
    };

    int exponent = int(nibble(address - 3)) * 10 + nibble(address - 6);
    if (nibble(address) == 9)
        exponent = -(100 - exponent);

    unsigned firstDigit = 0;
    while (nibble(address - 33 + firstDigit * 3) == 0) {
        if (exponent == 7 - int(firstDigit) || firstDigit == 7)
            break;
        ++firstDigit;
    }

    std::string formatted(1, nibble(address - 9) == 9 ? '-' : ' ');
    bool decimalPlaced = false;
    unsigned outputDigit = 0;
    for (int source = 7; source >= int(firstDigit); --source, ++outputDigit) {
        formatted.push_back(kSegments[nibble(address - 33 + unsigned(source) * 3)]);
        if ((outputDigit == 0 && (exponent < 0 || exponent > 7)) ||
            int(outputDigit) == exponent) {
            formatted.push_back('.');
            decimalPlaced = true;
        }
    }
    if (!decimalPlaced)
        formatted.push_back('.');
    if (exponent < 0 || exponent > 7) {
        const int target = exponent < 0 ? 10 : 11;
        while (int(formatted.size()) < target)
            formatted.push_back(' ');
        if (exponent < 0) {
            formatted += '-';
            if (exponent > -10)
                formatted += '0';
            formatted += std::to_string(-exponent);
        } else {
            if (exponent < 10)
                formatted += '0';
            formatted += std::to_string(exponent);
        }
    }

    std::array<std::uint8_t, 12> electricalCodes;
    electricalCodes.fill(0x0fU);
    unsigned visibleCell = 0;
    for (const char glyph : formatted) {
        if (glyph == '.') {
            if (visibleCell != 0) {
                const unsigned electrical =
                    hardware::VfdTube::electricalCell(visibleCell - 1);
                electricalCodes[electrical] |= 0x80U;
            }
            continue;
        }
        if (visibleCell >= 12)
            break;
        const char* found = std::find(std::begin(kSegments),
                                     std::end(kSegments), glyph);
        const std::uint8_t code = found == std::end(kSegments)
            ? 0x0fU : static_cast<std::uint8_t>(found - std::begin(kSegments));
        electricalCodes[hardware::VfdTube::electricalCell(visibleCell)] = code;
        ++visibleCell;
    }
    return hardware::VfdTube::steadySnapshot(electricalCodes);
}

void CalculatorCore::refreshStackDisplaySnapshots() {
    // Upstream order is X1, X, Y, Z, T. X1 is not part of the four-level RPN
    // stack, so the expanded display intentionally starts at element one.
    for (unsigned level = 0; level < stackDisplaySnapshots_.size(); ++level) {
        const MemoryAddress location = kStackAddresses[kStackSwap[level + 1]];
        stackDisplaySnapshots_[level] =
            stackValueSnapshot(location.chip, location.nibble);
    }
}

std::string CalculatorCore::displayText() const {
    std::string result;
    result.reserve(24);
    const auto append = [&](unsigned index, std::string& out) {
        const auto value = hardware_.display().codes()[index];
        out.push_back(kSegments[value & 0x0fU]);
        if ((value & 0x80U) != 0)
            out.push_back('.');
    };
    for (int i = 8; i >= 0; --i)
        append(static_cast<unsigned>(i), result);
    for (int i = 11; i >= 9; --i)
        append(static_cast<unsigned>(i), result);
    return result;
}

std::string CalculatorCore::displayGlyphText() const {
    std::string result;
    result.reserve(24);
    const auto append = [&](unsigned index) {
        const auto value = hardware_.display().codes()[index];
        result.push_back(kSegments[value & 0x0fU]);
        result.push_back((value & 0x80U) != 0 ? '.' : '/');
    };
    for (int i = 8; i >= 0; --i)
        append(static_cast<unsigned>(i));
    for (int i = 11; i >= 9; --i)
        append(static_cast<unsigned>(i));
    return result;
}

void CalculatorCore::alignProgramMemory() {
    // Java imports at IR2_1.microtick == 84. One Java nibble tick is four
    // EMU145 bit ticks, and the delay-memory head advances once per bit tick.
    // The complete MK-61 ring has five devices, so the same IR2 bit position
    // occurs at five different page phases. Program addresses are valid only
    // at the register-dump phase (the same phase used for stack snapshots).
    constexpr std::uint64_t alignedBit = 84U * 4U;
    constexpr unsigned alignedPhase = MemoryPhaseCount - 1U;
    while (ticks_ % DelayMemory::BitLength != alignedBit ||
           memorySyncCounter_ != alignedPhase)
        tick();
}

std::uint8_t CalculatorCore::readProgramNibble(unsigned chip,
                                               unsigned address) const {
    if (chip <= 2) {
        constexpr unsigned alignedNibble = 84;
        constexpr unsigned nibbleCount = DelayMemory::BitLength / 4;
        const unsigned offset =
            (address + nibbleCount - alignedNibble) % nibbleCount;
        return chip == 1 ? ir2_1_.nibbleFromHead(offset)
                         : ir2_2_.nibbleFromHead(offset);
    }
    if (chip == 3) return ik1302_.memoryNibble(address);
    if (chip == 4) return ik1303_.memoryNibble(address);
    return ik1306_.memoryNibble(address);
}

void CalculatorCore::writeProgramNibble(unsigned chip, unsigned address,
                                        std::uint8_t value) {
    value &= 0x0fU;
    if (chip <= 2) {
        constexpr unsigned alignedNibble = 84;
        constexpr unsigned nibbleCount = DelayMemory::BitLength / 4;
        const unsigned offset =
            (address + nibbleCount - alignedNibble) % nibbleCount;
        if (chip == 1) ir2_1_.setNibbleFromHead(offset, value);
        else ir2_2_.setNibbleFromHead(offset, value);
    } else if (chip == 3) {
        ik1302_.setMemoryNibble(address, value);
    } else if (chip == 4) {
        ik1303_.setMemoryNibble(address, value);
    } else {
        ik1306_.setMemoryNibble(address, value);
    }
}

CalculatorCore::Program CalculatorCore::exportProgram() {
    alignProgramMemory();
    Program program{};
    for (unsigned address = 0; address < program.size(); ++address) {
        const auto location = programAddress(address);
        const auto high = readProgramNibble(location.chip, location.nibble);
        const auto low = readProgramNibble(location.chip, location.nibble - 3);
        program[address] = static_cast<std::uint8_t>((high << 4U) | low);
    }
    return program;
}

void CalculatorCore::importProgram(const Program& program) {
    alignProgramMemory();
    for (unsigned address = 0; address < program.size(); ++address) {
        const auto location = programAddress(address);
        writeProgramNibble(location.chip, location.nibble,
                           program[address] >> 4U);
        writeProgramNibble(location.chip, location.nibble - 3,
                           program[address]);
    }
}

} // namespace pmk

