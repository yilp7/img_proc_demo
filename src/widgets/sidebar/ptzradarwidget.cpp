#include "ptzradarwidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
bool updateColor(QColor &slot, const QColor &color)
{
    if (slot == color) return false;
    slot = color;
    return true;
}
}

PtzRadarWidget::PtzRadarWidget(QWidget *parent)
    : QFrame(parent),
      m_angle_h(0.0f),
      m_angle_v(0.0f),
      m_zero_angle_h(0.0f),
      m_bg_color(0x04, 0x03, 0x03),
      m_border_color(0x24, 0x1E, 0x1A),
      m_grid_color(0x38, 0x2D, 0x25),
      m_grid_soft_color(0x24, 0x1E, 0x1A),
      m_text_color(0xC8, 0xB8, 0xA8),
      m_muted_color(0x8A, 0x74, 0x61),
      m_accent_color(0xE0, 0x92, 0x4A)
{
    setObjectName("PTZ_RADAR_WIDGET");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(190);
    setToolTip("device.ptz_zero_angle_h calibrates radar 0/360");
}

QSize PtzRadarWidget::sizeHint() const
{
    return QSize(212, 212);
}

QSize PtzRadarWidget::minimumSizeHint() const
{
    return QSize(180, 180);
}

void PtzRadarWidget::setRadarBackgroundColor(const QColor &color)
{
    if (updateColor(m_bg_color, color)) update();
}

void PtzRadarWidget::setRadarBorderColor(const QColor &color)
{
    if (updateColor(m_border_color, color)) update();
}

void PtzRadarWidget::setRadarGridColor(const QColor &color)
{
    if (updateColor(m_grid_color, color)) update();
}

void PtzRadarWidget::setRadarGridSoftColor(const QColor &color)
{
    if (updateColor(m_grid_soft_color, color)) update();
}

void PtzRadarWidget::setRadarTextColor(const QColor &color)
{
    if (updateColor(m_text_color, color)) update();
}

void PtzRadarWidget::setRadarMutedColor(const QColor &color)
{
    if (updateColor(m_muted_color, color)) update();
}

void PtzRadarWidget::setRadarAccentColor(const QColor &color)
{
    if (updateColor(m_accent_color, color)) update();
}

void PtzRadarWidget::setAngle(float h, float v)
{
    m_angle_h = normalizeAngle(h);
    m_angle_v = v;
    update();
}

void PtzRadarWidget::setZeroAngle(float h)
{
    m_zero_angle_h = normalizeAngle(h);
    update();
}

void PtzRadarWidget::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(QPen(m_border_color, 1));
    p.setBrush(m_bg_color);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    QFont title_font = font();
    title_font.setPixelSize(10);
    title_font.setBold(true);
    p.setFont(title_font);
    p.setPen(m_text_color);
    p.drawText(QRect(8, 6, width() - 16, 14), Qt::AlignLeft | Qt::AlignVCenter, "PTZ RADAR");

    QFont value_font = font();
    value_font.setPixelSize(9);
    p.setFont(value_font);
    const float rel = normalizeAngle(m_angle_h - m_zero_angle_h);
    const QString value = QString("H %1  V %2")
        .arg(QString::asprintf("%06.2f", rel))
        .arg(QString::asprintf("%+06.2f", m_angle_v));
    p.setFont(value_font);
    p.setPen(m_muted_color);
    p.drawText(QRect(8, 6, width() - 16, 14),
               Qt::AlignRight | Qt::AlignVCenter, value);

    const int top = 26;
    const int bottom = 8;
    const int side = std::max(64, std::min(width() - 18, height() - top - bottom));
    const QRectF dial((width() - side) / 2.0, top + (height() - top - bottom - side) / 2.0, side, side);
    const QPointF center = dial.center();
    const qreal radius = dial.width() / 2.0 - 13.0;

    p.setPen(QPen(m_grid_soft_color, 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, radius, radius);
    p.drawEllipse(center, radius * 0.67, radius * 0.67);
    p.drawEllipse(center, radius * 0.34, radius * 0.34);

    QFont label_font = font();
    label_font.setPixelSize(8);
    p.setFont(label_font);
    QFontMetrics fm(label_font);

    for (int deg = 0; deg < 360; deg += 45) {
        const qreal rad = qDegreesToRadians(static_cast<qreal>(deg));
        const QPointF outer(center.x() + std::sin(rad) * radius,
                            center.y() - std::cos(rad) * radius);
        const QPointF inner(center.x() + std::sin(rad) * radius * 0.18,
                            center.y() - std::cos(rad) * radius * 0.18);
        p.setPen(QPen(deg % 90 == 0 ? m_grid_color : m_grid_soft_color, deg % 90 == 0 ? 1.4 : 1.0));
        p.drawLine(inner, outer);

        const QString label = deg == 0 ? QString("0/360") : QString::number(deg);
        const qreal label_r = radius + 7.0;
        const QPointF label_pos(center.x() + std::sin(rad) * label_r,
                                center.y() - std::cos(rad) * label_r);
        const QRectF label_rect(label_pos.x() - fm.horizontalAdvance(label) / 2.0,
                                label_pos.y() - fm.height() / 2.0,
                                fm.horizontalAdvance(label),
                                fm.height());
        p.setPen(deg == 0 ? m_accent_color : m_muted_color);
        p.drawText(label_rect, Qt::AlignCenter, label);
    }

    const qreal angle_rad = qDegreesToRadians(static_cast<qreal>(rel));
    const QPointF tip(center.x() + std::sin(angle_rad) * radius * 0.82,
                      center.y() - std::cos(angle_rad) * radius * 0.82);
    const QPointF tail(center.x() - std::sin(angle_rad) * radius * 0.18,
                       center.y() + std::cos(angle_rad) * radius * 0.18);
    p.setPen(QPen(m_accent_color, 2.2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(tail, tip);
    p.setBrush(m_accent_color);
    p.drawEllipse(center, 3.0, 3.0);
    p.drawEllipse(tip, 3.5, 3.5);

    Q_UNUSED(event);
}

float PtzRadarWidget::normalizeAngle(float angle)
{
    float normalized = std::fmod(angle, 360.0f);
    if (normalized < 0.0f) normalized += 360.0f;
    if (normalized >= 360.0f) normalized -= 360.0f;
    return normalized;
}
