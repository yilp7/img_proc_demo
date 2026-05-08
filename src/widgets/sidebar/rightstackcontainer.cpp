#include "rightstackcontainer.h"

#include <QVBoxLayout>

RightStackContainer::RightStackContainer(QWidget *parent)
    : QFrame(parent), m_layout(nullptr)
{
    setObjectName("RIGHT_STACK");
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(6);
}

void RightStackContainer::addSection(QWidget *w, int stretch)
{
    if (!w) return;
    w->setParent(this);
    m_layout->addWidget(w, stretch);
}
