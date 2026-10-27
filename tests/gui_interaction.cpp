#include "calculator_widget.hpp"

#include <QApplication>
#include <QFocusEvent>
#include <QTest>

class GuiInteractionTest final : public QObject {
    Q_OBJECT

private slots:
    void stackViewWhilePoweredOff() {
        CalculatorWidget widget;
        widget.resize(480, 800);
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        const QSize compactSize = widget.size();
        const auto angle = widget.angleMode();
        widget.setStackDisplayExpanded(true);
        const QSize expandedSize = widget.size();
        QTest::keyClick(&widget, Qt::Key_F12);
        QVERIFY(!widget.isPowered());

        for (const bool expanded : {false, true, false}) {
            QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {240, 48});
            QCOMPARE(widget.isStackDisplayExpanded(), expanded);
            QCOMPARE(widget.size(), expanded ? expandedSize : compactSize);
            QVERIFY(!widget.isPowered());
            QVERIFY(!widget.hasActiveKeyContact());
            QCOMPARE(widget.angleMode(), angle);
            const QImage display = widget.grab(
                QRect(0, 0, 480, expanded ? 288 : 96)).toImage();
            QVERIFY(!display.isNull());
            int luminousPixels = 0;
            for (int y = 0; y < display.height(); ++y)
                for (int x = 0; x < display.width(); ++x) {
                    const QColor colour = display.pixelColor(x, y);
                    luminousPixels += colour.green() > 110 && colour.blue() > 110 &&
                                      colour.green() > colour.red() * 1.4;
                }
            QCOMPARE(luminousPixels, 0);
        }
        QTest::keyClick(&widget, Qt::Key_F12);
        QVERIFY(widget.isPowered());
        QCOMPARE(widget.size(), compactSize);
    }

    void contactOwnership() {
        CalculatorWidget widget;
        widget.show();
        QTest::keyPress(&widget, Qt::Key_1);
        QVERIFY(widget.hasActiveKeyContact());
        QTest::keyRelease(&widget, Qt::Key_Shift);
        QVERIFY(widget.hasActiveKeyContact());
        QTest::mouseRelease(&widget, Qt::RightButton, Qt::NoModifier, {48, 641});
        QVERIFY(widget.hasActiveKeyContact());
        QTest::keyRelease(&widget, Qt::Key_1);
        QVERIFY(!widget.hasActiveKeyContact());
        QTest::mousePress(&widget, Qt::RightButton, Qt::NoModifier, {48, 641});
        QVERIFY(!widget.hasActiveKeyContact());
        QTest::mouseRelease(&widget, Qt::RightButton, Qt::NoModifier, {48, 641});
    }

    void powerAndAngleControls() {
        CalculatorWidget widget;
        widget.resize(480, 800);
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));

        QVERIFY(widget.isPowered());
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {80, 175});
        QVERIFY(!widget.isPowered());
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {400, 169});
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Degrees);
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {370, 169});
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Grads);
        const QImage modelMark =
            widget.grab(QRect(315, 105, 105, 32)).toImage();
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {360, 120});
        QVERIFY(widget.grab(QRect(315, 105, 105, 32)).toImage() == modelMark);
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {80, 175});
        QVERIFY(widget.isPowered());
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Grads);

        // These are the same three mechanical centers used by the painter.
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {370, 169});
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Grads);
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {400, 169});
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Degrees);
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {340, 169});
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Radians);
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {356, 169});
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Grads);
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {386, 169});
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Degrees);
        QTest::keyClick(&widget, Qt::Key_R);
        QCOMPARE(widget.angleMode(), pmk::AngleMode::Radians);

        const QImage panel = widget.grab().toImage();
        const auto brightPixels = [&panel](const QRect& area) {
            int count = 0;
            for (int y = area.top(); y <= area.bottom(); ++y)
                for (int x = area.left(); x <= area.right(); ++x) {
                    const QColor colour = panel.pixelColor(x, y);
                    count += colour.red() > 180 && colour.green() > 180 &&
                             colour.blue() > 180;
                }
            return count;
        };
        QVERIFY2(brightPixels(QRect(20, 108, 200, 22)) > 100,
                 "maker name is missing from its independent anchor");
        QVERIFY2(brightPixels(QRect(315, 108, 105, 22)) > 30,
                 "model mark is missing from its independent anchor");
        QCOMPARE(brightPixels(QRect(220, 108, 90, 22)), 0);
        QVERIFY2(brightPixels(QRect(96, 151, 56, 28)) > 18,
                 "power-switch pad printing became too light");
        QVERIFY2(brightPixels(QRect(294, 151, 30, 28)) > 4,
                 "radian pad printing became too light");
        QVERIFY2(brightPixels(QRect(346, 137, 48, 22)) > 16,
                 "gradian pad printing became too light");
        QVERIFY2(brightPixels(QRect(416, 151, 28, 28)) > 4,
                 "degree pad printing became too light");
        for (int col = 1; col < 5; ++col) {
            const QRect registerArea(col * 96 + 30, 776, 36, 22);
            QVERIFY2(brightPixels(registerArea) > 10,
                     "bottom register pad printing became too light");
        }

        const auto meanRed = [&panel](const QRect& area) {
            qint64 sum = 0;
            for (int y = area.top(); y <= area.bottom(); ++y)
                for (int x = area.left(); x <= area.right(); ++x)
                    sum += panel.pixelColor(x, y).red();
            return double(sum) / (area.width() * area.height());
        };
        const double powerRed = meanRed(QRect(63, 163, 26, 12));
        const double angleRed = meanRed(QRect(327, 163, 26, 12));
        QVERIFY2(angleRed > powerRed + 20.0,
                 "power and angle actuators lost their distinct materials");

        const int compactHeight = widget.height();
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {240, 48});
        QVERIFY(widget.isStackDisplayExpanded());
        QVERIFY(widget.height() > compactHeight);
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, {240, 48});
        QVERIFY(!widget.isStackDisplayExpanded());
        QCOMPARE(widget.height(), compactHeight);

        QTest::mousePress(&widget, Qt::LeftButton, Qt::NoModifier, {48, 641});
        QVERIFY(widget.hasActiveKeyContact());
        QTest::mouseRelease(&widget, Qt::LeftButton, Qt::NoModifier, {48, 641});
        QVERIFY(!widget.hasActiveKeyContact());

        QTest::mousePress(&widget, Qt::LeftButton, Qt::NoModifier, {48, 641});
        const QImage pressedKey = widget.grab(QRect(12, 626, 72, 49)).toImage();
        QFocusEvent focusOut(QEvent::FocusOut, Qt::OtherFocusReason);
        QApplication::sendEvent(&widget, &focusOut);
        const QImage releasedKey = widget.grab(QRect(12, 626, 72, 49)).toImage();
        QVERIFY2(pressedKey != releasedKey,
                 "focus loss left a key visually depressed");

        widget.setFocus();
        QTest::keyClick(&widget, Qt::Key_Escape);
        QTest::qWait(100);
        QTest::keyClick(&widget, Qt::Key_F12);
        QVERIFY(!widget.isPowered());
        QTest::keyClick(&widget, Qt::Key_F12);
        QVERIFY(widget.isPowered());
        const auto luminousPixels = [&widget] {
            const QImage display =
                widget.grab(QRect(0, 0, 480, 96)).toImage();
            int count = 0;
            for (int y = 0; y < display.height(); ++y)
                for (int x = 0; x < display.width(); ++x) {
                    const QColor colour = display.pixelColor(x, y);
                    count += colour.green() > 110 && colour.blue() > 110 &&
                             colour.green() > colour.red() * 1.4;
                }
            return count;
        };
        QTRY_VERIFY_WITH_TIMEOUT(luminousPixels() > 20, 2000);
    }
};

QTEST_MAIN(GuiInteractionTest)
#include "gui_interaction.moc"
