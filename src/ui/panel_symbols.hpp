#pragma once

#include <QColor>
#include <QFont>
#include <QRectF>
#include <QString>

class QPainter;

namespace pmk::ui {

enum class PanelSymbol {
    SquareRoot,
    SinInverse,
    CosInverse,
    TgInverse,
    EPowerX,
    XSquared,
    XPowerY,
    TenPowerX,
    Random,
    DegreesMinutesFrom,
    DegreesMinutesTo,
    DegreesMinutesSecondsFrom,
    DegreesMinutesSecondsTo,
    LogicalAnd,
    LogicalOr,
    LogicalXor
};

// Paints the MK-61's technical panel pictograms as one coherent system.
// `textFont` supplies the ordinary bold lettering used inside composites;
// `sourceSymbolFamily` supplies PMK-Android's original custom outlines for
// the few symbols it actually contains.
void paintPanelSymbol(QPainter& painter, const QRectF& bounds,
                      PanelSymbol symbol, const QColor& color,
                      const QFont& textFont,
                      const QString& sourceSymbolFamily);

} // namespace pmk::ui
