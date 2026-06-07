#include "ptzradarwidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QtMath>
#include <algorithm>
#include <cmath>

PtzRadarWidget::PtzRadarWidget(QWidget *parent)
    : QFrame(parent),
      m_angle_h(0.0f),
      m_angle_v(0.0f),
      m_zero_angle_h(0.0f)
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

    const QColor bg(0x04, 0x03, 0x03);
    const QColor border(0x24, 0x1E, 0x1A);
    const QColor grid(0x38, 0x2D, 0x25);
    const QColor grid_soft(0x24, 0x1E, 0x1A);
    const QColor text(0xC8, 0xB8, 0xA8);
    const QColor muted(0x8A, 0x74, 0x61);
    const QColor accent(0xE0, 0x92, 0x4A);

    p.setPen(QPen(border, 1));
    p.setBrush(bg);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    QFont title_font = font();
    title_font.setPixelSize(10);
    title_font.setBold(true);
    p.setFont(title_font);
    p.setPen(text);
    p.drawText(QRect(8, 6, width() - 16, 14), Qt::AlignLeft | Qt::AlignVCenter, "PTZ RADAR");

    QFont value_font = font();
    value_font.setPixelSize(9);
    p.setFont(value_font);
    const float rel = normalizeAngle(m_angle_h - m_zero_angle_h);
    const QString value = QString("H %1  V %2")
        .arg(QString::asprintf("%06.2f", m_angle_h))
        .arg(QString::asprintf("%05.2f", m_angle_v));
    p.setFont(value_font);
    p.setPen(muted);
    p.drawText(QRect(8, 6, width() - 16, 14),
               Qt::AlignRight | Qt::AlignVCenter, value);

    const int top = 26;
    const int bottom = 8;
    const int side = std::max(64, std::min(width() - 18, height() - top - bottom));
    const QRectF dial((width() - side) / 2.0, top + (height() - top - bottom - side) / 2.0, side, side);
    const QPointF center = dial.center();
    const qreal radius = dial.width() / 2.0 - 13.0;

    p.setPen(QPen(grid_soft, 1));
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
        p.setPen(QPen(deg % 90 == 0 ? grid : grid_soft, deg % 90 == 0 ? 1.4 : 1.0));
        p.drawLine(inner, outer);

        const QString label = deg == 0 ? QString("0/360") : QString::number(deg);
        const qreal label_r = radius + 7.0;
        const QPointF label_pos(center.x() + std::sin(rad) * label_r,
                                center.y() - std::cos(rad) * label_r);
        const QRectF label_rect(label_pos.x() - fm.horizontalAdvance(label) / 2.0,
                                label_pos.y() - fm.height() / 2.0,
                                fm.horizontalAdvance(label),
                                fm.height());
        p.setPen(deg == 0 ? accent : muted);
        p.drawText(label_rect, Qt::AlignCenter, label);
    }

    const qreal angle_rad = qDegreesToRadians(static_cast<qreal>(rel));
    const QPointF tip(center.x() + std::sin(angle_rad) * radius * 0.82,
                      center.y() - std::cos(angle_rad) * radius * 0.82);
    const QPointF tail(center.x() - std::sin(angle_rad) * radius * 0.18,
                       center.y() + std::cos(angle_rad) * radius * 0.18);
    p.setPen(QPen(accent, 2.2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(tail, tip);
    p.setBrush(accent);
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
