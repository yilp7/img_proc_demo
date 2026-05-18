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
    // Map tab labels to source pages by objectName. Index-based extraction
    // is brittle: MISC_DISPLAY has 8 pages (NULL_PAGE placeholder at [0],
    // plus PTZ_PAGE at [3] whose PTZ_GRP has been reparented to OpticsPanel),
    // so the first six by index do NOT line up with the desired tabs.
    // YOLO_PAGE is skipped — its model lists were relocated to the IMG PROC
    // right-stack MODELS tab (same way PTZ_PAGE's PTZ_GRP went to OpticsPanel).
    struct TabSpec { const char *label; const char *pageName; };
    static const TabSpec specs[] = {
        { "DATA",  "DATA_PAGE"     },
        { "HIST",  "HIST_PAGE"     },
        { "ALT",   "ALT_CTRL_PAGE" },
        { "ADDON", "ADDON_PAGE"    },
        { "VID",   "VID_PAGE"      },
    };

    for (const auto &s : specs) {
        QWidget *page = misc_display->findChild<QWidget*>(
            QString::fromLatin1(s.pageName), Qt::FindDirectChildrenOnly);
        if (!page) continue;
        misc_display->removeWidget(page);
        m_tabs->addTab(page, s.label);
    }

    // 3D placeholder tab — actual point cloud rendering deferred.
    m_tabs->addTab(new QLabel("3D point cloud — placeholder"), "3D");

    adjustSize();
}
