#include "calculator_io.hpp"

#include <stdexcept>

namespace pmk::hardware {

void KeyboardMatrix::reset() {
    encodedContact_ = 0;
}

void KeyboardMatrix::pressContact(unsigned row, unsigned column) {
    if (row < 1 || row > 3 || column > 9)
        throw std::out_of_range("K745IK1302 keyboard-matrix coordinate");
    encodedContact_ = static_cast<std::uint16_t>((row << 8U) | column);
}

void KeyboardMatrix::releaseContact() {
    encodedContact_ = 0;
}

KeyboardLines KeyboardMatrix::sample(unsigned zeroBasedDCount,
                                     bool displayStrobe) {
    KeyboardLines lines{};
    if (!displayStrobe)
        return lines;

    if (zeroBasedDCount == 12)
        lines.k2 = true; // documented D13-to-K2 fixed connection

    if (encodedContact_ == 0 ||
        zeroBasedDCount + 1 != 2 + (encodedContact_ & 0xffU))
        return lines;

    switch (encodedContact_ >> 8U) {
    case 1: lines.k1 = true;  lines.k2 = false; break;
    case 2: lines.k1 = false; lines.k2 = true;  break;
    case 3: lines.k1 = true;  lines.k2 = true;  break;
    default: break;
    }
    return lines;
}

bool AngleSelector::inputLevel(unsigned zeroBasedDCount) const {
    const unsigned selectedD = position_ == Position::Radians ? 9U
                             : position_ == Position::Degrees ? 10U : 11U;
    return zeroBasedDCount != selectedD;
}

void VfdTube::reset() {
    codes_.fill(0x0fU);
    snapshot_ = VfdSnapshot{};
    for (auto& cell : snapshot_.cells)
        cell.code = 0x0fU;
    frameDriven_ = false;
}

void VfdTube::advanceMicrotick() {
    for (auto& cell : snapshot_.cells) {
        for (std::uint16_t& charge : cell.phosphor) {
            charge = charge > DecayPerMicrotick
                   ? static_cast<std::uint16_t>(charge - DecayPerMicrotick)
                   : 0U;
        }
    }
}

void VfdTube::excite(unsigned cell, std::uint8_t mask) {
    for (unsigned segment = 0; segment < 8; ++segment) {
        if ((mask & (1U << segment)) != 0)
            snapshot_.cells[cell].phosphor[segment] = FullExcitation;
    }
}

void VfdTube::acceptScan(bool displayStrobe, unsigned oneBasedDcycle,
                         std::uint8_t codeAndDecimal, bool sync) {
    if (displayStrobe && oneBasedDcycle > 1 && oneBasedDcycle < 14) {
        const unsigned cell = oneBasedDcycle - 2;
        codes_[cell] = codeAndDecimal;
        snapshot_.cells[cell].code = codeAndDecimal;
        std::uint8_t mask = decodedVfdSegments(codeAndDecimal);
        if ((codeAndDecimal & 0x80U) != 0)
            mask |= SegmentDecimal;
        excite(cell, mask);
        frameDriven_ = true;
    }

    if (!sync)
        return;

    ++snapshot_.completedScans;
    // EMU145's electrical boundary samples the display-enable command at
    // SYNC.  A disabled command produces an electrically blank frame even if
    // an earlier strobe in the interval happened to excite an anode.
    if (!displayStrobe || !frameDriven_) {
        codes_.fill(0x0fU);
        for (auto& cell : snapshot_.cells)
            cell.code = 0x0fU;
    }
    frameDriven_ = false;
}

VfdSnapshot VfdTube::steadySnapshot(
    const std::array<std::uint8_t, 12>& electricalCodes) {
    VfdTube tube;
    for (unsigned cell = 0; cell < electricalCodes.size(); ++cell) {
        const std::uint8_t code = electricalCodes[cell];
        tube.codes_[cell] = code;
        tube.snapshot_.cells[cell].code = code;
        std::uint8_t mask = decodedVfdSegments(code);
        if ((code & 0x80U) != 0)
            mask |= SegmentDecimal;
        tube.excite(cell, mask);
    }
    return tube.snapshot_;
}

void CalculatorIo::reset() {
    keyboard_.reset();
    angleSelector_.setPosition(AngleSelector::Position::Radians);
    display_.reset();
}

} // namespace pmk::hardware
