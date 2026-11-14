#ifndef AUXPANELMANAGER_H
#define AUXPANELMANAGER_H

#include <QObject>

class AuxPanelManager : public QObject
{
    Q_OBJECT

public:
    explicit AuxPanelManager(QObject *parent = nullptr);

    int get_alt_display_option() const { return m_alt_display_option; }

public slots:
    void select_display(int combo_index);

signals:
    void display_page_changed(int page);

private:
    int            m_alt_display_option;
};

#endif // AUXPANELMANAGER_H
