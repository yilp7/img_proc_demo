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

    m_colors[Off]  = QColor(0x55, 0x55, 0x55);
    m_colors[Ok]   = QColor(0x4c, 0xd9, 0x64);
    m_colors[Warn] = QColor(0xff, 0xc1, 0x07);
    m_colors[Err]  = QColor(0xe5, 0x3e, 0x3e);
}

void DotIndicator::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    update();
}

void DotIndicator::setColorForState(State s, const QColor &color)
{
    if (s < 0 || s > Err) return;
    m_colors[s] = color;
    if (s == m_state) update();
}

void DotIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const QPointF c = r.center();
    const double dot_r = qMin(r.width(), r.height()) / 2.0;

    // Soft glow when Ok.
    if (m_state == Ok) {
        QRadialGradient g(c, dot_r * 1.8);
        QColor glow = m_colors[Ok];
        glow.setAlpha(140);
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
