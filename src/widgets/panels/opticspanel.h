#ifndef OPTICSPANEL_H
#define OPTICSPANEL_H

#include "widgets/floatingpanel.h"

class QStackedWidget;
class QButtonGroup;

class OpticsPanel : public FloatingPanel
{
    Q_OBJECT
public:
    explicit OpticsPanel(QWidget *parent = nullptr);

    // Re-parents existing widgets into the classic page: lens_static is LENS_STATIC,
    // ptz_grp is PTZ_GRP. The iris buttons duplicate radius and are hidden.
    void setup(QWidget *lens_static, QWidget *ptz_grp,
               QWidget *iris_open, QWidget *iris_close);

    void set_modern_page(QWidget *w);

    enum class Mode { CLASSIC = 0, MODERN = 1 };
    Mode get_current_mode() const;

signals:
    void mode_changed(OpticsPanel::Mode mode);

private slots:
    void on_mode_toggled(int id);

private:
    QStackedWidget *m_stack;
    QWidget        *m_classic_page;
    QWidget        *m_modern_page;
    QButtonGroup   *m_mode_group;
};

#endif // OPTICSPANEL_H
