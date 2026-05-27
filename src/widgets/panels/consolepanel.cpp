#include "consolepanel.h"

#include <QLabel>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

ConsolePanel::ConsolePanel(QWidget *parent)
    : FloatingPanel(parent), m_tabs(nullptr)
{
    setTitle("Console");

    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName("CONSOLE_TABS");
    contentLayout()->addWidget(m_tabs, 1);
}

void ConsolePanel::setup(QStackedWidget *misc_display)
{
    struct TabSpec { const char *label; const char *pageName; };
    static const TabSpec specs[] = {
        { "DATA",  "DATA_PAGE"     },
        { "HIST",  "HIST_PAGE"     },
        { "ALT",   "ALT_CTRL_PAGE" },
        { "ADDON", "ADDON_PAGE"    },
        { "VID",   "VID_PAGE"      },
    };

    int vid_tab_idx = -1;
    for (const auto &s : specs) {
        QWidget *page = misc_display->findChild<QWidget*>(
            QString::fromLatin1(s.pageName), Qt::FindDirectChildrenOnly);
        if (!page) continue;
        misc_display->removeWidget(page);
        page->show();
        const int idx = m_tabs->addTab(page, s.label);
        if (QByteArray(s.label) == "VID") vid_tab_idx = idx;
    }

    m_tabs->addTab(new QLabel("3D point cloud — placeholder"), "3D");

    if (vid_tab_idx >= 0) m_tabs->setCurrentIndex(vid_tab_idx);
}
