#include "opticspanel.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

OpticsPanel::OpticsPanel(QWidget *parent)
    : FloatingPanel(parent),
      m_stack(nullptr), m_classic_page(nullptr),
      m_modern_page(nullptr), m_mode_group(nullptr)
{
    setTitle("Optics & PTZ");

    // mode-toggle row
    auto *toggle_row = new QWidget(this);
    auto *toggle_layout = new QHBoxLayout(toggle_row);
    toggle_layout->setContentsMargins(4, 2, 4, 2);
    toggle_layout->setSpacing(2);

    m_mode_group = new QButtonGroup(this);
    auto makeTab = [&](const char *text, int id) {
        auto *btn = new QPushButton(text, toggle_row);
        btn->setCheckable(true);
        btn->setObjectName(id == 0 ? "OPTICS_CLASSIC_BTN" : "OPTICS_MODERN_BTN");
        m_mode_group->addButton(btn, id);
        toggle_layout->addWidget(btn);
    };
    makeTab("Classic", 0);
    makeTab("Modern",  1);
    m_mode_group->setExclusive(true);
    m_mode_group->button(0)->setChecked(true);
    toggle_layout->addStretch(1);

    connect(m_mode_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &OpticsPanel::on_mode_toggled);

    // stacked widget
    m_stack = new QStackedWidget(this);
    m_classic_page = new QWidget(m_stack);
    m_modern_page  = new QWidget(m_stack);
    m_stack->addWidget(m_classic_page);  // index 0 = Classic
    m_stack->addWidget(m_modern_page);   // index 1 = Modern
    m_stack->setCurrentIndex(0);

    contentLayout()->addWidget(toggle_row);
    contentLayout()->addWidget(m_stack, 1);
}

void OpticsPanel::setup(QWidget *lens_static, QWidget *ptz_grp,
                        QWidget *iris_open, QWidget *iris_close)
{
    // iris buttons are duplicates of radius — delete them per sprint plan
    delete iris_open;
    delete iris_close;

    auto *page_layout = new QVBoxLayout(m_classic_page);
    page_layout->setContentsMargins(4, 4, 4, 4);
    page_layout->setSpacing(6);

    if (ptz_grp) {
        ptz_grp->setParent(m_classic_page);
        page_layout->addWidget(ptz_grp);
    }
    if (lens_static) {
        lens_static->setParent(m_classic_page);
        page_layout->addWidget(lens_static);
    }

    adjustSize();
}

void OpticsPanel::set_modern_page(QWidget *w)
{
    auto *layout = new QVBoxLayout(m_modern_page);
    layout->setContentsMargins(4, 4, 4, 4);
    w->setParent(m_modern_page);
    layout->addWidget(w, 1);
    m_modern_page->adjustSize();
}

OpticsPanel::Mode OpticsPanel::current_mode() const
{
    return static_cast<Mode>(m_stack->currentIndex());
}

void OpticsPanel::on_mode_toggled(int id)
{
    m_stack->setCurrentIndex(id);
    emit modeChanged(static_cast<Mode>(id));
}
