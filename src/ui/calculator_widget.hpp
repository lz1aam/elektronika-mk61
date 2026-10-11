#pragma once

#include "calculator_core.hpp"
#include "keymap.hpp"

#include <QWidget>
#include <QString>
#include <QSize>

#include <array>

class QTimer;
class QPainter;

class CalculatorWidget final : public QWidget {
public:
    explicit CalculatorWidget(QWidget* parent = nullptr);
    bool isPowered() const { return powered_; }
    pmk::AngleMode angleMode() const { return angleMode_; }
    pmk::CalculatorModel model() const { return model_; }
    bool hasPendingKeyEvent() const { return core_.keyPending(); }
    bool isStackDisplayExpanded() const { return stackDisplayExpanded_; }
    void setStackDisplayExpanded(bool expanded);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private:
    struct Key {
        pmk::PhysicalKey physicalKey;
        const char* main;
        const char* upper;
        const char* lower;
        int color;
    };

    // The checked-in Fastlane phone references are 480 x 800 pixels.
    static constexpr int LogicalWidth = 480;
    static constexpr int LogicalHeight = 800;
    static constexpr int DisplayHeight = 96;
    static constexpr int ExpandedDisplayHeight = 288;
    static const std::array<Key, 30> Keys;

    int keyAt(const QPointF& logical) const;
    bool selectAngleModeAt(const QPointF& logical);
    bool selectModelAt(const QPointF& logical);
    bool togglePowerAt(const QPointF& logical);
    bool showProgramMenuAt(const QPointF& logical, const QPoint& globalPosition);
    void loadTextProgram();
    void saveTextProgram();
    void setPowered(bool powered);
    void pressKeyIndex(int index);
    void releaseCurrentKey();
    void toggleStackDisplay();
    QPointF toLogical(const QPointF& position) const;
    int panelOffset() const {
        return stackDisplayExpanded_ ? ExpandedDisplayHeight - DisplayHeight : 0;
    }
    int logicalHeight() const { return LogicalHeight + panelOffset(); }

    pmk::CalculatorCore core_;
    QString symbolFontFamily_;
    pmk::AngleMode angleMode_{pmk::AngleMode::Radians};
    pmk::CalculatorModel model_{pmk::CalculatorModel::MK61};
    bool powered_{true};
    bool fIndicator_{};
    bool kIndicator_{};
    bool stackDisplayExpanded_{};
    QSize compactSize_;
    QTimer* timer_{};
    int pressed_{-1};
};

