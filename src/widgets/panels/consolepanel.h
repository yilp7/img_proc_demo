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

    // Pulls the existing console pages out of misc_display and shows them
    // through the classic three-selector console switcher.
    // misc_display = MISC_DISPLAY QStackedWidget (from LEFT panel)
    // After setup(), misc_display and its old parent frame (MISC_DISPLAY_GRP)
    // should be hidden; they are no longer needed.
    void setup(QStackedWidget *misc_display);

    int currentOption() const;

signals:
    void optionSelected(int optionId);

private:
    void populateSelector(MiscSelection *selector, int defaultOption);
    void activateSelector(int selectorIndex);
    void activateOption(int optionId);
    int selectorOption(int selectorIndex) const;

    QButtonGroup  *m_selector_group;
    QStackedWidget *m_stack;
    MiscSelection *m_selectors[3];
    QRadioButton  *m_radios[3];
    QMap<int, int> m_page_index_by_option;
    bool           m_updating;
};

#endif // CONSOLEPANEL_H
