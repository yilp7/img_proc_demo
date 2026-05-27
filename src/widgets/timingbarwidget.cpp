#include "timingbarwidget.h"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

namespace {
const int kRowSpacing   = 10;  // vertical gap between rows
const int kHandleHitPx  = 8;   // hit-zone each side of the end edge
const int kLeftMargin   = 6;
const int kRightMargin  = 96;  // generous right pad so the track's max
                               // position is well inside the panel border
const int kTopMargin    = 6;
const int kBottomMargin = 6;
const int kLabelAreaPx  = 72;  // reserved left strip — label (line 1) + value (line 2)
const int kLabelGap     = 8;   // space between label strip and track
const int kTrackHeight  = 10;  // visual track height (thin, prototype-like)
const int kEndCapWidth  = 3;   // accent bar drawn at the fill's right edge
}

TimingBarWidget::TimingBarWidget(QWidget *parent) :
    QWidget(parent),
    m_drag_bar(-1)
{
    qRegisterMetaType<TimingBarWidget::BarId>("TimingBarWidget::BarId");

    setObjectName("TimingBarWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(320, 140); // 4 bars × (label + value + thin track) rows

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
    // External setter: do NOT emit barValueChanged. That signal is reserved
    // for user-driven changes (mouse press/move handlers). Emitting here
    // would loop when the controller drives the bar from its own state
    // update — e.g. drag → barValueChanged → controller setter →
    // *_display_updated → setBarValue → barValueChanged again.
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
    if (m_bars[id].value > maxValue) m_bars[id].value = maxValue;
    // Scale change is configuration, not a user-driven value change.
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
    return kLeftMargin + kLabelAreaPx + kLabelGap;
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
    // Theme tokens — match prototype v3 and resources/style_dark.qss.
    static const QColor kBg        (0x12, 0x10, 0x0E);  // bg-raised
    static const QColor kTrackBg   (0x04, 0x03, 0x03);  // bg-deepest
    static const QColor kTrackEdge (0x38, 0x2D, 0x25);  // border-subtle
    static const QColor kLabel     (0x8F, 0x6B, 0x4D);  // text-dim
    static const QColor kValue     (0xCD, 0xB2, 0x99);  // readout

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), kBg);

    const int tw = trackWidth();
    const int rh = rowHeight();
    if (tw <= 0 || rh <= 0) return;

    const QFont base_font = p.font();
    QFont value_font = base_font;
    if (value_font.pointSize() > 0) {
        value_font.setPointSizeF(qMax(7.0, value_font.pointSizeF() - 1));
    }
    const QFontMetrics fm_label = QFontMetrics(base_font);
    const QFontMetrics fm_value = QFontMetrics(value_font);
    const int track_h = qMin(kTrackHeight, rh);

    for (int i = 0; i < BarCount; ++i) {
        const int top = rowTop(i);
        const int track_y = top + (rh - track_h) / 2; // vertically center the thin track
        const Bar &b = m_bars[i];

        // ---- Left text strip: label (line 1) + value+unit (line 2) ----
        const QString value_text = b.unit.isEmpty()
            ? QString::number(b.value)
            : QStringLiteral("%1 %2").arg(b.value).arg(b.unit);

        const int line_h_label = fm_label.height();
        const int line_h_value = fm_value.height();
        const int stack_h = line_h_label + line_h_value;
        const int stack_top = top + (rh - stack_h) / 2;

        const QRect label_rect(kLeftMargin, stack_top, kLabelAreaPx, line_h_label);
        const QRect value_rect(kLeftMargin, stack_top + line_h_label,
                               kLabelAreaPx, line_h_value);

        p.setFont(base_font);
        p.setPen(kLabel);
        p.drawText(label_rect, Qt::AlignLeft | Qt::AlignVCenter,
                   fm_label.elidedText(b.label, Qt::ElideRight, label_rect.width()));

        p.setFont(value_font);
        p.setPen(kValue);
        p.drawText(value_rect, Qt::AlignLeft | Qt::AlignVCenter,
                   fm_value.elidedText(value_text, Qt::ElideRight, value_rect.width()));
        p.setFont(base_font);

        // ---- Track + filled segment ----
        const QRectF track_rect(trackLeft(), track_y, tw, track_h);
        QPainterPath track_path;
        track_path.addRoundedRect(track_rect, track_h / 2.0, track_h / 2.0);
        p.fillPath(track_path, kTrackBg);

        const int seg_w = qMax(0, valueToPixel(b.value, tw, b.scale));
        if (seg_w > 0) {
            const QRectF seg_rect(track_rect.left(), track_y, seg_w, track_h);
            QLinearGradient g(seg_rect.topLeft(), seg_rect.topRight());
            g.setColorAt(0.0, b.color.darker(220));
            g.setColorAt(1.0, b.color);
            QPainterPath seg_path;
            seg_path.addRoundedRect(seg_rect, track_h / 2.0, track_h / 2.0);
            p.save();
            p.setClipPath(track_path);
            p.fillPath(seg_path, g);
            p.restore();
        }

        // Track outline.
        p.setPen(kTrackEdge);
        p.setBrush(Qt::NoBrush);
        p.drawPath(track_path);

        // ---- Right-end cap: always visible accent stripe at the fill's
        //      right edge, including when value == scale. Clipped to the
        //      track path so the rounded right corner stays clean.
        if (seg_w > 0) {
            double cap_left  = track_rect.left() + seg_w - kEndCapWidth;
            double cap_right = track_rect.left() + seg_w;
            // Keep the cap inside the track even at max — shift it left when
            // it would otherwise overflow the rounded right corner.
            if (cap_right > track_rect.right()) {
                cap_left  -= (cap_right - track_rect.right());
                cap_right  = track_rect.right();
            }
            const QRectF cap_rect(cap_left, track_y, cap_right - cap_left, track_h);
            p.save();
            p.setClipPath(track_path);
            p.fillRect(cap_rect, b.color.lighter(140));
            p.restore();
        }
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
    }
    QWidget::mouseMoveEvent(event);
}

void TimingBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) m_drag_bar = -1;
    QWidget::mouseReleaseEvent(event);
}
