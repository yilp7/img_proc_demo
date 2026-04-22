#include "timingbarwidget.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace {
const int kRowSpacing   = 6;
const int kHandleHitPx  = 8;   // hit-zone each side of the end edge
const int kLeftMargin   = 10;
const int kRightMargin  = 10;
const int kTopMargin    = 6;
const int kBottomMargin = 6;
const int kLabelAreaPx  = 110; // reserved left strip for "Label: value unit"
}

TimingBarWidget::TimingBarWidget(QWidget *parent) :
    QWidget(parent),
    m_drag_bar(-1)
{
    qRegisterMetaType<TimingBarWidget::BarId>("TimingBarWidget::BarId");

    setObjectName("TimingBarWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMouseTracking(true);
    setMinimumSize(360, 140);

    m_bars[LaserWidth] = { 100,    1000,    QStringLiteral("Laser Width"), QStringLiteral("ns"), QColor(0x3d, 0x8b, 0xff) };
    m_bars[Distance]   = { 500,    5000,    QStringLiteral("Distance"),    QStringLiteral("m"),  QColor(0x4c, 0xd9, 0x64) };
    m_bars[Dov]        = { 100,    1000,    QStringLiteral("DOV"),         QStringLiteral("m"),  QColor(0x4d, 0xd0, 0xe1) };
    m_bars[Mcp]        = { 128,     255,    QStringLiteral("MCP"),         QString(),            QColor(0xa5, 0xe3, 0x4d) };
}

void TimingBarWidget::setBarValue(BarId id, quint64 value)
{
    if (id < 0 || id >= BarCount) return;
    if (value > m_bars[id].scale) value = m_bars[id].scale;
    if (m_bars[id].value == value) return;
    m_bars[id].value = value;
    emit barValueChanged(id, value);
    update();
}

quint64 TimingBarWidget::barValue(BarId id) const
{
    if (id < 0 || id >= BarCount) return 0;
    return m_bars[id].value;
}

void TimingBarWidget::setBarScale(BarId id, quint64 maxValue)
{
    if (id < 0 || id >= BarCount) return;
    if (maxValue == 0) return;
    if (m_bars[id].scale == maxValue) return;
    m_bars[id].scale = maxValue;
    if (m_bars[id].value > maxValue) {
        m_bars[id].value = maxValue;
        emit barValueChanged(id, maxValue);
    }
    update();
}

quint64 TimingBarWidget::barScale(BarId id) const
{
    if (id < 0 || id >= BarCount) return 0;
    return m_bars[id].scale;
}

void TimingBarWidget::setBarLabel(BarId id, const QString &label)
{
    if (id < 0 || id >= BarCount) return;
    m_bars[id].label = label;
    update();
}

void TimingBarWidget::setBarUnit(BarId id, const QString &unit)
{
    if (id < 0 || id >= BarCount) return;
    m_bars[id].unit = unit;
    update();
}

void TimingBarWidget::setBarColor(BarId id, const QColor &color)
{
    if (id < 0 || id >= BarCount) return;
    m_bars[id].color = color;
    update();
}

quint64 TimingBarWidget::pixelToValue(int px, int widthPx, quint64 scaleMax)
{
    if (widthPx <= 0 || scaleMax == 0) return 0;
    if (px <= 0) return 0;
    if (px >= widthPx) return scaleMax;
    return static_cast<quint64>(px) * scaleMax / static_cast<quint64>(widthPx);
}

int TimingBarWidget::valueToPixel(quint64 value, int widthPx, quint64 scaleMax)
{
    if (widthPx <= 0 || scaleMax == 0) return 0;
    if (value >= scaleMax) return widthPx;
    return static_cast<int>(value * static_cast<quint64>(widthPx) / scaleMax);
}

int TimingBarWidget::rowHeight() const
{
    const int usable = height() - kTopMargin - kBottomMargin;
    return (usable - (BarCount - 1) * kRowSpacing) / BarCount;
}

int TimingBarWidget::trackLeft() const
{
    return kLeftMargin + kLabelAreaPx;
}

int TimingBarWidget::trackWidth() const
{
    return qMax(0, width() - trackLeft() - kRightMargin);
}

int TimingBarWidget::rowTop(int idx) const
{
    return kTopMargin + idx * (rowHeight() + kRowSpacing);
}

int TimingBarWidget::endHandleAt(const QPoint &pos) const
{
    const int tw = trackWidth();
    const int rh = rowHeight();
    if (tw <= 0 || rh <= 0) return -1;

    for (int i = 0; i < BarCount; ++i) {
        const int top = rowTop(i);
        if (pos.y() < top || pos.y() >= top + rh) continue;
        const int ex = trackLeft() + valueToPixel(m_bars[i].value, tw, m_bars[i].scale);
        if (qAbs(pos.x() - ex) <= kHandleHitPx) return i;
    }
    return -1;
}

void TimingBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), palette().window());

    const int tw = trackWidth();
    const int rh = rowHeight();
    if (tw <= 0 || rh <= 0) return;

    const QFontMetrics fm = p.fontMetrics();

    for (int i = 0; i < BarCount; ++i) {
        const int top = rowTop(i);
        const QRect label_rect(kLeftMargin, top, kLabelAreaPx - 4, rh);
        const QRect track_rect(trackLeft(), top, tw, rh);

        // Label + value readout on the left.
        const Bar &b = m_bars[i];
        QString label_text = b.unit.isEmpty()
            ? QStringLiteral("%1: %2").arg(b.label).arg(b.value)
            : QStringLiteral("%1: %2 %3").arg(b.label).arg(b.value).arg(b.unit);
        p.setPen(palette().text().color());
        p.drawText(label_rect,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   fm.elidedText(label_text, Qt::ElideRight, label_rect.width()));

        // Track background.
        p.fillRect(track_rect, QColor(0x22, 0x22, 0x22));

        // Filled bar from track-left to the current value.
        const int ex = trackLeft() + valueToPixel(b.value, tw, b.scale);
        const QRect seg_rect(track_rect.left(), top,
                             qMax(1, ex - track_rect.left()), rh);
        p.fillRect(seg_rect, b.color);

        // Track outline.
        p.setPen(QColor(0xff, 0xff, 0xff));
        p.setBrush(Qt::NoBrush);
        p.drawRect(track_rect.adjusted(0, 0, -1, -1));

        // End handle marker (small vertical tick).
        p.setPen(QPen(QColor(0xff, 0xff, 0xff), 2.0));
        p.drawLine(ex, top + 2, ex, top + rh - 2);
    }
}

void TimingBarWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    // Prefer dragging the end handle when the press is within its hit-zone.
    int bar = endHandleAt(event->pos());

    // Otherwise: if the press is inside a bar's track, jump the value to the
    // clicked x and let the subsequent drag refine it.
    if (bar < 0) {
        const int tw = trackWidth();
        const int rh = rowHeight();
        const int x  = event->pos().x();
        const int y  = event->pos().y();
        if (tw > 0 && rh > 0 && x >= trackLeft() && x <= trackLeft() + tw) {
            for (int i = 0; i < BarCount; ++i) {
                const int top = rowTop(i);
                if (y < top || y >= top + rh) continue;
                const quint64 new_value = pixelToValue(x - trackLeft(), tw, m_bars[i].scale);
                if (new_value != m_bars[i].value) {
                    m_bars[i].value = new_value;
                    emit barValueChanged(static_cast<BarId>(i), new_value);
                    update();
                }
                bar = i;
                break;
            }
        }
    }

    m_drag_bar = bar;
    QWidget::mousePressEvent(event);
}

void TimingBarWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_drag_bar >= 0) {
        const int tw = trackWidth();
        const int local_x = event->pos().x() - trackLeft();
        Bar &b = m_bars[m_drag_bar];
        const quint64 new_value = pixelToValue(local_x, tw, b.scale);
        if (new_value != b.value) {
            b.value = new_value;
            emit barValueChanged(static_cast<BarId>(m_drag_bar), new_value);
            update();
        }
    } else {
        setCursor(endHandleAt(event->pos()) >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
    }
    QWidget::mouseMoveEvent(event);
}

void TimingBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) m_drag_bar = -1;
    QWidget::mouseReleaseEvent(event);
}
