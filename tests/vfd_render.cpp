#include "vfd_display.hpp"
#include "calculator_core.hpp"
#include "keymap.hpp"
#include "vfd_reference.hpp"

#include <QApplication>
#include <QImage>
#include <QPainter>

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
// Interior electrode locations in our 480x96 render, not geometry helpers.
constexpr std::array<QPoint, 7> renderedCentres = {{
    {22, 24}, {30, 36}, {26, 60}, {15, 72}, {7, 60}, {11, 36}, {18, 48}
}};

bool referenceLit(unsigned code, unsigned electrode) {
    return ManualGlyphElectrodes[code].find(char('A' + electrode)) !=
           std::string_view::npos;
}

bool renderedLit(const QImage& image, unsigned slot, unsigned electrode) {
    const QPoint centre = renderedCentres[electrode] + QPoint(slot * 40, 0);
    // A small interior patch tolerates mesh lines and rasterizer variation.
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            if (image.pixelColor(centre + QPoint(x, y)).green() > 170)
                return true;
    return false;
}

bool verifyAlphabet() {
    for (unsigned code = 0; code < 16; ++code) {
        std::array<std::uint8_t, 12> codes;
        codes.fill(15);
        codes[pmk::hardware::VfdTube::electricalCell(0)] = code;
        QImage image(480, 96, QImage::Format_RGB32);
        image.fill(Qt::black);
        QPainter painter(&image);
        pmk::ui::paintVfdDisplay(painter, image.rect(), codes, true, false, false);
        painter.end();
        pmk::hardware::VfdTube scannedTube;
        scannedTube.acceptScan(true, pmk::hardware::VfdTube::electricalCell(0) + 2,
                               code, false);
        const auto& scannedCell = scannedTube.snapshot().cells[
            pmk::hardware::VfdTube::electricalCell(0)];
        for (unsigned electrode = 0; electrode < 7; ++electrode) {
            const bool expected = referenceLit(code, electrode);
            const bool decoded = (pmk::hardware::decodedVfdSegments(code) &
                                  (1U << electrode)) != 0;
            const bool excited = scannedCell.phosphor[electrode] != 0;
            if (decoded != expected || excited != expected ||
                renderedLit(image, 0, electrode) != expected) {
                std::cerr << "Manual reference mismatch: code " << code
                          << ", electrode " << char('A' + electrode) << '\n';
                return false;
            }
        }
    }
    return true;
}
} // namespace

int main(int argc, char** argv) {
    QApplication application(argc, argv);

    if (!verifyAlphabet())
        return 8;

    std::array<std::uint8_t, 12> display;
    display.fill(15);
    constexpr std::array<int, 12> order =
        {{8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 10, 9}};
    const std::array<std::uint8_t, 12> fullTest =
        {{10, 8 | 0x80, 8, 8, 8, 8, 8, 8, 8, 10, 8, 8}};
    for (int slot = 0; slot < int(order.size()); ++slot)
        display[order[slot]] = fullTest[slot];

    QImage image(480, 96, QImage::Format_RGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    pmk::ui::paintVfdDisplay(painter, image.rect(), display,
                             true, false, false);
    painter.end();

    int brightPixels = 0;
    int cyanPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QColor color(line[x]);
            if (color.green() > 170)
                ++brightPixels;
            if (color.green() > color.red() * 2 &&
                color.blue() > color.red() * 2)
                ++cyanPixels;
        }
    }
    if (brightPixels < 2500 || cyanPixels < 3500) {
        std::cerr << "VFD render contains too few lit cyan phosphor pixels\n";
        return 1;
    }

    // The physical ИЛЦ2-12/8Л bars are oblique, with the upper end displaced
    // to the right.  Slot 1 contains an 8, so comparing its upper and lower
    // luminous centroids makes the inclination a regression-tested property
    // rather than a subjective screenshot detail.
    const auto brightCentroidX = [&image](int left, int top, int right, int bottom) {
        qint64 xSum = 0;
        int count = 0;
        for (int y = top; y < bottom; ++y) {
            const auto* line =
                reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = left; x < right; ++x) {
                if (QColor(line[x]).green() > 170) {
                    xSum += x;
                    ++count;
                }
            }
        }
        return count == 0 ? 0.0 : double(xSum) / count;
    };
    const double upperCentroid = brightCentroidX(40, 18, 80, 38);
    const double lowerCentroid = brightCentroidX(40, 59, 80, 79);
    if (upperCentroid - lowerCentroid < 3.0) {
        std::cerr << "VFD segment geometry lost its physical rightward lean\n";
        return 3;
    }

    if (argc > 1 && !image.save(QString::fromLocal8Bit(argv[1]))) {
        std::cerr << "cannot save VFD reference render\n";
        return 2;
    }

    // The supplied manual calls the error signal EDD0D; the hardware study
    // identifies the stored constant as EDD0DFFF.  D is the calculator's
    // three-segment Cyrillic-looking Г and F is blank, so the five visible
    // tube cells must read EГГ0Г with no decimal points.
    // Exercise the unmodified CPU/ROM, not a string replacement or mock error.
    pmk::CalculatorCore core;
    core.advanceTicks(250000);
    const auto key = [&core](pmk::PhysicalKey physical) {
        const auto matrix = pmk::matrixKey(physical);
        core.pressMatrixKey(matrix.row, matrix.column);
        core.advanceTicks(50000);
        core.releaseKey();
        core.advanceTicks(250000);
    };
    key(pmk::PhysicalKey::Digit1);
    key(pmk::PhysicalKey::Enter);
    key(pmk::PhysicalKey::Digit0);
    key(pmk::PhysicalKey::Divide);
    // Error blinks. Wait for a complete lit frame, bounded in emulated ticks.
    constexpr std::array<std::uint8_t, 12> errorCodes =
        {{15, 14, 13, 13, 0, 13, 15, 15, 15, 15, 15, 15}};
    bool foundError = false;
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        foundError = true;
        for (unsigned slot = 0; slot < 12; ++slot)
            foundError &= core.displaySegments()[order[slot]] == errorCodes[slot];
        if (foundError)
            break;
        core.advanceTicks(5000);
    }
    if (!foundError) {
        std::cerr << "Real divide-by-zero did not emit the documented error frame\n";
        return 9;
    }
    const auto errorDisplay = core.displaySegments();

    QImage errorImage(480, 96, QImage::Format_RGB32);
    errorImage.fill(Qt::black);
    QPainter errorPainter(&errorImage);
    pmk::ui::paintVfdDisplay(errorPainter, errorImage.rect(), errorDisplay,
                             true, false, false);
    errorPainter.end();

    for (int slot = 0; slot < 12; ++slot) {
        for (unsigned electrode = 0; electrode < 7; ++electrode) {
            if (renderedLit(errorImage, slot, electrode) !=
                referenceLit(errorCodes[slot], electrode)) {
                std::cerr << "Real error render differs from original manual\n";
                return 10;
            }
        }
        int lit = 0;
        const int left = slot * 40;
        for (int y = 0; y < errorImage.height(); ++y) {
            const auto* line =
                reinterpret_cast<const QRgb*>(errorImage.constScanLine(y));
            for (int x = left; x < left + 40; ++x)
                if (QColor(line[x]).green() > 170)
                    ++lit;
        }
        if ((slot >= 1 && slot <= 5 && lit < 20) ||
            ((slot == 0 || slot > 5) && lit != 0)) {
            std::cerr << "EDD0DFFF error fixture has incorrect lit cells\n";
            return 4;
        }
    }
    if (argc > 2 && !errorImage.save(QString::fromLocal8Bit(argv[2]))) {
        std::cerr << "cannot save VFD error reference render\n";
        return 5;
    }

    // Auxiliary labels are electrode assemblies too, not cover-glass text.
    // Verify all requested glyphs light through the common renderer and that
    // removing power leaves no luminous pixels.
    constexpr std::array<char, 6> labelGlyphs = {{'T', 'Z', 'Y', 'X', 'F', 'K'}};
    const auto renderLabels = [&labelGlyphs](bool powered) {
        QImage labels(180, 72, QImage::Format_RGB32);
        labels.fill(Qt::black);
        QPainter labelPainter(&labels);
        pmk::ui::paintVfdGlass(labelPainter, labels.rect(), powered);
        for (unsigned i = 0; i < labelGlyphs.size(); ++i)
            pmk::ui::paintVfdElectrodeLabel(
                labelPainter, QRectF(i * 30, 0, 30, 72),
                labelGlyphs[i], powered);
        labelPainter.end();
        return labels;
    };
    const QImage litLabels = renderLabels(true);
    const QImage darkLabels = renderLabels(false);
    for (unsigned glyph = 0; glyph < labelGlyphs.size(); ++glyph) {
        int litPixels = 0;
        int darkPixels = 0;
        for (int y = 0; y < litLabels.height(); ++y) {
            for (int x = int(glyph) * 30; x < int(glyph + 1) * 30; ++x) {
                litPixels += QColor(litLabels.pixel(x, y)).green() > 170;
                darkPixels += QColor(darkLabels.pixel(x, y)).green() > 170;
            }
        }
        if (litPixels < 10 || darkPixels != 0) {
            std::cerr << "VFD auxiliary electrode glyph power behavior failed\n";
            return 6;
        }
    }
    if (argc > 3 && !litLabels.save(QString::fromLocal8Bit(argv[3]))) {
        std::cerr << "cannot save VFD auxiliary-label reference render\n";
        return 7;
    }
    return 0;
}
