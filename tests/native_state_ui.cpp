#include "calculator_core.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <QTest>

#define private public
#include "calculator_widget.hpp"
#undef private

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir directory;
        if (!directory.isValid()) return EXIT_FAILURE;
        CalculatorWidget widget;
        widget.show();
        widget.timer_->stop();
        widget.core_.advanceTicks(250000);
        widget.core_.setAngleMode(pmk::AngleMode::Degrees);
        widget.core_.pressMatrixKey(1, 7);
        widget.core_.advanceTicks(50000);
        widget.core_.releaseKey();
        widget.core_.advanceTicks(250000);
        widget.setStackDisplayExpanded(true);
        const auto path = directory.filePath("calculator.mk61state");
        widget.saveNativeStateFile(path);
        widget.setPowered(false);
        const auto poweredOffState = widget.core_.saveState();
        widget.setStackDisplayExpanded(false);
        if (widget.isPowered() || widget.isStackDisplayExpanded() ||
            widget.core_.saveState() != poweredOffState)
            throw std::runtime_error("collapsing while off changed power or hardware state");
        widget.setStackDisplayExpanded(true);
        if (widget.isPowered() || !widget.isStackDisplayExpanded() ||
            widget.core_.saveState() != poweredOffState)
            throw std::runtime_error("expanding while off changed power or hardware state");
        widget.loadNativeStateFile(path);
        if (!widget.isPowered() || !widget.isStackDisplayExpanded() ||
            widget.angleMode() != pmk::AngleMode::Degrees ||
            widget.hasActiveKeyContact() ||
            !widget.core_.displayText().starts_with(" 7."))
            return EXIT_FAILURE;
        const auto before = widget.core_.saveState();
        QFile invalid(directory.filePath("invalid.mk61state"));
        if (!invalid.open(QIODevice::WriteOnly) || invalid.write("invalid") != 7)
            return EXIT_FAILURE;
        invalid.close();
        bool rejected = false;
        try { widget.loadNativeStateFile(invalid.fileName()); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected || widget.core_.saveState() != before)
            return EXIT_FAILURE;

        // API can store a held contact. Desktop restoration must release it.
        widget.core_.pressMatrixKey(2, 0);
        widget.saveNativeStateFile(path);
        widget.loadNativeStateFile(path);
        if (widget.hasActiveKeyContact()) return EXIT_FAILURE;

        // Failure to replace a destination directory must leave it intact.
        bool writeFailed = false;
        try { widget.saveNativeStateFile(directory.path()); }
        catch (const std::runtime_error&) { writeFailed = true; }
        if (!writeFailed || !directory.isValid()) return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
