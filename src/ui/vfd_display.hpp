#pragma once

#include "calculator_io.hpp"

#include <QRectF>

#include <array>
#include <cstdint>

class QPainter;

namespace pmk::ui {

inline constexpr auto SegmentA = hardware::SegmentA;
inline constexpr auto SegmentB = hardware::SegmentB;
inline constexpr auto SegmentC = hardware::SegmentC;
inline constexpr auto SegmentD = hardware::SegmentD;
inline constexpr auto SegmentE = hardware::SegmentE;
inline constexpr auto SegmentF = hardware::SegmentF;
inline constexpr auto SegmentG = hardware::SegmentG;

// The display chips emit a four-bit character code.  Codes 10--14 are the
// calculator's -, L, C, Г and E glyphs; 15 is blank.
constexpr std::uint8_t vfdSegmentMask(std::uint8_t code) {
    return hardware::decodedVfdSegments(code);
}

// Split layers are used by the expanded stack view: one uninterrupted front
// glass covers several independently rendered electrode groups.
void paintVfdGlass(QPainter& painter, const QRectF& bounds, bool powered);
void paintVfdElectrodes(QPainter& painter, const QRectF& bounds,
                        const hardware::VfdSnapshot& snapshot,
                        bool powered, bool fIndicator, bool kIndicator);
void paintVfdElectrodeLabel(QPainter& painter, const QRectF& bounds,
                            char glyph, bool powered);

void paintVfdDisplay(QPainter& painter, const QRectF& bounds,
                     const hardware::VfdSnapshot& snapshot,
                     bool powered, bool fIndicator, bool kIndicator);

// Paints the twelve-position ИЛЦ2-12/8Л VFD. Each input byte contains the
// chip glyph code in its low nibble and the decimal-point flag in bit 7.
void paintVfdDisplay(QPainter& painter, const QRectF& bounds,
                     const std::array<std::uint8_t, 12>& chipSegments,
                     bool powered, bool fIndicator, bool kIndicator);

} // namespace pmk::ui
