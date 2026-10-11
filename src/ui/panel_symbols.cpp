#include "panel_symbols.hpp"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QTransform>

#include <algorithm>

namespace pmk::ui {
namespace {

void paintSourceGlyph(QPainter& painter, const QRectF& bounds,
                      const QString& encoded, const QColor& color,
                      const QString& family) {
    if (family.isEmpty())
        return;

    QFont font(family);
    font.setPixelSize(100);
    QPainterPath source;
    source.addText(QPointF(0.0, 0.0), font, encoded);
    const QRectF ink = source.boundingRect();
    if (ink.isEmpty())
        return;

    const qreal targetHeight = std::min<qreal>(12.0, bounds.height() - 2.0);
    qreal scale = targetHeight / ink.height();
    if (ink.width() * scale > bounds.width() - 2.0)
        scale = (bounds.width() - 2.0) / ink.width();

    QTransform transform;
    transform.translate(bounds.center().x(), bounds.center().y());
    transform.scale(scale, scale);
    transform.translate(-ink.center().x(), -ink.center().y());
    const QPainterPath path = transform.map(source);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    // The source face was drawn markedly lighter than the calculator's other
    // panel printing.  A small outline expansion keeps its authentic contour
    // while matching the common screen-print stroke at native size.
    painter.setPen(QPen(color, 0.75, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(color);
    painter.drawPath(path);
    painter.restore();
}

void paintPower(QPainter& painter, const QRectF& bounds,
                const QString& base, const QString& exponent,
                const QColor& color, QFont font) {
    QFont exponentFont = font;
    exponentFont.setPixelSize(9);
    exponentFont.setWeight(QFont::Bold);

    const QFontMetricsF baseMetrics(font);
    const QFontMetricsF exponentMetrics(exponentFont);
    const QRectF baseInk = baseMetrics.tightBoundingRect(base);
    const QRectF exponentInk = exponentMetrics.tightBoundingRect(exponent);
    const QPointF baseOrigin(0.0, 0.0);
    const QPointF exponentOrigin(baseInk.right() + 1.2 - exponentInk.left(),
                                 -baseInk.height() * 0.48);
    const QRectF composite = baseInk.translated(baseOrigin).united(
        exponentInk.translated(exponentOrigin));
    const QPointF offset = bounds.center() - composite.center();

    painter.save();
    painter.setPen(color);
    painter.setFont(font);
    painter.drawText(offset + baseOrigin, base);
    painter.setFont(exponentFont);
    painter.drawText(offset + exponentOrigin, exponent);
    painter.restore();
}

void paintAngleConversion(QPainter& painter, const QRectF& bounds,
                          int tickCount, bool pointsRight,
                          const QColor& color) {
    constexpr qreal degreeDiameter = 3.8;
    constexpr qreal arrowWidth = 9.0;
    constexpr qreal arrowToUnitGap = 4.0;
    constexpr qreal degreeToPrimeGap = 1.8;
    // A prime followed by a double-prime is not three evenly spaced marks:
    // the two strokes of the double-prime form a visibly tighter pair.
    constexpr qreal singleToDoubleGap = 3.7;
    constexpr qreal doublePrimeStep = 2.4;
    const qreal tickSpan = tickCount == 1
                               ? 1.0
                               : singleToDoubleGap + doublePrimeStep + 1.0;
    const qreal totalWidth = degreeDiameter + degreeToPrimeGap + tickSpan +
                             arrowToUnitGap + arrowWidth;
    const qreal left = bounds.center().x() - totalWidth / 2.0;
    const qreal midY = bounds.center().y() + 1.0;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(color, 1.55, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    // The physical faceplate reads literally as `°′←/→` and `°′″←/→`:
    // the unit group precedes the arrow. Keep this independent of any text
    // renderer's bidirectional treatment of arrow characters.
    const qreal degreeLeft = left;
    painter.drawEllipse(QRectF(degreeLeft, midY - 5.0,
                               degreeDiameter, degreeDiameter));

    const qreal firstPrime = degreeLeft + degreeDiameter + degreeToPrimeGap;
    const auto drawPrime = [&](qreal x) {
        painter.drawLine(QPointF(x, midY - 4.8),
                         QPointF(x - 0.8, midY - 1.5));
    };
    drawPrime(firstPrime);
    if (tickCount == 3) {
        drawPrime(firstPrime + singleToDoubleGap);
        drawPrime(firstPrime + singleToDoubleGap + doublePrimeStep);
    }

    const qreal start = left + degreeDiameter + degreeToPrimeGap +
                        tickSpan + arrowToUnitGap;
    const qreal end = start + arrowWidth;
    painter.drawLine(QPointF(start, midY), QPointF(end, midY));
    const qreal tip = pointsRight ? end : start;
    const qreal inner = pointsRight ? end - 3.0 : start + 3.0;
    painter.drawLine(QPointF(tip, midY), QPointF(inner, midY - 2.7));
    painter.drawLine(QPointF(tip, midY), QPointF(inner, midY + 2.7));
    painter.restore();
}

void paintLogic(QPainter& painter, const QRectF& bounds,
                PanelSymbol symbol, const QColor& color) {
    const QPointF center = bounds.center() + QPointF(0.0, 0.5);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(color, 1.75, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    if (symbol == PanelSymbol::LogicalXor) {
        painter.drawEllipse(center, 4.7, 4.7);
        painter.drawLine(center + QPointF(-2.7, 0.0),
                         center + QPointF(2.7, 0.0));
        painter.drawLine(center + QPointF(0.0, -2.7),
                         center + QPointF(0.0, 2.7));
    } else {
        QPainterPath wedge;
        const qreal direction = symbol == PanelSymbol::LogicalAnd ? -1.0 : 1.0;
        wedge.moveTo(center + QPointF(-5.2, -direction * 3.6));
        wedge.lineTo(center + QPointF(0.0, direction * 3.6));
        wedge.lineTo(center + QPointF(5.2, -direction * 3.6));
        painter.drawPath(wedge);
    }
    painter.restore();
}

} // namespace

void paintPanelSymbol(QPainter& painter, const QRectF& bounds,
                      PanelSymbol symbol, const QColor& color,
                      const QFont& textFont,
                      const QString& sourceSymbolFamily) {
    switch (symbol) {
    case PanelSymbol::SquareRoot:
        paintSourceGlyph(painter, bounds, QStringLiteral(")"), color,
                         sourceSymbolFamily);
        break;
    case PanelSymbol::EPowerX:
        paintSourceGlyph(painter, bounds, QStringLiteral("%$"), color,
                         sourceSymbolFamily);
        break;
    case PanelSymbol::XPowerY:
        paintSourceGlyph(painter, bounds, QStringLiteral("(&"), color,
                         sourceSymbolFamily);
        break;
    case PanelSymbol::TenPowerX:
        paintSourceGlyph(painter, bounds, QStringLiteral("!#$"), color,
                         sourceSymbolFamily);
        break;
    case PanelSymbol::Random:
        paintSourceGlyph(painter, bounds, QStringLiteral("0"), color,
                         sourceSymbolFamily);
        break;
    case PanelSymbol::SinInverse:
        paintPower(painter, bounds, QStringLiteral("sin"),
                   QStringLiteral("−1"), color, textFont);
        break;
    case PanelSymbol::CosInverse:
        paintPower(painter, bounds, QStringLiteral("cos"),
                   QStringLiteral("−1"), color, textFont);
        break;
    case PanelSymbol::TgInverse:
        paintPower(painter, bounds, QStringLiteral("tg"),
                   QStringLiteral("−1"), color, textFont);
        break;
    case PanelSymbol::XSquared:
        paintPower(painter, bounds, QStringLiteral("x"),
                   QStringLiteral("2"), color, textFont);
        break;
    case PanelSymbol::DegreesMinutesFrom:
        paintAngleConversion(painter, bounds, 1, false, color);
        break;
    case PanelSymbol::DegreesMinutesTo:
        paintAngleConversion(painter, bounds, 1, true, color);
        break;
    case PanelSymbol::DegreesMinutesSecondsFrom:
        // The physical legend is degree + prime + double-prime, hence three
        // separate pad-printed ticks rather than a generic quote glyph.
        paintAngleConversion(painter, bounds, 3, false, color);
        break;
    case PanelSymbol::DegreesMinutesSecondsTo:
        paintAngleConversion(painter, bounds, 3, true, color);
        break;
    case PanelSymbol::LogicalAnd:
    case PanelSymbol::LogicalOr:
    case PanelSymbol::LogicalXor:
        paintLogic(painter, bounds, symbol, color);
        break;
    }
}

} // namespace pmk::ui
