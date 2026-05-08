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

    // Re-parents existing widgets from the .ui into Classic mode page.
    // lens_static  = LENS_STATIC QGroupBox (from RIGHT panel)
    // ptz_grp      = PTZ_GRP QFrame (from LEFT/PARAMS_STATIC/PTZ_PAGE)
    // iris_open, iris_close = deleted per sprint plan (duplicate of radius)
    void setup(QWidget *lens_static, QWidget *ptz_grp,
               QWidget *iris_open, QWidget *iris_close);

    // Set Modern-mode page content (16.8).
    void set_modern_page(QWidget *w);

    enum class Mode { Classic = 0, Modern = 1 };
    Mode current_mode() const;

signals:
    void modeChanged(OpticsPanel::Mode mode);

private slots:
    void on_mode_toggled(int id);

private:
    QStackedWidget* m_stack;
    QWidget*        m_classic_page;
    QWidget*        m_modern_page;
    QButtonGroup*   m_mode_group;
};

#endif // OPTICSPANEL_H
