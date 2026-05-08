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
    // Tab names matching the prototype and the old radio labels
    static const char *names[] = { "DATA", "HIST", "ALT", "ADDON", "VID", "YOLO" };
    const int count = qMin(misc_display->count(), 6);

    // Extract pages front-to-back. After the first removeWidget the indices shift,
    // so always take widget(0) after removing it.
    QList<QWidget*> pages;
    for (int i = 0; i < count; ++i)
        pages.append(misc_display->widget(0)),
        misc_display->removeWidget(misc_display->widget(0));

    for (int i = 0; i < pages.size(); ++i)
        m_tabs->addTab(pages[i], names[i]);

    // 3D placeholder tab
    m_tabs->addTab(new QLabel("3D point cloud — placeholder"), "3D");

    adjustSize();
}
