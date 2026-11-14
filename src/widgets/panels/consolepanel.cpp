#include "consolepanel.h"

#include "widgets/mywidget.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {
const int OPTION_NONE = 0;
const int OPTION_DATA = 1;
const int OPTION_HIST = 2;
const int OPTION_ALT  = 4;
const int OPTION_VID  = 6;

struct OptionSpec {
    int option;
};

const OptionSpec SELECTOR_OPTIONS[] = {
    { OPTION_DATA },
    { OPTION_HIST },
    { OPTION_ALT  },
    { OPTION_VID  },
    { OPTION_NONE },
};
}

ConsolePanel::ConsolePanel(QWidget *parent) :
    FloatingPanel(parent),
    m_selector_group(nullptr),
    m_stack(nullptr),
    m_selectors{nullptr, nullptr, nullptr},
    m_radios{nullptr, nullptr, nullptr},
    m_page_index_by_option(),
    m_updating(false)
{
    set_title(tr("Console"));

    auto *selector_bar = new QFrame(this);
    selector_bar->setObjectName("CONSOLE_SELECTOR_BAR");
    selector_bar->setAttribute(Qt::WA_StyledBackground, true);

    auto *selector_layout = new QHBoxLayout(selector_bar);
    selector_layout->setContentsMargins(3, 2, 3, 2);
    selector_layout->setSpacing(4);

    m_selector_group = new QButtonGroup(this);
    m_selector_group->setExclusive(true);

    for (int i = 0; i < 3; ++i) {
        m_radios[i] = new QRadioButton(selector_bar);
        m_radios[i]->setObjectName(QString("CONSOLE_SELECTOR_RADIO_%1").arg(i + 1));
        m_radios[i]->setFixedSize(14, 18);
        m_selector_group->addButton(m_radios[i], i);
        selector_layout->addWidget(m_radios[i]);

        m_selectors[i] = new MiscSelection(selector_bar);
        m_selectors[i]->setObjectName(QString("CONSOLE_SELECTOR_%1").arg(i + 1));
        m_selectors[i]->setFixedSize(46, 20);
        selector_layout->addWidget(m_selectors[i]);

        connect(m_selectors[i], &MiscSelection::selected, this, [this, i]() {
            activate_selector(i);
        });
        connect(m_selectors[i], QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, i](int) {
            activate_selector(i);
        });
    }
    selector_layout->addStretch(1);

    connect(m_selector_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, [this](int id) {
        activate_selector(id);
    });

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName("CONSOLE_STACK");

    get_content_layout()->addWidget(selector_bar);
    get_content_layout()->addWidget(m_stack, 1);
}

void ConsolePanel::setup(QStackedWidget *misc_display)
{
    struct PageSpec { int option; const char *page_name; };
    static const PageSpec specs[] = {
        { OPTION_DATA, "DATA_PAGE"     },
        { OPTION_HIST, "HIST_PAGE"     },
        { OPTION_ALT,  "ALT_CTRL_PAGE" },
        { OPTION_VID,  "VID_PAGE"      },
    };

    m_page_index_by_option.clear();
    for (const auto &s : specs) {
        QWidget *page = misc_display->findChild<QWidget*>(
            QString::fromLatin1(s.page_name), Qt::FindDirectChildrenOnly);
        if (!page) continue;
        misc_display->removeWidget(page);
        page->show();
        const int idx = m_stack->addWidget(page);
        m_page_index_by_option.insert(s.option, idx);
    }

    auto *placeholder = new QLabel(tr("3D point cloud - placeholder"), m_stack);
    placeholder->setObjectName("CONSOLE_PLACEHOLDER");
    placeholder->setAlignment(Qt::AlignCenter);
    m_page_index_by_option.insert(OPTION_NONE, m_stack->addWidget(placeholder));

    m_updating = true;
    populate_selector(m_selectors[0], OPTION_HIST);
    populate_selector(m_selectors[1], OPTION_ALT);
    populate_selector(m_selectors[2], OPTION_VID);
    m_updating = false;

    m_radios[2]->setChecked(true);
    activate_selector(2);
}

int ConsolePanel::get_current_option() const
{
    const int id = m_selector_group ? m_selector_group->checkedId() : 0;
    return get_selector_option(id < 0 ? 0 : id);
}

void ConsolePanel::populate_selector(MiscSelection *selector, int default_option)
{
    if (!selector) return;
    selector->clear();
    for (const auto &s : SELECTOR_OPTIONS) {
        QString label;
        switch (s.option) {
        case OPTION_DATA: label = tr("DATA"); break;
        case OPTION_HIST: label = tr("HIST"); break;
        case OPTION_ALT:  label = tr("ALT");  break;
        case OPTION_VID:  label = tr("VID");  break;
        default:          label = tr("3D");   break;
        }
        selector->addItem(label, s.option);
    }
    const int idx = selector->findData(default_option);
    if (idx >= 0) selector->setCurrentIndex(idx);
}

void ConsolePanel::activate_selector(int selector_index)
{
    if (m_updating || selector_index < 0 || selector_index >= 3) return;
    if (m_radios[selector_index]) m_radios[selector_index]->setChecked(true);
    activate_option(get_selector_option(selector_index));
}

void ConsolePanel::activate_option(int option_id)
{
    const int stack_index = m_page_index_by_option.value(option_id, -1);
    if (stack_index >= 0 && m_stack) m_stack->setCurrentIndex(stack_index);
    emit option_selected(option_id);
}

int ConsolePanel::get_selector_option(int selector_index) const
{
    if (selector_index < 0 || selector_index >= 3 || !m_selectors[selector_index]) return OPTION_NONE;
    return m_selectors[selector_index]->currentData().toInt();
}
