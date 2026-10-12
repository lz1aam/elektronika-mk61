#pragma once

#include <array>
#include <cstdint>

namespace pmk::hardware {

struct KeyboardLines {
    bool k1{};
    bool k2{};
};

// Electrical keyboard interface around K745IK1302.  A contact is selected by
// one of the D scan strobes and returns one of the K1/K2 combinations.  D13 is
// hard-wired to K2 in the calculator circuit.
class KeyboardMatrix {
public:
    void reset();
    void pressContact(unsigned row, unsigned column);
    void releaseContact();
    KeyboardLines sample(unsigned zeroBasedDCount, bool displayStrobe);
    bool contactActive() const { return encodedContact_ != 0; }

private:
    std::uint16_t encodedContact_{};
};

// The Р/ГРД/Г switch selects an active-low scanned input of K745IK1303.
class AngleSelector {
public:
    enum class Position { Radians, Degrees, Grads };

    void setPosition(Position position) { position_ = position; }
    Position position() const { return position_; }
    bool inputLevel(unsigned zeroBasedDCount) const;

private:
    Position position_{Position::Radians};
};

enum VfdSegment : std::uint8_t {
    SegmentA = 1U << 0,
    SegmentB = 1U << 1,
    SegmentC = 1U << 2,
    SegmentD = 1U << 3,
    SegmentE = 1U << 4,
    SegmentF = 1U << 5,
    SegmentG = 1U << 6,
    SegmentDecimal = 1U << 7,
};

// Decoder truth table for the display codes emitted by K745IK1302.  Codes
// A..E are the calculator's -, L, C, Г, E forms and F is electrically blank.
constexpr std::uint8_t decodedVfdSegments(std::uint8_t code) {
    constexpr std::array<std::uint8_t, 16> masks = {{
        SegmentA | SegmentB | SegmentC | SegmentD | SegmentE | SegmentF,
        SegmentB | SegmentC,
        SegmentA | SegmentB | SegmentD | SegmentE | SegmentG,
        SegmentA | SegmentB | SegmentC | SegmentD | SegmentG,
        SegmentB | SegmentC | SegmentF | SegmentG,
        SegmentA | SegmentC | SegmentD | SegmentF | SegmentG,
        SegmentA | SegmentC | SegmentD | SegmentE | SegmentF | SegmentG,
        SegmentA | SegmentB | SegmentC,
        SegmentA | SegmentB | SegmentC | SegmentD | SegmentE | SegmentF |
            SegmentG,
        SegmentA | SegmentB | SegmentC | SegmentD | SegmentF | SegmentG,
        SegmentG,
        SegmentD | SegmentE | SegmentF,
        SegmentA | SegmentD | SegmentE | SegmentF,
        SegmentA | SegmentF | SegmentG,
        SegmentA | SegmentD | SegmentE | SegmentF | SegmentG,
        0,
    }};
    return masks[code & 0x0fU];
}

struct VfdCellState {
    std::uint8_t code{0x0f};
    // A..G and decimal point, in the bit order of VfdSegment.
    std::array<std::uint16_t, 8> phosphor{};
};

struct VfdSnapshot {
    std::array<VfdCellState, 12> cells{};
    std::uint64_t completedScans{};
};

// Signal-level ИЛЦ2-12/8Л interface.  It consumes the D-cycle/code/comma/sync
// outputs from the original microcode engine and models multiplexed excitation
// plus deterministic phosphor persistence.  It deliberately does not invent
// a host-number representation.
class VfdTube {
public:
    static constexpr std::uint16_t FullExcitation = 0xffffU;
    static constexpr std::uint16_t DecayPerMicrotick = 128U;

    VfdTube() { reset(); }

    void reset();
    void advanceMicrotick();
    void acceptScan(bool displayStrobe, unsigned oneBasedDcycle,
                    std::uint8_t codeAndDecimal, bool sync);

    // Maps the left-to-right tube position to IK1302's electrical cell.
    static constexpr unsigned electricalCell(unsigned visiblePosition) {
        constexpr std::array<unsigned, 12> cells =
            {{8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 10, 9}};
        return cells[visiblePosition];
    }
    // Produces the same per-anode representation for values read directly
    // from circulating calculator memory (stack/register inspection).
    static VfdSnapshot steadySnapshot(
        const std::array<std::uint8_t, 12>& electricalCodes);

    const VfdSnapshot& snapshot() const { return snapshot_; }
    const std::array<std::uint8_t, 12>& codes() const { return codes_; }

private:
    void excite(unsigned cell, std::uint8_t mask);

    VfdSnapshot snapshot_{};
    std::array<std::uint8_t, 12> codes_{};
    bool frameDriven_{};
};

class CalculatorIo {
public:
    void reset();

    KeyboardMatrix& keyboard() { return keyboard_; }
    const KeyboardMatrix& keyboard() const { return keyboard_; }
    AngleSelector& angleSelector() { return angleSelector_; }
    const AngleSelector& angleSelector() const { return angleSelector_; }
    VfdTube& display() { return display_; }
    const VfdTube& display() const { return display_; }

private:
    KeyboardMatrix keyboard_;
    AngleSelector angleSelector_;
    VfdTube display_;
};

} // namespace pmk::hardware
