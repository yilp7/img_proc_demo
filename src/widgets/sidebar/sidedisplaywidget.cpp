#include "sidedisplaywidget.h"

#include "widgets/display.h"

#include <QLabel>
#include <QVBoxLayout>

SideDisplayWidget::SideDisplayWidget(QWidget *parent)
    : QFrame(parent), m_layout(nullptr), m_placeholder(nullptr),
      m_display(nullptr), m_active(false)
{
    setObjectName("SIDE_DISPLAY_WIDGET");
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_placeholder = new QLabel("Side display — inactive", this);
    m_placeholder->setObjectName("SIDE_DISPLAY_PLACEHOLDER");
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_placeholder);
}

void SideDisplayWidget::setDisplay(Display *display)
{
    if (m_display == display) return;

    if (m_display) {
        m_layout->removeWidget(m_display);
        m_display->setParent(nullptr);
    }

    m_display = display;
    if (m_display) {
        m_display->setParent(this);
        m_layout->addWidget(m_display);
    }

    setActive(m_active);  // re-apply visibility rules
}

void SideDisplayWidget::setActive(bool active)
{
    m_active = active;
    const bool show_display = active && m_display != nullptr;
    m_placeholder->setVisible(!show_display);
    if (m_display) m_display->setVisible(show_display);
}
