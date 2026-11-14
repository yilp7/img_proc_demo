#include "timingbarwidget.h"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

namespace {
const int ROW_SPACING   = 10;  // vertical gap between rows
const int HANDLE_HIT_PX = 8;   // hit-zone each side of the end edge
const int LEFT_MARGIN   = 6;
const int RIGHT_MARGIN  = 96;  // right pad that keeps the track's maximum well inside the panel border
const int TOP_MARGIN    = 6;
const int BOTTOM_MARGIN = 6;
const int LABEL_AREA_PX = 72;  // left strip for the label (line 1) and the value (line 2)
const int LABEL_GAP     = 8;   // space between label strip and track
const int TRACK_HEIGHT  = 10;  // visual track height (thin, prototype-like)
const int END_CAP_WIDTH = 3;   // accent bar drawn at the fill's right edge

bool update_color(QColor &slot, const QColor &color)
{
    if (slot == color) return false;
    slot = color;
    return true;
}
}

TimingBarWidget::TimingBarWidget(QWidget *parent) :
    QWidget(parent),
    m_drag_bar(-1),
    m_bg_color(0x12, 0x10, 0x0E),
    m_track_bg_color(0x04, 0x03, 0x03),
    m_track_edge_color(0x38, 0x2D, 0x25),
    m_label_color(0x8F, 0x6B, 0x4D),
    m_value_color(0xCD, 0xB2, 0x99),
    m_gradient_start_factor(220),
    m_gradient_end_factor(100)
{
    qRegisterMetaType<TimingBarWidget::BarId>("TimingBarWidget::BarId");

    setObjectName("TimingBarWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(320, 140); // four rows of label, value and thin track

    m_bars[LASER_WIDTH] = { 100,    1000,    QStringLiteral("Laser Width"), QStringLiteral("ns"), QColor(0x3d, 0x8b, 0xff) };
    m_bars[DISTANCE]    = { 500,    5000,    QStringLiteral("Distance"),    QStringLiteral("m"),  QColor(0x4c, 0xd9, 0x64) };
    m_bars[DOV]         = { 100,    1000,    QStringLiteral("DOV"),         QStringLiteral("m"),  QColor(0x4d, 0xd0, 0xe1) };
    m_bars[MCP]         = { 128,     255,    QStringLiteral("MCP"),         QString(),            QColor(0xa5, 0xe3, 0x4d) };
}

void TimingBarWidget::set_bar_value(BarId id, quint64 value)
{
    if (id < 0 || id >= BAR_COUNT) return;
    if (value > m_bars[id].scale) value = m_bars[id].scale;
    if (m_bars[id].value == value) return;
    m_bars[id].value = value;
    // no signal here: the controller calls this from its own update, emitting would loop
    update();
}

quint64 TimingBarWidget::get_bar_value(BarId id) const
{
    if (id < 0 || id >= BAR_COUNT) return 0;
    return m_bars[id].value;
}

void TimingBarWidget::set_bar_scale(BarId id, quint64 max_value)
{
    if (id < 0 || id >= BAR_COUNT) return;
    if (max_value == 0) return;
    if (m_bars[id].scale == max_value) return;
    m_bars[id].scale = max_value;
    if (m_bars[id].value > max_value) m_bars[id].value = max_value;
    // Scale change is configuration, not a user-driven value change.
    update();
}

quint64 TimingBarWidget::get_bar_scale(BarId id) const
{
    if (id < 0 || id >= BAR_COUNT) return 0;
    return m_bars[id].scale;
}

void TimingBarWidget::set_bar_label(BarId id, const QString &label)
{
    if (id < 0 || id >= BAR_COUNT) return;
    m_bars[id].label = label;
    update();
}

void TimingBarWidget::set_bar_unit(BarId id, const QString &unit)
{
    if (id < 0 || id >= BAR_COUNT) return;
    m_bars[id].unit = unit;
    update();
}

void TimingBarWidget::set_bar_color(BarId id, const QColor &color)
{
    if (id < 0 || id >= BAR_COUNT) return;
    if (m_bars[id].color == color) return;
    m_bars[id].color = color;
    update();
}

void TimingBarWidget::set_bar_background_color(const QColor &color)
{
    if (update_color(m_bg_color, color)) update();
}

void TimingBarWidget::set_bar_track_background_color(const QColor &color)
{
    if (update_color(m_track_bg_color, color)) update();
}

void TimingBarWidget::set_bar_track_border_color(const QColor &color)
{
    if (update_color(m_track_edge_color, color)) update();
}

void TimingBarWidget::set_bar_label_color(const QColor &color)
{
    if (update_color(m_label_color, color)) update();
}

void TimingBarWidget::set_bar_value_color(const QColor &color)
{
    if (update_color(m_value_color, color)) update();
}

void TimingBarWidget::set_laser_color(const QColor &color)
{
    set_bar_color(LASER_WIDTH, color);
}

void TimingBarWidget::set_distance_color(const QColor &color)
{
    set_bar_color(DISTANCE, color);
}

void TimingBarWidget::set_dov_color(const QColor &color)
{
    set_bar_color(DOV, color);
}

void TimingBarWidget::set_mcp_color(const QColor &color)
{
    set_bar_color(MCP, color);
}

void TimingBarWidget::set_bar_gradient_start_factor(int factor)
{
    const int bounded = qBound(100, factor, 300);
    if (m_gradient_start_factor == bounded) return;
    m_gradient_start_factor = bounded;
    update();
}

void TimingBarWidget::set_bar_gradient_end_factor(int factor)
{
    const int bounded = qBound(100, factor, 300);
    if (m_gradient_end_factor == bounded) return;
    m_gradient_end_factor = bounded;
    update();
}

quint64 TimingBarWidget::pixel_to_value(int px, int width_px, quint64 scale_max)
{
    if (width_px <= 0 || scale_max == 0) return 0;
    if (px <= 0) return 0;
    if (px >= width_px) return scale_max;
    return static_cast<quint64>(px) * scale_max / static_cast<quint64>(width_px);
}

int TimingBarWidget::value_to_pixel(quint64 value, int width_px, quint64 scale_max)
{
    if (width_px <= 0 || scale_max == 0) return 0;
    if (value >= scale_max) return width_px;
    return static_cast<int>(value * static_cast<quint64>(width_px) / scale_max);
}

int TimingBarWidget::row_height() const
{
    const int usable = height() - TOP_MARGIN - BOTTOM_MARGIN;
    return (usable - (BAR_COUNT - 1) * ROW_SPACING) / BAR_COUNT;
}

int TimingBarWidget::track_left() const
{
    return LEFT_MARGIN + LABEL_AREA_PX + LABEL_GAP;
}

int TimingBarWidget::track_width() const
{
    return qMax(0, width() - track_left() - RIGHT_MARGIN);
}

int TimingBarWidget::row_top(int idx) const
{
    return TOP_MARGIN + idx * (row_height() + ROW_SPACING);
}

int TimingBarWidget::end_handle_at(const QPoint &pos) const
{
    const int tw = track_width();
    const int rh = row_height();
    if (tw <= 0 || rh <= 0) return -1;

    for (int i = 0; i < BAR_COUNT; ++i) {
        const int top = row_top(i);
        if (pos.y() < top || pos.y() >= top + rh) continue;
        const int ex = track_left() + value_to_pixel(m_bars[i].value, tw, m_bars[i].scale);
        if (qAbs(pos.x() - ex) <= HANDLE_HIT_PX) return i;
    }
    return -1;
}

void TimingBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), m_bg_color);

    const int tw = track_width();
    const int rh = row_height();
    if (tw <= 0 || rh <= 0) return;

    const QFont base_font = p.font();
    QFont value_font = base_font;
    if (value_font.pointSize() > 0) {
        value_font.setPointSizeF(qMax(7.0, value_font.pointSizeF() - 1));
    }
    const QFontMetrics fm_label = QFontMetrics(base_font);
    const QFontMetrics fm_value = QFontMetrics(value_font);
    const int track_h = qMin(TRACK_HEIGHT, rh);

    for (int i = 0; i < BAR_COUNT; ++i) {
        const int top = row_top(i);
        const int track_y = top + (rh - track_h) / 2; // vertically center the thin track
        const Bar &b = m_bars[i];

        // left text strip: label on line 1, value and unit on line 2
        const QString value_text = b.unit.isEmpty()
            ? QString::number(b.value)
            : QStringLiteral("%1 %2").arg(b.value).arg(b.unit);

        const int line_h_label = fm_label.height();
        const int line_h_value = fm_value.height();
        const int stack_h = line_h_label + line_h_value;
        const int stack_top = top + (rh - stack_h) / 2;

        const QRect label_rect(LEFT_MARGIN, stack_top, LABEL_AREA_PX, line_h_label);
        const QRect value_rect(LEFT_MARGIN, stack_top + line_h_label,
                               LABEL_AREA_PX, line_h_value);

        p.setFont(base_font);
        p.setPen(m_label_color);
        p.drawText(label_rect, Qt::AlignLeft | Qt::AlignVCenter,
                   fm_label.elidedText(b.label, Qt::ElideRight, label_rect.width()));

        p.setFont(value_font);
        p.setPen(m_value_color);
        p.drawText(value_rect, Qt::AlignLeft | Qt::AlignVCenter,
                   fm_value.elidedText(value_text, Qt::ElideRight, value_rect.width()));
        p.setFont(base_font);

        // track and filled segment
        const QRectF track_rect(track_left(), track_y, tw, track_h);
        QPainterPath track_path;
        track_path.addRoundedRect(track_rect, track_h / 2.0, track_h / 2.0);
        p.fillPath(track_path, m_track_bg_color);

        const int seg_w = qMax(0, value_to_pixel(b.value, tw, b.scale));
        if (seg_w > 0) {
            const QRectF seg_rect(track_rect.left(), track_y, seg_w, track_h);
            QLinearGradient g(seg_rect.topLeft(), seg_rect.topRight());
            g.setColorAt(0.0, b.color.darker(m_gradient_start_factor));
            g.setColorAt(1.0, b.color.lighter(m_gradient_end_factor));
            QPainterPath seg_path;
            seg_path.addRoundedRect(seg_rect, track_h / 2.0, track_h / 2.0);
            p.save();
            p.setClipPath(track_path);
            p.fillPath(seg_path, g);
            p.restore();
        }

        // Track outline.
        p.setPen(m_track_edge_color);
        p.setBrush(Qt::NoBrush);
        p.drawPath(track_path);

        // Right-end cap: an accent stripe at the fill's right edge, also at full scale.
        // Clipped to the track path so the rounded corner stays clean.
        if (seg_w > 0) {
            double cap_left  = track_rect.left() + seg_w - END_CAP_WIDTH;
            double cap_right = track_rect.left() + seg_w;
            // Keep the cap inside the track at the maximum: shift it left when it would
            // overflow the rounded right corner.
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
    int bar = end_handle_at(event->pos());

    // Otherwise: if the press is inside a bar's track, jump the value to the
    // clicked x and let the subsequent drag refine it.
    if (bar < 0) {
        const int tw = track_width();
        const int rh = row_height();
        const int x  = event->pos().x();
        const int y  = event->pos().y();
        if (tw > 0 && rh > 0 && x >= track_left() && x <= track_left() + tw) {
            for (int i = 0; i < BAR_COUNT; ++i) {
                const int top = row_top(i);
                if (y < top || y >= top + rh) continue;
                const quint64 new_value = pixel_to_value(x - track_left(), tw, m_bars[i].scale);
                if (new_value != m_bars[i].value) {
                    m_bars[i].value = new_value;
                    emit bar_value_changed(static_cast<BarId>(i), new_value);
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
        const int tw = track_width();
        const int local_x = event->pos().x() - track_left();
        Bar &b = m_bars[m_drag_bar];
        const quint64 new_value = pixel_to_value(local_x, tw, b.scale);
        if (new_value != b.value) {
            b.value = new_value;
            emit bar_value_changed(static_cast<BarId>(m_drag_bar), new_value);
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
