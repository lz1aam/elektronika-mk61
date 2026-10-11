#include "calculator_widget.hpp"
#include "panel_symbols.hpp"
#include "program_text.hpp"
#include "vfd_display.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace {

constexpr qreal KeyboardTop = 224.0;
constexpr qreal KeyCellWidth = 96.0;
constexpr qreal KeyCellHeight = 92.0;

QFont panelFont(int pixelSize, QFont::Weight weight = QFont::Normal,
                int stretch = QFont::Condensed) {
    QFont font(QStringLiteral("DejaVu Sans Condensed"));
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    font.setStretch(stretch);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    return font;
}

enum class LetteringRole {
    Brand,
    Switch,
    Function,
    KeyCap,
    Register
};

QFont letteringFont(LetteringRole role) {
    switch (role) {
    case LetteringRole::Brand: {
        QFont font = panelFont(17, QFont::DemiBold, 100);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 0.68);
        return font;
    }
    case LetteringRole::Switch:
        // All case legends are pad-printed. Medium at this size leaves
        // one-pixel stems that are not credible for the physical process.
        return panelFont(12, QFont::DemiBold, 100);
    case LetteringRole::Function:
        return panelFont(13, QFont::DemiBold, 100);
    case LetteringRole::KeyCap:
        return panelFont(18, QFont::DemiBold, 96);
    case LetteringRole::Register:
        // The register letters are smaller than the function row, but use
        // the same robust ink weight rather than a thin caption style.  The
        // larger em size compensates for the low x-height of lowercase a-d.
        return panelFont(15, QFont::Bold, 100);
    }
    return panelFont(13);
}

// QWidget::drawText centres the font's ascent/descent box.  That produces
// visibly different baselines for digits, Cyrillic, arrows and the upstream
// special-symbol face.  Centre the actual painted bounds instead, while still
// using drawText so small lettering retains the font's native hinting.
void drawInkAlignedText(QPainter& painter, const QRectF& bounds,
                        Qt::Alignment alignment, const QString& text) {
    if (text.isEmpty())
        return;
    const QRectF ink = QFontMetricsF(painter.font()).tightBoundingRect(text);
    qreal x = bounds.center().x() - ink.center().x();
    if (alignment.testFlag(Qt::AlignLeft))
        x = bounds.left() - ink.left();
    else if (alignment.testFlag(Qt::AlignRight))
        x = bounds.right() - ink.right();
    qreal baseline = bounds.center().y() - ink.center().y();
    if (alignment.testFlag(Qt::AlignTop))
        baseline = bounds.top() - ink.top();
    else if (alignment.testFlag(Qt::AlignBottom))
        baseline = bounds.bottom() - ink.bottom();
    painter.drawText(QPointF(x, baseline), text);
}

std::optional<pmk::ui::PanelSymbol> upperPanelSymbol(
    pmk::PhysicalKey key) {
    using Symbol = pmk::ui::PanelSymbol;
    switch (key) {
    case pmk::PhysicalKey::Subtract:   return Symbol::SquareRoot;
    case pmk::PhysicalKey::Digit4:     return Symbol::SinInverse;
    case pmk::PhysicalKey::Digit5:     return Symbol::CosInverse;
    case pmk::PhysicalKey::Digit6:     return Symbol::TgInverse;
    case pmk::PhysicalKey::Digit1:     return Symbol::EPowerX;
    case pmk::PhysicalKey::Multiply:   return Symbol::XSquared;
    case pmk::PhysicalKey::ExchangeXY: return Symbol::XPowerY;
    case pmk::PhysicalKey::Digit0:     return Symbol::TenPowerX;
    case pmk::PhysicalKey::Decimal:    return Symbol::Random;
    default:                           return std::nullopt;
    }
}

std::optional<pmk::ui::PanelSymbol> lowerPanelSymbol(
    pmk::PhysicalKey key) {
    using Symbol = pmk::ui::PanelSymbol;
    switch (key) {
    case pmk::PhysicalKey::Digit6:     return Symbol::DegreesMinutesFrom;
    case pmk::PhysicalKey::Add:        return Symbol::DegreesMinutesTo;
    case pmk::PhysicalKey::Digit3:     return Symbol::DegreesMinutesSecondsFrom;
    case pmk::PhysicalKey::ExchangeXY: return Symbol::DegreesMinutesSecondsTo;
    case pmk::PhysicalKey::Decimal:    return Symbol::LogicalAnd;
    case pmk::PhysicalKey::ChangeSign: return Symbol::LogicalOr;
    case pmk::PhysicalKey::Exponent:   return Symbol::LogicalXor;
    default:                           return std::nullopt;
    }
}

QRectF keyCapRect(int index) {
    const int row = index / 5;
    const int column = index % 5;
    return QRectF(column * KeyCellWidth + 15.0,
                  KeyboardTop + row * KeyCellHeight + 31.0,
                  66.0, 42.0);
}

enum class SlideMaterial { Graphite, BrownPhenolic };

struct SlideSwitchSpec {
    QRectF channel;
    QRectF hitArea;
    qreal thumbWidth;
    qreal thumbHeight;
    int positionCount;
    SlideMaterial material;
};

const SlideSwitchSpec& powerSwitchSpec() {
    static const SlideSwitchSpec spec{
        QRectF(32, 160, 58, 18), QRectF(10, 143, 145, 62),
        28, 14, 2, SlideMaterial::Graphite
    };
    return spec;
}

const SlideSwitchSpec& angleSwitchSpec() {
    static const SlideSwitchSpec spec{
        QRectF(326, 160, 88, 18), QRectF(296, 138, 146, 66),
        28, 14, 3, SlideMaterial::BrownPhenolic
    };
    return spec;
}

qreal slideCenter(const SlideSwitchSpec& spec, int position) {
    const qreal inset = spec.thumbWidth / 2.0;
    const qreal travel = spec.channel.width() - 2.0 * inset;
    return spec.channel.left() + inset +
        (spec.positionCount == 1 ? 0.0
         : travel * position / (spec.positionCount - 1));
}

int nearestSlidePosition(const SlideSwitchSpec& spec, const QPointF& point) {
    if (!spec.hitArea.contains(point))
        return -1;
    int nearest = 0;
    qreal nearestDistance = std::abs(point.x() - slideCenter(spec, 0));
    for (int position = 1; position < spec.positionCount; ++position) {
        const qreal distance = std::abs(point.x() - slideCenter(spec, position));
        if (distance < nearestDistance) {
            nearest = position;
            nearestDistance = distance;
        }
    }
    return nearest;
}

void drawPlasticKey(QPainter& painter, QRectF bounds, const QColor& base,
                    bool blackKey, bool pressed) {
    if (pressed)
        bounds.translate(0.0, 2.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(5, 6, 7, pressed ? 125 : 190));
    painter.drawRoundedRect(bounds.translated(1.4, pressed ? 1.0 : 2.6),
                            3.2, 3.2);

    QLinearGradient edge(bounds.topLeft(), bounds.bottomLeft());
    if (blackKey) {
        edge.setColorAt(0.0, pressed ? QColor(25, 26, 27)
                                     : QColor(62, 64, 65));
        edge.setColorAt(0.22, pressed ? QColor(13, 14, 15)
                                      : QColor(31, 32, 33));
        edge.setColorAt(1.0, QColor(5, 6, 7));
    } else {
        edge.setColorAt(0.0, pressed ? base.darker(125) : base.lighter(119));
        edge.setColorAt(0.24, pressed ? base.darker(112) : base.lighter(105));
        edge.setColorAt(1.0, base.darker(142));
    }
    painter.setPen(QPen(blackKey ? QColor(7, 8, 9) : QColor(28, 29, 29), 1.0));
    painter.setBrush(edge);
    painter.drawRoundedRect(bounds, 3.0, 3.0);

    const QRectF face = bounds.adjusted(1.6, 1.4, -1.6, -2.5);
    QLinearGradient faceGradient(face.topLeft(), face.bottomLeft());
    if (blackKey) {
        faceGradient.setColorAt(0.0, pressed ? QColor(12, 13, 14)
                                             : QColor(27, 28, 29));
        faceGradient.setColorAt(1.0, QColor(5, 6, 7));
    } else {
        faceGradient.setColorAt(0.0, pressed ? base.darker(113)
                                             : base.lighter(108));
        faceGradient.setColorAt(1.0, pressed ? base.darker(127)
                                             : base.darker(108));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(faceGradient);
    painter.drawRoundedRect(face, 2.2, 2.2);

    painter.setPen(QPen(blackKey ? QColor(115, 118, 119, 65)
                                 : QColor(255, 255, 248, 95), 0.7));
    painter.drawLine(QPointF(face.left() + 3.0, face.top() + 1.0),
                     QPointF(face.right() - 3.0, face.top() + 1.0));
}

void drawSlideSwitch(QPainter& painter, const SlideSwitchSpec& spec,
                     int position) {
    const QRectF& track = spec.channel;
    // The recessed opening is slightly taller than the actuator footprint,
    // leaving the visible clearance found in the frontal hardware references.
    painter.setPen(QPen(QColor(8, 9, 9), 1.0));
    QLinearGradient recess(track.topLeft(), track.bottomLeft());
    recess.setColorAt(0.0, QColor(48, 48, 46));
    recess.setColorAt(0.22, QColor(8, 9, 9));
    recess.setColorAt(0.78, QColor(13, 14, 14));
    recess.setColorAt(1.0, QColor(72, 72, 68));
    painter.setBrush(recess);
    painter.drawRoundedRect(track, 2.0, 2.0);

    painter.setPen(QPen(QColor(103, 104, 99, 50), 0.65));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(track.adjusted(1.2, 1.2, -1.2, -1.2), 1.4, 1.4);

    const qreal centerX = slideCenter(spec, position);
    const QRectF thumb(centerX - spec.thumbWidth / 2.0,
                       track.center().y() - spec.thumbHeight / 2.0,
                       spec.thumbWidth, spec.thumbHeight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.drawRoundedRect(thumb.translated(1.4, 2.0), 1.5, 1.5);

    QLinearGradient plastic(thumb.topLeft(), thumb.bottomRight());
    if (spec.material == SlideMaterial::Graphite) {
        plastic.setColorAt(0.0, QColor(62, 65, 63));
        plastic.setColorAt(0.28, QColor(31, 33, 32));
        plastic.setColorAt(1.0, QColor(8, 9, 9));
    } else {
        plastic.setColorAt(0.0, QColor(137, 111, 72));
        plastic.setColorAt(0.28, QColor(91, 72, 46));
        plastic.setColorAt(1.0, QColor(43, 34, 24));
    }
    painter.setPen(QPen(spec.material == SlideMaterial::Graphite
                            ? QColor(5, 6, 6) : QColor(39, 30, 20), 1.0));
    painter.setBrush(plastic);
    painter.drawRoundedRect(thumb, 1.2, 1.2);
    painter.setPen(QPen(spec.material == SlideMaterial::Graphite
                            ? QColor(145, 146, 139, 90)
                            : QColor(198, 164, 108, 105), 0.7));
    painter.drawLine(QPointF(thumb.left() + 3.0, thumb.top() + 2.0),
                     QPointF(thumb.right() - 3.0, thumb.top() + 2.0));
    for (qreal dx : {-4.0, 0.0, 4.0}) {
        painter.setPen(QPen(QColor(5, 6, 6, 150), 0.65));
        painter.drawLine(QPointF(thumb.center().x() + dx, thumb.top() + 5.0),
                         QPointF(thumb.center().x() + dx, thumb.bottom() - 4.0));
        painter.setPen(QPen(spec.material == SlideMaterial::Graphite
                                ? QColor(112, 113, 108, 50)
                                : QColor(175, 142, 91, 65), 0.45));
        painter.drawLine(QPointF(thumb.center().x() + dx + 1.0, thumb.top() + 5.0),
                         QPointF(thumb.center().x() + dx + 1.0, thumb.bottom() - 4.0));
    }
}

} // namespace

const std::array<CalculatorWidget::Key, 30> CalculatorWidget::Keys = {{
    {pmk::PhysicalKey::F,"F","","",2}, {pmk::PhysicalKey::StepForward,"ШГ→","x<0","",0},
    {pmk::PhysicalKey::StepBack,"ШГ←","x=0","",0}, {pmk::PhysicalKey::Return,"В/О","x≥0","",0}, {pmk::PhysicalKey::RunStop,"С/П","x≠0","",0},
    {pmk::PhysicalKey::K,"K","","",3}, {pmk::PhysicalKey::Recall,"П→X","L0","",0},
    {pmk::PhysicalKey::Store,"X→П","L1","",0}, {pmk::PhysicalKey::Branch,"БП","L2","",0}, {pmk::PhysicalKey::Subroutine,"ПП","L3","",0},
    {pmk::PhysicalKey::Digit7,"7","sin","[x]",1}, {pmk::PhysicalKey::Digit8,"8","cos","{x}",1}, {pmk::PhysicalKey::Digit9,"9","tg","max",1},
    {pmk::PhysicalKey::Subtract,"−","√","",1}, {pmk::PhysicalKey::Divide,"÷","1/x","",1},
    {pmk::PhysicalKey::Digit4,"4","sin⁻¹","|x|",1}, {pmk::PhysicalKey::Digit5,"5","cos⁻¹","ЗН",1}, {pmk::PhysicalKey::Digit6,"6","tg⁻¹","°′←",1},
    {pmk::PhysicalKey::Add,"+","π","°′→",1}, {pmk::PhysicalKey::Multiply,"×","x²","",1},
    {pmk::PhysicalKey::Digit1,"1","eˣ","",1}, {pmk::PhysicalKey::Digit2,"2","lg","",1}, {pmk::PhysicalKey::Digit3,"3","ln","°′″←",1},
    {pmk::PhysicalKey::ExchangeXY,"↔","xʸ","°′″→",1}, {pmk::PhysicalKey::Enter,"В↑","Вx","СЧ",1},
    {pmk::PhysicalKey::Digit0,"0","10ˣ","НОП",1}, {pmk::PhysicalKey::Decimal,"•","○","∧",1}, {pmk::PhysicalKey::ChangeSign,"/−/","АВТ","∨",1},
    {pmk::PhysicalKey::Exponent,"ВП","ПРГ","⊕",1}, {pmk::PhysicalKey::Clear,"CX","CF","ИНВ",4}
}};

CalculatorWidget::CalculatorWidget(QWidget* parent) : QWidget(parent) {
    setWindowTitle(QStringLiteral("Электроника МК 61"));
    setMinimumSize(360, 600);
    resize(LogicalWidth, LogicalHeight);
    setFocusPolicy(Qt::StrongFocus);

    QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/DejaVuSansCondensed.ttf"));
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/DejaVuSansCondensed-Bold.ttf"));

    const int symbolFontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/missing-symbols.ttf"));
    const QStringList symbolFamilies =
        QFontDatabase::applicationFontFamilies(symbolFontId);
    if (!symbolFamilies.isEmpty())
        symbolFontFamily_ = symbolFamilies.constFirst();

    timer_ = new QTimer(this);
    timer_->setTimerType(Qt::PreciseTimer);
    emulationClock_.start();
    connect(timer_, &QTimer::timeout, this, [this] {
        constexpr qint64 NanosecondsPerSecond = 1'000'000'000;
        constexpr qint64 EmulatedTicksPerSecond = 168'000;
        constexpr qint64 MaximumCatchUpNanoseconds = 100'000'000;
        const qint64 elapsed = std::clamp(
            emulationClock_.nsecsElapsed(), qint64(0),
            MaximumCatchUpNanoseconds);
        emulationClock_.restart();
        if (!powered_)
            return;

        // Advance from monotonic elapsed time rather than assuming that every
        // GUI timeout arrived exactly on schedule. A bounded 100 ms catch-up
        // preserves normal timing without creating a resume-after-suspend
        // storm. Fractional ticks carry into the next callback.
        tickRemainder_ += elapsed * EmulatedTicksPerSecond;
        const auto ticks = static_cast<std::size_t>(
            tickRemainder_ / NanosecondsPerSecond);
        tickRemainder_ %= NanosecondsPerSecond;
        core_.advanceTicks(ticks);

        // Painting the complete panel at the former 125 Hz was needlessly
        // expensive; cap VFD presentation near 31 Hz.
        if (displayStateChanged())
            displayRefreshFrames_ = 3;
        repaintPhase_ = !repaintPhase_;
        if (repaintPhase_ && displayRefreshFrames_ != 0) {
            updateDisplay();
            --displayRefreshFrames_;
        }
    });
    timer_->start(16);
}

bool CalculatorWidget::displayStateChanged() {
    bool changed = !displayStateInitialized_;
    const auto& live = core_.displaySegments();
    if (live != lastDisplayCodes_) {
        lastDisplayCodes_ = live;
        changed = true;
    }
    if (stackDisplayExpanded_) {
        const auto& stack = core_.stackDisplaySnapshots();
        for (std::size_t row = 0; row < stack.size(); ++row) {
            std::array<std::uint8_t, 12> codes{};
            for (std::size_t cell = 0; cell < codes.size(); ++cell)
                codes[cell] = stack[row].cells[cell].code;
            if (codes != lastStackCodes_[row]) {
                lastStackCodes_[row] = codes;
                changed = true;
            }
        }
    }
    displayStateInitialized_ = true;
    return changed;
}

void CalculatorWidget::updateDisplay() {
    const qreal scale = std::min(width() / qreal(LogicalWidth),
                                 height() / qreal(logicalHeight()));
    const QPointF origin((width() - LogicalWidth * scale) / 2.0,
                         (height() - logicalHeight() * scale) / 2.0);
    const int displayHeight = stackDisplayExpanded_
        ? ExpandedDisplayHeight : DisplayHeight;
    update(QRectF(origin.x(), origin.y(), LogicalWidth * scale,
                  displayHeight * scale).toAlignedRect());
}

QPointF CalculatorWidget::toLogical(const QPointF& position) const {
    const qreal scale = std::min(width() / qreal(LogicalWidth),
                                 height() / qreal(logicalHeight()));
    const QPointF origin((width() - LogicalWidth * scale) / 2.0,
                         (height() - logicalHeight() * scale) / 2.0);
    return (position - origin) / scale;
}

void CalculatorWidget::toggleStackDisplay() {
    setStackDisplayExpanded(!stackDisplayExpanded_);
}

void CalculatorWidget::setStackDisplayExpanded(bool expanded) {
    if (!powered_ || stackDisplayExpanded_ == expanded)
        return;
    if (expanded) {
        compactSize_ = size();
        const qreal scale = std::min(width() / qreal(LogicalWidth),
                                     height() / qreal(LogicalHeight));
        stackDisplayExpanded_ = true;
        resize(width(), height() + qRound(panelOffset() * scale));
    } else {
        stackDisplayExpanded_ = false;
        if (compactSize_.isValid())
            resize(compactSize_);
    }
    updateGeometry();
    update();
}

int CalculatorWidget::keyAt(const QPointF& p) const {
    if (p.x() < 0 || p.y() < KeyboardTop)
        return -1;
    const int column = int(p.x() / KeyCellWidth);
    const int row = int((p.y() - KeyboardTop) / KeyCellHeight);
    if (column < 0 || column >= 5 || row < 0 || row >= 6)
        return -1;
    const int index = row * 5 + column;
    return keyCapRect(index).adjusted(-3.0, -3.0, 3.0, 3.0).contains(p)
        ? index : -1;
}

bool CalculatorWidget::selectAngleModeAt(const QPointF& p) {
    const int position = nearestSlidePosition(angleSwitchSpec(), p);
    if (position < 0)
        return false;
    if (position == 0)
        angleMode_ = pmk::AngleMode::Radians;
    else if (position == 1)
        angleMode_ = pmk::AngleMode::Grads;
    else
        angleMode_ = pmk::AngleMode::Degrees;
    core_.setAngleMode(angleMode_);
    update();
    return true;
}

bool CalculatorWidget::selectModelAt(const QPointF& p) {
    if (powered_ || !QRectF(6, 101, 260, 40).contains(p))
        return false;
    model_ = model_ == pmk::CalculatorModel::MK61
           ? pmk::CalculatorModel::MK54 : pmk::CalculatorModel::MK61;
    core_.setModel(model_);
    update();
    return true;
}

void CalculatorWidget::setPowered(bool powered) {
    if (powered_ == powered)
        return;
    powered_ = powered;
    pressed_ = -1;
    fIndicator_ = false;
    kIndicator_ = false;
    // A power transition changes phosphor excitation even when the decoded
    // character codes before and after reset happen to be identical.
    displayStateInitialized_ = false;
    displayRefreshFrames_ = 3;
    tickRemainder_ = 0;
    emulationClock_.restart();
    if (powered_) {
        core_.reset();
        core_.setAngleMode(angleMode_);
    }
    update();
}

bool CalculatorWidget::togglePowerAt(const QPointF& p) {
    if (!powerSwitchSpec().hitArea.contains(p))
        return false;
    setPowered(!powered_);
    return true;
}

bool CalculatorWidget::showProgramMenuAt(const QPointF& p,
                                         const QPoint& globalPosition) {
    if (!QRectF(98, 143, 62, 48).contains(p))
        return false;
    QMenu menu(this);
    QAction* load = menu.addAction(tr("Load text program…"));
    QAction* save = menu.addAction(tr("Save text program…"));
    const bool available = powered_ && model_ == pmk::CalculatorModel::MK61;
    load->setEnabled(available);
    save->setEnabled(available);
    QAction* selected = menu.exec(globalPosition);
    if (selected == load) loadTextProgram();
    if (selected == save) saveTextProgram();
    return true;
}

void CalculatorWidget::loadTextProgram() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load PMK text program"), {}, tr("PMK text programs (*.txt);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("PMK program"), file.errorString());
        return;
    }
    try {
        const QByteArray contents = file.readAll();
        const auto parsed = pmk::importProgramText(
            std::string_view(contents.constData(), std::size_t(contents.size())));
        core_.importProgram(parsed.bytes);
        update();
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("PMK program"),
                             QString::fromUtf8(error.what()));
    }
}

void CalculatorWidget::saveTextProgram() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save PMK text program"), QStringLiteral("program.txt"),
        tr("PMK text programs (*.txt);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("PMK program"), file.errorString());
        return;
    }
    try {
        const std::string text = pmk::exportProgramText(core_.exportProgram());
        if (file.write(text.data(), static_cast<qint64>(text.size())) !=
            static_cast<qint64>(text.size()))
            QMessageBox::warning(this, tr("PMK program"), file.errorString());
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("PMK program"),
                             QString::fromUtf8(error.what()));
    }
}

void CalculatorWidget::pressKeyIndex(int index) {
    if (!powered_ || index < 0 || index >= int(Keys.size()))
        return;
    pressed_ = index;
    const auto physicalKey = Keys[index].physicalKey;
    const bool wasF = fIndicator_;
    const bool wasK = kIndicator_;
    fIndicator_ = physicalKey == pmk::PhysicalKey::F;
    kIndicator_ = physicalKey == pmk::PhysicalKey::K;
    if (wasF && physicalKey == pmk::PhysicalKey::F)
        fIndicator_ = true;
    if (wasK && physicalKey == pmk::PhysicalKey::K)
        kIndicator_ = true;
    const auto matrix = pmk::matrixKey(Keys[index].physicalKey);
    core_.pressMatrixKey(matrix.row, matrix.column);
    update();
}

void CalculatorWidget::releaseCurrentKey() {
    core_.releaseKey();
    pressed_ = -1;
    update();
}

void CalculatorWidget::mousePressEvent(QMouseEvent* event) {
    QPointF logical = toLogical(event->position());
    const int activeDisplayHeight = stackDisplayExpanded_
        ? ExpandedDisplayHeight : DisplayHeight;
    if (powered_ && QRectF(0, 0, LogicalWidth, activeDisplayHeight)
                        .contains(logical)) {
        toggleStackDisplay();
        event->accept();
        return;
    }
    logical.ry() -= panelOffset();
    if (showProgramMenuAt(logical, event->globalPosition().toPoint()))
        return;
    if (togglePowerAt(logical))
        return;
    if (selectModelAt(logical))
        return;
    // The mechanical angle selector remains movable without electrical power.
    if (!selectAngleModeAt(logical) && powered_)
        pressKeyIndex(keyAt(logical));
}

void CalculatorWidget::mouseReleaseEvent(QMouseEvent*) {
    releaseCurrentKey();
}

void CalculatorWidget::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat())
        return;
    if (event->key() == Qt::Key_F12) {
        setPowered(!powered_);
        event->accept();
        return;
    }
    if (!powered_) {
        QWidget::keyPressEvent(event);
        return;
    }
    std::optional<pmk::PhysicalKey> physicalKey;
    if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9)
        physicalKey = static_cast<pmk::PhysicalKey>(
            static_cast<int>(pmk::PhysicalKey::Digit0) +
            event->key() - Qt::Key_0);
    else {
        switch (event->key()) {
        case Qt::Key_Comma:
        case Qt::Key_Period: physicalKey = pmk::PhysicalKey::Decimal; break;
        case Qt::Key_Return:
        case Qt::Key_Enter: physicalKey = pmk::PhysicalKey::Enter; break;
        case Qt::Key_Plus: physicalKey = pmk::PhysicalKey::Add; break;
        case Qt::Key_Minus: physicalKey = pmk::PhysicalKey::Subtract; break;
        case Qt::Key_Asterisk: physicalKey = pmk::PhysicalKey::Multiply; break;
        case Qt::Key_Slash: physicalKey = pmk::PhysicalKey::Divide; break;
        case Qt::Key_Backspace:
        case Qt::Key_Escape: physicalKey = pmk::PhysicalKey::Clear; break;
        case Qt::Key_F: physicalKey = pmk::PhysicalKey::F; break;
        case Qt::Key_K: physicalKey = pmk::PhysicalKey::K; break;
        case Qt::Key_R:
            angleMode_ = pmk::AngleMode::Radians;
            core_.setAngleMode(angleMode_);
            update();
            event->accept();
            return;
        case Qt::Key_D:
            angleMode_ = pmk::AngleMode::Degrees;
            core_.setAngleMode(angleMode_);
            update();
            event->accept();
            return;
        case Qt::Key_G:
            angleMode_ = pmk::AngleMode::Grads;
            core_.setAngleMode(angleMode_);
            update();
            event->accept();
            return;
        default: break;
        }
    }
    if (physicalKey) {
        for (int i = 0; i < int(Keys.size()); ++i)
            if (Keys[i].physicalKey == *physicalKey) {
                pressKeyIndex(i);
                event->accept();
                return;
            }
    }
    QWidget::keyPressEvent(event);
}

void CalculatorWidget::keyReleaseEvent(QKeyEvent* event) {
    if (!event->isAutoRepeat() && pressed_ >= 0) {
        releaseCurrentKey();
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void CalculatorWidget::focusOutEvent(QFocusEvent* event) {
    if (pressed_ >= 0)
        releaseCurrentKey();
    QWidget::focusOutEvent(event);
}

void CalculatorWidget::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const qreal scale = std::min(width() / qreal(LogicalWidth),
                                 height() / qreal(logicalHeight()));
    const QPointF origin((width() - LogicalWidth * scale) / 2.0,
                         (height() - logicalHeight() * scale) / 2.0);
    const int activeDisplayHeight = stackDisplayExpanded_
        ? ExpandedDisplayHeight : DisplayHeight;
    const QRect displayRect = QRectF(
        origin.x(), origin.y(), LogicalWidth * scale,
        activeDisplayHeight * scale).toAlignedRect();
    const bool displayOnly = displayRect.contains(event->region().boundingRect());
    p.translate(origin);
    p.scale(scale, scale);

    // PMK-Android values/colors.xml: commonBackground and indicatorOff.
    p.fillRect(QRectF(0, 0, LogicalWidth, logicalHeight()), QColor("#404040"));
    if (!stackDisplayExpanded_) {
        pmk::ui::paintVfdDisplay(p, QRectF(0, 1, LogicalWidth, DisplayHeight),
                                 core_.displaySnapshot(), powered_,
                                 fIndicator_, kIndicator_);
    } else {
        // Draw the RPN stack in its physical push direction: X is the bottom
        // (top-of-stack) entry, while ENTER lifts values toward T above it.
        constexpr std::array<const char*, 4> labels = {{"T", "Z", "Y", "X"}};
        const auto& stack = core_.stackDisplaySnapshots();
        const qreal rowHeight = ExpandedDisplayHeight / 4.0;
        // One continuous tinted cover belongs in front of all four virtual
        // tubes. Drawing a glass gradient per row creates false black seams.
        pmk::ui::paintVfdGlass(
            p, QRectF(0, 0, LogicalWidth, ExpandedDisplayHeight), powered_);
        for (unsigned row = 0; row < stack.size(); ++row) {
            const unsigned stackLevel = stack.size() - 1U - row;
            const QRectF rowBounds(22.0, row * rowHeight,
                                   LogicalWidth - 22.0, rowHeight);
            pmk::ui::paintVfdElectrodes(
                p, rowBounds, stack[stackLevel], powered_,
                stackLevel == 0 && fIndicator_,
                stackLevel == 0 && kIndicator_);
            pmk::ui::paintVfdElectrodeLabel(
                p, QRectF(0, row * rowHeight, 22, rowHeight),
                labels[row][0], powered_);
        }
    }

    if (displayOnly)
        return;

    p.translate(0, panelOffset());

    // The maker's name and model are two independently positioned pieces of
    // case printing on the real panel.  Do not use whitespace to position the
    // model: its location would then change with font metrics and fallback.
    p.setPen(QColor(238, 240, 235));
    p.setFont(letteringFont(LetteringRole::Brand));
    drawInkAlignedText(p, QRectF(24, 105, 260, 32),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QStringLiteral("ЭЛЕКТРОНИКА"));
    drawInkAlignedText(p, QRectF(315, 105, 105, 32),
                       Qt::AlignCenter,
                       model_ == pmk::CalculatorModel::MK61
                           ? QStringLiteral("МК 61")
                           : QStringLiteral("МК-54"));

    // The real machine uses compact raised slide switches. The deliberately
    // larger invisible hit regions remain convenient for mouse/touch input.
    p.setFont(letteringFont(LetteringRole::Switch));
    p.setPen(QColor(236, 238, 232));
    drawInkAlignedText(p, QRectF(98, 152, 52, 28),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QStringLiteral("ВКЛ"));
    drawSlideSwitch(p, powerSwitchSpec(), powered_ ? 1 : 0);

    p.setPen(QColor(236, 238, 232));
    drawInkAlignedText(p, QRectF(296, 153, 26, 26),
                       Qt::AlignCenter, QStringLiteral("Р"));
    drawInkAlignedText(p, QRectF(348, 138, 44, 20),
                       Qt::AlignCenter, QStringLiteral("ГРД"));
    drawInkAlignedText(p, QRectF(418, 153, 24, 26),
                       Qt::AlignCenter, QStringLiteral("Г"));
    const int activeMode = angleMode_ == pmk::AngleMode::Radians ? 0
                         : angleMode_ == pmk::AngleMode::Grads ? 1 : 2;
    drawSlideSwitch(p, angleSwitchSpec(), activeMode);

    const QColor face[] = {
        QColor(15, 16, 17), QColor(195, 197, 191), QColor(229, 222, 67),
        QColor(48, 190, 211), QColor(218, 32, 43)
    };
    for (int i = 0; i < int(Keys.size()); ++i) {
        const int row = i / 5;
        const int col = i % 5;
        const QRectF r = keyCapRect(i);
        drawPlasticKey(p, r, face[Keys[i].color], Keys[i].color == 0,
                       i == pressed_);

        const QFont functionFont = letteringFont(LetteringRole::Function);
        p.setFont(functionFont);
        p.setPen(QColor(247, 239, 49));
        const QRectF labelRect(col * KeyCellWidth + 5,
                               KeyboardTop + row * KeyCellHeight + 3,
                               KeyCellWidth - 10, 22);
        const bool showBlue = model_ == pmk::CalculatorModel::MK61 &&
                              Keys[i].lower[0] != '\0';
        const QString upperLabel = QString::fromUtf8(Keys[i].upper);
        const auto upperSymbol = upperPanelSymbol(Keys[i].physicalKey);
        const auto lowerSymbol = lowerPanelSymbol(Keys[i].physicalKey);
        const auto paintLegend = [&](const QRectF& area, const QString& text,
                                     const QColor& color,
                                     const std::optional<pmk::ui::PanelSymbol>& symbol) {
            if (symbol) {
                pmk::ui::paintPanelSymbol(p, area, *symbol, color,
                                          functionFont, symbolFontFamily_);
            } else {
                p.setFont(functionFont);
                p.setPen(color);
                drawInkAlignedText(p, area, Qt::AlignCenter, text);
            }
        };

        if (!showBlue) {
            paintLegend(labelRect, upperLabel, QColor(247, 239, 49),
                        upperSymbol);
        } else {
            const QRectF upperArea(labelRect.left(), labelRect.top(),
                                   labelRect.width() / 2, labelRect.height());
            const QRectF lowerArea(labelRect.center().x(), labelRect.top(),
                                   labelRect.width() / 2, labelRect.height());
            paintLegend(upperArea, upperLabel, QColor(247, 239, 49),
                        upperSymbol);
            paintLegend(lowerArea, QString::fromUtf8(Keys[i].lower),
                        QColor(83, 203, 229), lowerSymbol);
        }

        const auto physicalKey = Keys[i].physicalKey;
        QFont buttonFont = letteringFont(LetteringRole::KeyCap);
        QString mainLabel = QString::fromUtf8(Keys[i].main);
        p.setFont(buttonFont);
        p.setPen(Keys[i].color == 1 || Keys[i].color == 2
                     ? QColor(13, 14, 14) : QColor(235, 237, 234));
        if (model_ == pmk::CalculatorModel::MK54 &&
            Keys[i].physicalKey == pmk::PhysicalKey::Clear)
            mainLabel = QStringLiteral("Cx");
        if (model_ == pmk::CalculatorModel::MK54 &&
            Keys[i].physicalKey == pmk::PhysicalKey::ExchangeXY)
        {
            mainLabel = QStringLiteral("XY");
            p.setFont(buttonFont);
        }
        QRectF textRect = r;
        if (i == pressed_)
            textRect.translate(0.0, 2.0);
        drawInkAlignedText(p, textRect, Qt::AlignCenter, mainLabel);
    }

    p.setPen(QColor(235, 237, 231));
    p.setFont(letteringFont(LetteringRole::Register));
    for (int col = 1; col < 5; ++col) {
        drawInkAlignedText(p, QRectF(col * KeyCellWidth, 776, KeyCellWidth, 20),
                           Qt::AlignCenter,
                           QString(QChar('a' + col - 1)));
    }
    if (model_ == pmk::CalculatorModel::MK54)
        drawInkAlignedText(p, QRectF(0, 776, KeyCellWidth, 20),
                           Qt::AlignCenter, QStringLiteral("НОП"));
}

