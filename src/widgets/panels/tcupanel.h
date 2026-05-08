#ifndef TCUPANEL_H
#define TCUPANEL_H

#include "widgets/floatingpanel.h"

class QStackedWidget;
class QButtonGroup;
class QPushButton;

class TCUPanel : public FloatingPanel
{
    Q_OBJECT
public:
    explicit TCUPanel(QWidget *parent = nullptr);

    // Re-parents existing widgets.
    // tcu_static   = TCU_STATIC QGroupBox (from RIGHT panel) — goes into Guru page
    // laser_btn    = LASER_BTN QPushButton (from LASER_STATIC) — always visible
    // fire_btn     = FIRE_LASER_BTN QPushButton (from LASER_STATIC) — always visible
    void setup(QWidget *tcu_static, QWidget *laser_btn, QWidget *fire_btn);

    // Set Normal-mode page content (16.8).
    void set_normal_page(QWidget *w);

    enum class Mode { Normal = 0, Guru = 1 };
    Mode current_mode() const;

signals:
    void modeChanged(TCUPanel::Mode mode);

private slots:
    void on_mode_toggled(int id);

private:
    QWidget*        m_fire_row;     // always-visible laser buttons
    QStackedWidget* m_stack;
    QWidget*        m_normal_page;
    QWidget*        m_guru_page;
    QButtonGroup*   m_mode_group;
};

#endif // TCUPANEL_H
