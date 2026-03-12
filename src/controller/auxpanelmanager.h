#ifndef AUXPANELMANAGER_H
#define AUXPANELMANAGER_H

#include <QObject>

class QButtonGroup;

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class AuxPanelManager : public QObject
{
    Q_OBJECT

public:
    explicit AuxPanelManager(QObject *parent = nullptr);

    void init(Ui::UserPanel *ui);

    // Getters
    int get_alt_display_option() const { return alt_display_option; }

public slots:
    void on_MISC_RADIO_1_clicked();
    void on_MISC_RADIO_2_clicked();
    void on_MISC_RADIO_3_clicked();
    void on_MISC_OPTION_1_currentIndexChanged(int index);
    void on_MISC_OPTION_2_currentIndexChanged(int index);
    void on_MISC_OPTION_3_currentIndexChanged(int index);

private:
    Ui::UserPanel* m_ui;
    QButtonGroup*  display_grp;
    int            alt_display_option;
};

#endif // AUXPANELMANAGER_H
