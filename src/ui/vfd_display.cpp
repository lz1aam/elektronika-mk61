#include "vfd_display.hpp"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>

#include <algorithm>
#include <vector>

namespace pmk::ui {
namespace {

// The ИЛЦ2-12/8Л anode artwork is oblique: the top of each vertical bar is
// displaced to the right relative to its foot.  This value is measured from
// the supplied macro photographs (roughly 8.5 degrees), not added as
// a generic "digital italic" effect.
constexpr qreal SegmentLean = 0.15;

QPainterPath horizontalSegment(qreal left, qreal top, qreal width,
                               qreal thickness) {
    const qreal bevel = thickness * 0.42;
    QPainterPath path;
    path.moveTo(left + bevel, top);
    path.lineTo(left + width - bevel, top);
    path.lineTo(left + width, top + thickness / 2.0);
    path.lineTo(left + width - bevel, top + thickness);
    path.lineTo(left + bevel, top + thickness);
    path.lineTo(left, top + thickness / 2.0);
    path.closeSubpath();
    return path;
}

QPainterPath verticalSegment(qreal left, qreal top, qreal thickness,
                             qreal height) {
    const qreal bevel = thickness * 0.42;
    QPainterPath path;
    path.moveTo(left + thickness / 2.0, top);
    path.lineTo(left + thickness, top + bevel);
    path.lineTo(left + thickness, top + height - bevel);
    path.lineTo(left + thickness / 2.0, top + height);
    path.lineTo(left, top + height - bevel);
    path.lineTo(left, top + bevel);
    path.closeSubpath();
    return path;
}

int scaledAlpha(int alpha, qreal intensity) {
    return std::clamp(qRound(alpha * intensity), 0, 255);
}

void paintLitPath(QPainter& painter, const QPainterPath& path,
                  qreal intensity, qreal glowScale = 1.0) {
    // VFD light is not a flat neon fill: the phosphor has a pale cyan core,
    // surrounded by a broader, lower-energy blue-green bloom through the
    // calculator's tinted window.
    painter.setBrush(QColor(0, 229, 232, scaledAlpha(16, intensity)));
    painter.setPen(QPen(QColor(0, 229, 232, scaledAlpha(16, intensity)),
                        7.4 * glowScale,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);

    painter.setBrush(QColor(4, 241, 235, scaledAlpha(66, intensity)));
    painter.setPen(QPen(QColor(4, 241, 235, scaledAlpha(58, intensity)),
                        2.8 * glowScale,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(35, 255, 241, scaledAlpha(238, intensity)));
    painter.drawPath(path);
}

QPainterPath electrodeStroke(const QPointF& from, const QPointF& to,
                             qreal width) {
    QPainterPath centerLine;
    centerLine.moveTo(from);
    centerLine.lineTo(to);
    QPainterPathStroker stroker;
    stroker.setWidth(width);
    stroker.setCapStyle(Qt::FlatCap);
    stroker.setJoinStyle(Qt::MiterJoin);
    return stroker.createStroke(centerLine);
}

void paintGlyphAnodes(QPainter& painter, const QRectF& cell, char glyph,
                      bool lit, bool powered) {
    const QRectF r = cell.adjusted(cell.width() * 0.20, cell.height() * 0.16,
                                   -cell.width() * 0.20,
                                   -cell.height() * 0.16);
    const QPointF topLeft(r.left(), r.top());
    const QPointF topRight(r.right(), r.top());
    const QPointF middleLeft(r.left(), r.center().y());
    const QPointF middle(r.center().x(), r.center().y());
    const QPointF middleRight(r.right(), r.center().y());
    const QPointF bottomLeft(r.left(), r.bottom());
    const QPointF bottomMiddle(r.center().x(), r.bottom());
    const QPointF bottomRight(r.right(), r.bottom());
    const qreal thickness = std::max<qreal>(1.0, cell.width() * 0.13);
    std::vector<QPainterPath> anodes;
    const auto add = [&](const QPointF& from, const QPointF& to) {
        anodes.push_back(electrodeStroke(from, to, thickness));
    };

    switch (glyph) {
    case 'F':
        add(topLeft, topRight);
        add(topLeft, bottomLeft);
        add(middleLeft, middleRight);
        break;
    case 'K':
        add(topLeft, bottomLeft);
        add(middleLeft, topRight);
        add(middleLeft, bottomRight);
        break;
    case 'T':
        add(topLeft, topRight);
        add(QPointF(r.center().x(), r.top()), bottomMiddle);
        break;
    case 'Z':
        add(topLeft, topRight);
        add(topRight, bottomLeft);
        add(bottomLeft, bottomRight);
        break;
    case 'Y':
        add(topLeft, middle);
        add(topRight, middle);
        add(middle, bottomMiddle);
        break;
    case 'X':
        add(topLeft, bottomRight);
        add(topRight, bottomLeft);
        break;
    default:
        return;
    }

    for (const auto& anode : anodes) {
        if (lit && powered)
            paintLitPath(painter, anode, 0.86, 0.43);
        else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(12, 55, 57, powered ? 18 : 10));
            painter.drawPath(anode);
        }
    }
}

void paintDot(QPainter& painter, const QPointF& center, qreal radius,
              qreal intensity, bool powered) {
    if (!powered || intensity <= 0.001) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(12, 55, 57, 20));
        painter.drawEllipse(center, radius, radius);
        return;
    }
    painter.setBrush(QColor(0, 229, 232, scaledAlpha(17, intensity)));
    painter.setPen(QPen(QColor(0, 229, 232, scaledAlpha(17, intensity)), 6.8));
    painter.drawEllipse(center, radius, radius);
    painter.setBrush(QColor(4, 241, 235, scaledAlpha(70, intensity)));
    painter.setPen(QPen(QColor(4, 241, 235, scaledAlpha(59, intensity)), 2.5));
    painter.drawEllipse(center, radius, radius);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(42, 255, 243, scaledAlpha(240, intensity)));
    painter.drawEllipse(center, radius, radius);
}

void paintDigit(QPainter& painter, const QRectF& digit,
                const hardware::VfdCellState& cell, bool powered) {
    const qreal visibleWidth = digit.width() + SegmentLean * digit.height();
    const qreal thickness = visibleWidth * 0.175;
    // Build all seven anodes from the same six junction centres.  The former
    // implementation positioned horizontal and vertical paths independently,
    // so their proportions and overlaps changed unpredictably when a single
    // endpoint was adjusted.  The macro photograph shows a narrow, consistent
    // insulating cut around each junction; trim every incident centreline by
    // the same amount and leave the accepted outer envelope untouched.
    const qreal junctionTrim = thickness * 0.14;
    const qreal leftJunction = digit.left() + thickness / 2.0;
    const qreal rightJunction = digit.right() - thickness / 2.0;
    const qreal topJunction = digit.top() + thickness * 1.12;
    const qreal middleJunction = digit.center().y();
    const qreal bottomJunction = digit.bottom() - thickness * 1.12;

    const auto horizontalAt = [&](qreal y) {
        const qreal left = leftJunction + junctionTrim;
        const qreal right = rightJunction - junctionTrim;
        return horizontalSegment(left, y - thickness / 2.0,
                                 right - left, thickness);
    };
    const auto verticalBetween = [&](qreal x, qreal upper, qreal lower) {
        const qreal top = upper + junctionTrim;
        const qreal bottom = lower - junctionTrim;
        return verticalSegment(x - thickness / 2.0, top,
                               thickness, bottom - top);
    };

    std::array<QPainterPath, 7> paths = {{
        horizontalAt(topJunction),
        verticalBetween(rightJunction, topJunction, middleJunction),
        verticalBetween(rightJunction, middleJunction, bottomJunction),
        horizontalAt(bottomJunction),
        verticalBetween(leftJunction, middleJunction, bottomJunction),
        verticalBetween(leftJunction, topJunction, middleJunction),
        horizontalAt(middleJunction),
    }};

    // Shear around the lower baseline so that cell spacing and decimal-point
    // alignment remain stable while the luminous bars lean to the right.
    const QTransform oblique(1.0, 0.0, -SegmentLean, 1.0,
                             SegmentLean * digit.bottom(), 0.0);
    for (QPainterPath& path : paths)
        path = oblique.map(path);

    painter.setPen(Qt::NoPen);
    for (int segment = 0; segment < int(paths.size()); ++segment) {
        const qreal intensity = powered
            ? qreal(cell.phosphor[segment]) / hardware::VfdTube::FullExcitation
            : 0.0;
        if (intensity > 0.001)
            paintLitPath(painter, paths[segment], intensity);
        else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(12, 55, 57, powered ? 18 : 10));
            painter.drawPath(paths[segment]);
        }
    }

    const QPointF dot = oblique.map(
        QPointF(digit.right() + thickness * 1.45,
                digit.bottom() - thickness * 0.72));
    const qreal dotIntensity = powered
        ? qreal(cell.phosphor[7]) / hardware::VfdTube::FullExcitation : 0.0;
    paintDot(painter, dot, thickness * 0.42, dotIntensity, powered);
}

void paintControlGrid(QPainter& painter, const QRectF& bounds, bool powered) {
    // The ИЛЦ2-12/8Л macro photographs show the digit-control electrode as a
    // fine honeycomb between the phosphor anodes and the cathode filaments.
    // It is obvious under magnification but nearly lost through the blue
    // front filter, so it must occlude lit phosphor without becoming a panel
    // decoration over the dark glass.
    constexpr qreal radius = 2.1;
    constexpr qreal halfHeight = 1.82;
    constexpr qreal columnStep = radius * 1.5;
    constexpr qreal rowStep = halfHeight * 2.0;
    QPainterPath mesh;
    int column = 0;
    for (qreal cx = bounds.left() - radius; cx <= bounds.right() + radius;
         cx += columnStep, ++column) {
        const qreal yOffset = (column & 1) ? halfHeight : 0.0;
        for (qreal cy = bounds.top() - rowStep + yOffset;
             cy <= bounds.bottom() + rowStep; cy += rowStep) {
            mesh.moveTo(cx - radius, cy);
            mesh.lineTo(cx - radius / 2.0, cy - halfHeight);
            mesh.lineTo(cx + radius / 2.0, cy - halfHeight);
            mesh.lineTo(cx + radius, cy);
            mesh.lineTo(cx + radius / 2.0, cy + halfHeight);
            mesh.lineTo(cx - radius / 2.0, cy + halfHeight);
            mesh.closeSubpath();
        }
    }

    painter.save();
    painter.setClipRect(bounds.adjusted(0.0, 13.0, 0.0, -13.0));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(76, 101, 98, powered ? 5 : 3), 0.55,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(mesh);
    painter.setPen(QPen(QColor(0, 9, 11, powered ? 40 : 18), 0.28,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(mesh);
    painter.restore();
}

void paintCathodeFilaments(QPainter& painter, const QRectF& bounds,
                           bool powered) {
    // Two oxide-coated tungsten cathode wires cross the active area in the
    // supplied macro photographs. They run in front of the control grid.
    const std::array<qreal, 2> yPositions =
        {{bounds.center().y() - 18.0, bounds.center().y() + 18.0}};
    for (const qreal y : yPositions) {
        const QPointF left(bounds.left() + 5.0, y);
        const QPointF right(bounds.right() - 5.0, y);
        painter.setPen(QPen(QColor(43, 31, 38, powered ? 72 : 34), 0.9,
                            Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(left, right);
        painter.setPen(QPen(QColor(137, 108, 112, powered ? 42 : 20), 0.32,
                            Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(left, right);
    }
}

} // namespace

void paintVfdGlass(QPainter& painter, const QRectF& bounds, bool powered) {
    painter.save();
    painter.setClipRect(bounds);

    QLinearGradient glass(bounds.topLeft(), bounds.bottomLeft());
    glass.setColorAt(0.0, powered ? QColor(2, 15, 23) : QColor(1, 5, 7));
    glass.setColorAt(0.45, powered ? QColor(2, 25, 31) : QColor(1, 8, 10));
    glass.setColorAt(1.0, powered ? QColor(1, 10, 18) : QColor(0, 3, 5));
    painter.fillRect(bounds, glass);

    // A restrained reflection keeps the cover glass from looking like a
    // modern OLED while remaining almost invisible at 1x.
    QLinearGradient reflection(bounds.topLeft(),
                               QPointF(bounds.left(), bounds.top() + bounds.height() * 0.34));
    reflection.setColorAt(0.0, QColor(54, 101, 111, 24));
    reflection.setColorAt(1.0, QColor(15, 54, 62, 0));
    painter.fillRect(QRectF(bounds.left(), bounds.top(), bounds.width(),
                            bounds.height() * 0.34), reflection);
    painter.restore();
}

void paintVfdElectrodes(QPainter& painter, const QRectF& bounds,
                        const hardware::VfdSnapshot& snapshot,
                        bool powered, bool fIndicator, bool kIndicator) {
    painter.save();
    painter.setClipRect(bounds);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal slotWidth = bounds.width() / 12.0;
    const qreal digitHeight = std::min(bounds.height() * 0.64, slotWidth * 1.55);
    // The documented 2.4:4.3 ratio describes the visible, already-oblique
    // character envelope.  Subtract the shear displacement from the base
    // construction width so adjacent cells do not drift into each other.
    const qreal digitWidth = digitHeight * (2.4 / 4.3);
    const qreal digitBaseWidth = digitWidth - SegmentLean * digitHeight;
    const qreal top = bounds.center().y() - digitHeight / 2.0;
    for (unsigned slot = 0; slot < 12; ++slot) {
        const qreal slotLeft = bounds.left() + slot * slotWidth;
        // The decimal point consumes the right side of each physical cell,
        // just as it does in the tube; the digit itself is slightly left-set.
        const QRectF digit(slotLeft + (slotWidth - digitWidth) * 0.25,
                           top, digitBaseWidth, digitHeight);
        paintDigit(painter, digit,
                   snapshot.cells[hardware::VfdTube::electricalCell(slot)],
                   powered);
    }

    // F/K are auxiliary virtual electrodes, not font painted on the cover.
    // They share the phosphor and are occluded by the same grid and cathodes.
    const QRectF fCell(bounds.left() + 4.0, bounds.top() + 1.5, 11.0, 16.0);
    const QRectF kCell(bounds.left() + 17.0, bounds.top() + 1.5, 11.0, 16.0);
    paintGlyphAnodes(painter, fCell, 'F', fIndicator, powered);
    paintGlyphAnodes(painter, kCell, 'K', kIndicator, powered);

    // Physical front-to-back order: phosphor anodes, control-grid mesh, then
    // the directly heated cathode filaments nearest the cover glass.
    paintControlGrid(painter, bounds, powered);
    paintCathodeFilaments(painter, bounds, powered);

    painter.restore();
}

void paintVfdElectrodeLabel(QPainter& painter, const QRectF& bounds,
                            char glyph, bool powered) {
    painter.save();
    painter.setClipRect(bounds);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // Match the 11:16 F/K auxiliary-electrode proportion. The previous
    // 12:24 cell made T/Z/Y/X visibly taller and stylistically unrelated.
    constexpr qreal glyphWidth = 12.0;
    constexpr qreal glyphHeight = glyphWidth * (16.0 / 11.0);
    const QRectF glyphCell(bounds.center().x() - glyphWidth / 2.0,
                           bounds.center().y() - glyphHeight / 2.0,
                           glyphWidth, glyphHeight);
    paintGlyphAnodes(painter, glyphCell, glyph, powered, powered);
    paintControlGrid(painter, bounds, powered);
    paintCathodeFilaments(painter, bounds, powered);
    painter.restore();
}

void paintVfdDisplay(QPainter& painter, const QRectF& bounds,
                     const hardware::VfdSnapshot& snapshot,
                     bool powered, bool fIndicator, bool kIndicator) {
    paintVfdGlass(painter, bounds, powered);
    paintVfdElectrodes(painter, bounds, snapshot, powered,
                       fIndicator, kIndicator);
}

void paintVfdDisplay(QPainter& painter, const QRectF& bounds,
                     const std::array<std::uint8_t, 12>& chipSegments,
                     bool powered, bool fIndicator, bool kIndicator) {
    const auto snapshot = hardware::VfdTube::steadySnapshot(chipSegments);
    paintVfdDisplay(painter, bounds, snapshot, powered,
                    fIndicator, kIndicator);
}

} // namespace pmk::ui
