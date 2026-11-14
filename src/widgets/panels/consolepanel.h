#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include "widgets/floatingpanel.h"

#include <QMap>

class QButtonGroup;
class QRadioButton;
class QStackedWidget;
class MiscSelection;

class ConsolePanel : public FloatingPanel
{
    Q_OBJECT
public:
    explicit ConsolePanel(QWidget *parent = nullptr);

    // Takes the console pages out of misc_display (the MISC_DISPLAY stacked widget)
    // and shows them through its own three-selector switcher. The caller hides
    // misc_display and its frame afterwards.
    void setup(QStackedWidget *misc_display);

    int get_current_option() const;

signals:
    void option_selected(int option_id);

private:
    void populate_selector(MiscSelection *selector, int default_option);
    void activate_selector(int selector_index);
    void activate_option(int option_id);
    int get_selector_option(int selector_index) const;

    QButtonGroup  *m_selector_group;
    QStackedWidget *m_stack;
    MiscSelection *m_selectors[3];
    QRadioButton  *m_radios[3];
    QMap<int, int> m_page_index_by_option;
    bool           m_updating;
};

#endif // CONSOLEPANEL_H
