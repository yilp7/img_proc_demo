#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include "widgets/floatingpanel.h"

class QTabWidget;
class QStackedWidget;

class ConsolePanel : public FloatingPanel
{
    Q_OBJECT
public:
    explicit ConsolePanel(QWidget *parent = nullptr);

    // Pulls the 6 existing pages out of misc_display and adds them as tabs.
    // misc_display = MISC_DISPLAY QStackedWidget (from LEFT panel)
    // After setup(), misc_display and its old parent frame (MISC_DISPLAY_GRP)
    // should be hidden — they are no longer needed.
    void setup(QStackedWidget *misc_display);

    QTabWidget* tab_widget() const { return m_tabs; }

private:
    QTabWidget* m_tabs;
};

#endif // CONSOLEPANEL_H
