#include "tcupanel.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

TCUPanel::TCUPanel(QWidget *parent)
    : FloatingPanel(parent),
      m_fire_row(nullptr), m_stack(nullptr),
      m_normal_page(nullptr), m_guru_page(nullptr), m_mode_group(nullptr)
{
    setTitle("TCU & Laser");

    // always-visible laser button row (populated in setup())
    m_fire_row = new QWidget(this);
    auto *fr_layout = new QHBoxLayout(m_fire_row);
    fr_layout->setContentsMargins(4, 2, 4, 2);
    fr_layout->setSpacing(6);
    fr_layout->addStretch(1); // buttons inserted by setup()

    // mode-toggle row: Normal (0) | Guru (1)
    auto *toggle_row = new QWidget(this);
    auto *toggle_layout = new QHBoxLayout(toggle_row);
    toggle_layout->setContentsMargins(4, 2, 4, 2);
    toggle_layout->setSpacing(2);

    m_mode_group = new QButtonGroup(this);
    auto makeTab = [&](const char *text, int id) {
        auto *btn = new QPushButton(text, toggle_row);
        btn->setCheckable(true);
        btn->setObjectName(id == 0 ? "TCU_NORMAL_BTN" : "TCU_GURU_BTN");
        m_mode_group->addButton(btn, id);
        toggle_layout->addWidget(btn);
    };
    makeTab("Normal", 0);
    makeTab("Guru",   1);
    m_mode_group->setExclusive(true);
    // Default is Guru for 16.7 (Normal page is empty until 16.8)
    m_mode_group->button(1)->setChecked(true);
    toggle_layout->addStretch(1);

    connect(m_mode_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &TCUPanel::on_mode_toggled);

    // stacked widget
    m_stack = new QStackedWidget(this);
    m_normal_page = new QWidget(m_stack);
    m_guru_page   = new QWidget(m_stack);
    m_stack->addWidget(m_normal_page);  // index 0 = Normal (stub until 16.8)
    m_stack->addWidget(m_guru_page);    // index 1 = Guru (TCU_STATIC)
    m_stack->setCurrentIndex(1);        // default: Guru

    contentLayout()->addWidget(m_fire_row);
    contentLayout()->addWidget(toggle_row);
    contentLayout()->addWidget(m_stack, 1);
}

void TCUPanel::setup(QWidget *tcu_static, QWidget *laser_btn, QWidget *fire_btn)
{
    // laser + fire buttons: always visible at top
    auto *fr_layout = qobject_cast<QHBoxLayout*>(m_fire_row->layout());
    if (laser_btn) {
        laser_btn->setParent(m_fire_row);
        fr_layout->insertWidget(0, laser_btn);
    }
    if (fire_btn) {
        fire_btn->setParent(m_fire_row);
        fr_layout->insertWidget(1, fire_btn);
    }

    // TCU_STATIC → Guru page
    auto *gl = new QVBoxLayout(m_guru_page);
    gl->setContentsMargins(4, 4, 4, 4);
    if (tcu_static) {
        tcu_static->setParent(m_guru_page);
        gl->addWidget(tcu_static);
    }

    // Normal page: label placeholder until 16.8
    auto *nl = new QVBoxLayout(m_normal_page);
    nl->setContentsMargins(4, 4, 4, 4);
    nl->addWidget(new QLabel("Normal mode — populated in Sprint 16.8", m_normal_page));

    adjustSize();
}

void TCUPanel::set_normal_page(QWidget *w)
{
    // Remove placeholder and add real content (called in 16.8)
    qDeleteAll(m_normal_page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly));
    auto *layout = m_normal_page->layout();
    if (!layout) layout = new QVBoxLayout(m_normal_page);
    w->setParent(m_normal_page);
    layout->addWidget(w);
    // Switch default to Normal now that it has content
    m_stack->setCurrentIndex(0);
    m_mode_group->button(0)->setChecked(true);
}

TCUPanel::Mode TCUPanel::current_mode() const
{
    return static_cast<Mode>(m_stack->currentIndex());
}

void TCUPanel::on_mode_toggled(int id)
{
    m_stack->setCurrentIndex(id);
    emit modeChanged(static_cast<Mode>(id));
}
