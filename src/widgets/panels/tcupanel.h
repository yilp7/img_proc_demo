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

    // Re-parents existing widgets: tcu_static (TCU_STATIC) goes into the Guru page;
    // laser_btn and fire_btn stay visible in both modes.
    void setup(QWidget *tcu_static, QWidget *laser_btn, QWidget *fire_btn);

    void set_normal_page(QWidget *w);

    enum class Mode { NORMAL = 0, GURU = 1 };
    Mode get_current_mode() const;

signals:
    void mode_changed(TCUPanel::Mode mode);

private slots:
    void on_mode_toggled(int id);

private:
    QWidget        *m_fire_row;     // always-visible laser buttons
    QStackedWidget *m_stack;
    QWidget        *m_normal_page;
    QWidget        *m_guru_page;
    QButtonGroup   *m_mode_group;
};

#endif // TCUPANEL_H
