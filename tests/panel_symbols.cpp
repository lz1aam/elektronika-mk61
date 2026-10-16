#include "panel_symbols.hpp"

#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QIcon>
#include <QPainter>

#include <array>
#include <iostream>

int main(int argc, char** argv) {
    QApplication application(argc, argv);
    const QIcon applicationIcon(
        QStringLiteral(":/icons/elektronika-mk61.svg"));
    if (applicationIcon.isNull() || applicationIcon.pixmap(64, 64).isNull()) {
        std::cerr << "cannot load application icon\n";
        return 4;
    }
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/DejaVuSansCondensed.ttf"));
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/DejaVuSansCondensed-Bold.ttf"));
    const int symbolId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/missing-symbols.ttf"));
    const QStringList families = QFontDatabase::applicationFontFamilies(symbolId);
    if (families.isEmpty()) {
        std::cerr << "cannot load original panel-symbol outlines\n";
        return 1;
    }

    constexpr std::array<pmk::ui::PanelSymbol, 16> symbols = {{
        pmk::ui::PanelSymbol::SquareRoot,
        pmk::ui::PanelSymbol::SinInverse,
        pmk::ui::PanelSymbol::CosInverse,
        pmk::ui::PanelSymbol::TgInverse,
        pmk::ui::PanelSymbol::EPowerX,
        pmk::ui::PanelSymbol::XSquared,
        pmk::ui::PanelSymbol::XPowerY,
        pmk::ui::PanelSymbol::TenPowerX,
        pmk::ui::PanelSymbol::Random,
        pmk::ui::PanelSymbol::DegreesMinutesFrom,
        pmk::ui::PanelSymbol::DegreesMinutesTo,
        pmk::ui::PanelSymbol::DegreesMinutesSecondsFrom,
        pmk::ui::PanelSymbol::DegreesMinutesSecondsTo,
        pmk::ui::PanelSymbol::LogicalAnd,
        pmk::ui::PanelSymbol::LogicalOr,
        pmk::ui::PanelSymbol::LogicalXor,
    }};

    constexpr int cellWidth = 48;
    constexpr int cellHeight = 28;
    QImage image(cellWidth * 8, cellHeight * 2, QImage::Format_RGB32);
    image.fill(QColor(64, 64, 64));
    QPainter painter(&image);
    QFont textFont(QStringLiteral("DejaVu Sans Condensed"));
    textFont.setPixelSize(13);
    textFont.setWeight(QFont::DemiBold);
    for (unsigned index = 0; index < symbols.size(); ++index) {
        const QRectF cell((index % 8) * cellWidth, (index / 8) * cellHeight,
                          cellWidth, cellHeight);
        pmk::ui::paintPanelSymbol(painter, cell, symbols[index],
                                  QColor(247, 239, 49), textFont,
                                  families.constFirst());
    }
    painter.end();

    const auto brightPixels = [&image](const QRect& area) {
        int bright = 0;
        for (int y = area.top(); y <= area.bottom(); ++y)
            for (int x = area.left(); x <= area.right(); ++x) {
                const QColor color = image.pixelColor(x, y);
                bright += color.red() > 180 && color.green() > 170;
            }
        return bright;
    };
    for (unsigned index = 0; index < symbols.size(); ++index) {
        const QRect area((index % 8) * cellWidth, (index / 8) * cellHeight,
                         cellWidth, cellHeight);
        const int bright = brightPixels(area);
        if (bright < 12) {
            std::cerr << "panel symbol produced insufficient visible ink at index "
                      << index << '\n';
            return 2;
        }
    }

    // These are deliberate technical pictograms, not whatever similarly
    // named glyph happens to exist in the host font.  Verify the defining
    // parts of the three logic marks: broad upper/lower arms and the ring,
    // horizontal stroke and vertical stroke of circled-plus XOR.
    const QRect andCell(5 * cellWidth, cellHeight, cellWidth, cellHeight);
    const QRect orCell(6 * cellWidth, cellHeight, cellWidth, cellHeight);
    const QRect xorCell(7 * cellWidth, cellHeight, cellWidth, cellHeight);
    if (brightPixels(QRect(andCell.center().x() - 2, andCell.top() + 8, 5, 5)) < 2 ||
        brightPixels(QRect(orCell.center().x() - 2, orCell.bottom() - 12, 5, 5)) < 2 ||
        brightPixels(QRect(xorCell.center().x() - 4, xorCell.center().y() - 1, 9, 3)) < 8 ||
        brightPixels(QRect(xorCell.center().x() - 1, xorCell.center().y() - 4, 3, 9)) < 8) {
        std::cerr << "logical panel pictogram geometry regressed\n";
        return 3;
    }

    // °′″ contains one prime followed by a double prime: the physical
    // legend has three distinct short strokes.  Guard against reducing it
    // back to a generic double-quote/two-stroke approximation.
    const QRect dmsFromCell(3 * cellWidth, cellHeight, cellWidth, cellHeight);
    const QRect dmsToCell(4 * cellWidth, cellHeight, cellWidth, cellHeight);
    const auto hasThreePrimePositions = [&brightPixels](const QRect& cell) {
        // At native resolution the two strokes of the double-prime may touch
        // through antialiasing, so test their combined ink instead of
        // demanding artificial blank pixel columns between them.
        return brightPixels(QRect(cell.left() + 15,
                                  cell.top() + 8, 12, 7)) >= 12;
    };
    if (!hasThreePrimePositions(dmsFromCell) ||
        !hasThreePrimePositions(dmsToCell)) {
        std::cerr << "degrees/minutes/seconds legend lost one of its three ticks\n";
        return 4;
    }

    // The physical printing is the degree/prime group followed by its arrow.
    // A shaft must cross the baseline in the right half of both DMS cells.
    if (brightPixels(QRect(dmsFromCell.left() + 27,
                           dmsFromCell.center().y(), 12, 3)) < 7 ||
        brightPixels(QRect(dmsToCell.left() + 27,
                           dmsToCell.center().y(), 12, 3)) < 7) {
        std::cerr << "angle-conversion arrow is no longer after its unit group\n";
        return 5;
    }

    if (argc > 1 && !image.save(QString::fromLocal8Bit(argv[1]))) {
        std::cerr << "cannot save panel-symbol reference render\n";
        return 6;
    }
    return 0;
}
