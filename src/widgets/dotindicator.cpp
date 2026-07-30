#include "dotindicator.h"

#include <QPainter>
#include <QPaintEvent>
#include <QRadialGradient>

namespace {
const int kDefaultSize = 12;
}

DotIndicator::DotIndicator(QWidget *parent) :
    QWidget(parent),
    m_state(Off)
{
    setObjectName("DotIndicator");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedSize(kDefaultSize, kDefaultSize);

    // Theme tokens — prototype v3 dot-{x,g,y,r} colors.
    m_colors[Off]  = QColor(0x24, 0x1E, 0x1A);  // dot-off
    m_colors[Ok]   = QColor(0x00, 0xff, 0x7f);  // positive
    m_colors[Warn] = QColor(0xE0, 0x92, 0x4A);  // accent (yellow dot)
    m_colors[Err]  = QColor(0xE6, 0x45, 0x5D);  // negative
}

void DotIndicator::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    update();
}

void DotIndicator::setOffColor(const QColor &color)
{
    setColorForState(Off, color);
}

void DotIndicator::setOkColor(const QColor &color)
{
    setColorForState(Ok, color);
}

void DotIndicator::setWarnColor(const QColor &color)
{
    setColorForState(Warn, color);
}

void DotIndicator::setErrColor(const QColor &color)
{
    setColorForState(Err, color);
}

void DotIndicator::setColorForState(State s, const QColor &color)
{
    if (s < 0 || s > Err) return;
    if (m_colors[s] == color) return;
    m_colors[s] = color;
    if (s == m_state) update();
}

void DotIndicator::update_status(int status)
{
    static const State map[] = { Off, Err, Err, Warn, Ok };
    setState(status >= 0 && status <= 4 ? map[status] : Off);
}

void DotIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const QPointF c = r.center();
    const double dot_r = qMin(r.width(), r.height()) / 2.0;

    // Soft glow under all live states (matches prototype `box-shadow: 0 0 5px`).
    // Off has no glow.
    if (m_state != Off) {
        QRadialGradient g(c, dot_r * 1.8);
        QColor glow = m_colors[m_state];
        // Alpha matches prototype: positive-glow / accent-glow-33 / negative-glow.
        glow.setAlpha(m_state == Warn ? 0x55 : 0x88);
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
