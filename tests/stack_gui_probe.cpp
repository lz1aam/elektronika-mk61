#include <QApplication>
#include <QTest>
#include <QThread>
#include <QTimer>

#define private public
#include "calculator_widget.hpp"
#undef private

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

static constexpr char Glyphs[] = "0123456789-LCDE ";

std::string snapshotText(const pmk::hardware::VfdSnapshot& snapshot) {
    std::string result;
    for (unsigned visible = 0; visible < 12; ++visible) {
        const auto& cell = snapshot.cells[
            pmk::hardware::VfdTube::electricalCell(visible)];
        result.push_back(Glyphs[cell.code & 0x0fU]);
        if ((cell.code & 0x80U) != 0)
            result.push_back('.');
    }
    return result;
}

void physicalKeyClick(CalculatorWidget& widget, Qt::Key key,
                      int heldMilliseconds, int releasedMilliseconds) {
    QTest::keyPress(&widget, key);
    QTest::qWait(heldMilliseconds);
    QTest::keyRelease(&widget, key);
    QTest::qWait(releasedMilliseconds);
}

void settledCoreKey(pmk::CalculatorCore& core, pmk::PhysicalKey key) {
    const auto matrix = pmk::matrixKey(key);
    core.pressMatrixKey(matrix.row, matrix.column);
    core.advanceTicks(50'000);
    core.releaseKey();
    core.advanceTicks(500'000);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    {
        CalculatorWidget widget;
        widget.show();
        QTest::qWait(300);
        // A complete contact between GUI timeouts must still advance the
        // processor while held, without buffering the input for a later tick.
        widget.timer_->stop();
        widget.advanceEmulationToNow();
        QTest::keyPress(&widget, Qt::Key_1);
        const auto pressedAt = widget.core_.tickCount();
        QThread::msleep(5);
        QTest::keyRelease(&widget, Qt::Key_1);
        if (widget.core_.tickCount() <= pressedAt ||
            widget.hasActiveKeyContact())
            return EXIT_FAILURE;
        widget.core_.reset();
        widget.emulationClock_.restart();
        widget.timer_->start(16);
        QTest::qWait(300);
        for (int value = 0; value < 4; ++value) {
            settledCoreKey(widget.core_, pmk::PhysicalKey::Digit1);
            settledCoreKey(widget.core_, pmk::PhysicalKey::Digit2);
            settledCoreKey(widget.core_, pmk::PhysicalKey::Digit3);
            if (value != 3)
                settledCoreKey(widget.core_, pmk::PhysicalKey::Enter);
        }
        widget.core_.advanceTicks(2'000'000);
        QTest::qWait(300);
        {
            const auto& initial = widget.core_.stackDisplaySnapshots();
            if (!widget.core_.displayText().starts_with(" 123."))
                return EXIT_FAILURE;
            for (std::size_t level = 1; level < initial.size(); ++level)
                if (!snapshotText(initial[level]).starts_with(" 123.")) {
                    std::cerr << "initial stack level " << level << ": ["
                              << snapshotText(initial[level]) << "]\n";
                    return EXIT_FAILURE;
                }
        }
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {240, 48});
        for (int operation = 0; operation < 14; ++operation)
            physicalKeyClick(widget, Qt::Key_Plus, 50, 50);
        QTest::qWait(1500);
        const auto& stack = widget.core_.stackDisplaySnapshots();
        const std::string result = widget.core_.displayText();
        constexpr std::array<std::string_view, 14> ValidResults = {
            " 246.", " 369.", " 492.", " 615.",
            " 738.", " 861.", " 984.", " 1107.", " 1230.",
            " 1353.", " 1476.", " 1599.", " 1722.", " 1845."
        };
        const auto accepted = std::find_if(
            ValidResults.begin(), ValidResults.end(),
            [&result](std::string_view valid) { return result.starts_with(valid); });
        if (widget.hasActiveKeyContact() || accepted == ValidResults.end()) {
            std::cerr << "rapid result: live [" << result << "] contact "
                      << widget.hasActiveKeyContact() << '\n';
            return EXIT_FAILURE;
        }
        for (std::size_t level = 1; level < stack.size(); ++level)
            if (!snapshotText(stack[level]).starts_with(" 123.")) {
                std::cerr << "stack level " << level << ": ["
                          << snapshotText(stack[level]) << "]\n";
                return EXIT_FAILURE;
            }
        QTest::qWait(1000);
        if (widget.core_.displayText() != result) {
            std::cerr << "continued after release: [" << result << "] -> ["
                      << widget.core_.displayText() << "]\n";
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
