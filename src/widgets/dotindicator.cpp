#include "dotindicator.h"

#include <QPainter>
#include <QPaintEvent>
#include <QRadialGradient>

namespace {
const int DEFAULT_SIZE = 12;
}

DotIndicator::DotIndicator(QWidget *parent) :
    QWidget(parent),
    m_state(OFF)
{
    setObjectName("DotIndicator");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedSize(DEFAULT_SIZE, DEFAULT_SIZE);

    // default colours
    m_colors[OFF]  = QColor(0x24, 0x1E, 0x1A);  // dot-off
    m_colors[OK]   = QColor(0x00, 0xff, 0x7f);  // positive
    m_colors[WARN] = QColor(0xE0, 0x92, 0x4A);  // accent (yellow dot)
    m_colors[ERR]  = QColor(0xE6, 0x45, 0x5D);  // negative
}

void DotIndicator::set_state(State s)
{
    if (m_state == s) return;
    m_state = s;
    update();
}

void DotIndicator::set_off_color(const QColor &color)
{
    set_color_for_state(OFF, color);
}

void DotIndicator::set_ok_color(const QColor &color)
{
    set_color_for_state(OK, color);
}

void DotIndicator::set_warn_color(const QColor &color)
{
    set_color_for_state(WARN, color);
}

void DotIndicator::set_err_color(const QColor &color)
{
    set_color_for_state(ERR, color);
}

void DotIndicator::set_color_for_state(State s, const QColor &color)
{
    if (s < 0 || s > ERR) return;
    if (m_colors[s] == color) return;
    m_colors[s] = color;
    if (s == m_state) update();
}

void DotIndicator::update_status(int status)
{
    static const State map[] = { OFF, ERR, ERR, WARN, OK };
    set_state(status >= 0 && status <= 4 ? map[status] : OFF);
}

void DotIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const QPointF c = r.center();
    const double dot_r = qMin(r.width(), r.height()) / 2.0;

    // soft glow under every live state; OFF has none
    if (m_state != OFF) {
        QRadialGradient g(c, dot_r * 1.8);
        QColor glow = m_colors[m_state];
        // Alpha matches prototype: positive-glow / accent-glow-33 / negative-glow.
        glow.setAlpha(m_state == WARN ? 0x55 : 0x88);
        g.setColorAt(0.0, glow);
        glow.setAlpha(0);
        g.setColorAt(1.0, glow);
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawEllipse(c, dot_r * 1.8, dot_r * 1.8);
    }

    // Core dot.
    p.setPen(Qt::NoPen);
    p.setBrush(m_colors[m_state]);
    p.drawEllipse(c, dot_r, dot_r);
}
