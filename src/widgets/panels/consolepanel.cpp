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
const int kOptionNone = 0;
const int kOptionData = 1;
const int kOptionHist = 2;
const int kOptionAlt  = 4;
const int kOptionVid  = 6;

struct OptionSpec
{
    int option;
};

const OptionSpec kSelectorOptions[] = {
    { kOptionData },
    { kOptionHist },
    { kOptionAlt  },
    { kOptionVid  },
    { kOptionNone },
};
}

ConsolePanel::ConsolePanel(QWidget *parent)
    : FloatingPanel(parent),
      m_selector_group(nullptr),
      m_stack(nullptr),
      m_selectors{nullptr, nullptr, nullptr},
      m_radios{nullptr, nullptr, nullptr},
      m_page_index_by_option(),
      m_updating(false)
{
    setTitle(tr("Console"));

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
            activateSelector(i);
        });
        connect(m_selectors[i], QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, i](int) {
            activateSelector(i);
        });
    }
    selector_layout->addStretch(1);

    connect(m_selector_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, [this](int id) {
        activateSelector(id);
    });

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName("CONSOLE_STACK");

    contentLayout()->addWidget(selector_bar);
    contentLayout()->addWidget(m_stack, 1);
}

void ConsolePanel::setup(QStackedWidget *misc_display)
{
    struct PageSpec { int option; const char *pageName; };
    static const PageSpec specs[] = {
        { kOptionData, "DATA_PAGE"     },
        { kOptionHist, "HIST_PAGE"     },
        { kOptionAlt,  "ALT_CTRL_PAGE" },
        { kOptionVid,  "VID_PAGE"      },
    };

    m_page_index_by_option.clear();
    for (const auto &s : specs) {
        QWidget *page = misc_display->findChild<QWidget*>(
            QString::fromLatin1(s.pageName), Qt::FindDirectChildrenOnly);
        if (!page) continue;
        misc_display->removeWidget(page);
        page->show();
        const int idx = m_stack->addWidget(page);
        m_page_index_by_option.insert(s.option, idx);
    }

    auto *placeholder = new QLabel(tr("3D point cloud - placeholder"), m_stack);
    placeholder->setObjectName("CONSOLE_PLACEHOLDER");
    placeholder->setAlignment(Qt::AlignCenter);
    m_page_index_by_option.insert(kOptionNone, m_stack->addWidget(placeholder));

    m_updating = true;
    populateSelector(m_selectors[0], kOptionHist);
    populateSelector(m_selectors[1], kOptionAlt);
    populateSelector(m_selectors[2], kOptionVid);
    m_updating = false;

    m_radios[2]->setChecked(true);
    activateSelector(2);
}

int ConsolePanel::currentOption() const
{
    const int id = m_selector_group ? m_selector_group->checkedId() : 0;
    return selectorOption(id < 0 ? 0 : id);
}

void ConsolePanel::populateSelector(MiscSelection *selector, int defaultOption)
{
    if (!selector) return;
    selector->clear();
    for (const auto &s : kSelectorOptions) {
        QString label;
        switch (s.option) {
        case kOptionData: label = tr("DATA"); break;
        case kOptionHist: label = tr("HIST"); break;
        case kOptionAlt:  label = tr("ALT");  break;
        case kOptionVid:  label = tr("VID");  break;
        default:          label = tr("3D");   break;
        }
        selector->addItem(label, s.option);
    }
    const int idx = selector->findData(defaultOption);
    if (idx >= 0) selector->setCurrentIndex(idx);
}

void ConsolePanel::activateSelector(int selectorIndex)
{
    if (m_updating || selectorIndex < 0 || selectorIndex >= 3) return;
    if (m_radios[selectorIndex]) m_radios[selectorIndex]->setChecked(true);
    activateOption(selectorOption(selectorIndex));
}

void ConsolePanel::activateOption(int optionId)
{
    const int stack_index = m_page_index_by_option.value(optionId, -1);
    if (stack_index >= 0 && m_stack) m_stack->setCurrentIndex(stack_index);
    emit optionSelected(optionId);
}

int ConsolePanel::selectorOption(int selectorIndex) const
{
    if (selectorIndex < 0 || selectorIndex >= 3 || !m_selectors[selectorIndex]) return kOptionNone;
    return m_selectors[selectorIndex]->currentData().toInt();
}
