#ifndef ADDONPANEL_H
#define ADDONPANEL_H

#include "widgets/floatingpanel.h"

#include <QPoint>
#include <QRect>

class QEvent;
class QResizeEvent;

class AddonPanel : public FloatingPanel
{
    Q_OBJECT
public:
    explicit AddonPanel(QWidget *parent = nullptr);

    QWidget *get_plugin_host() const { return m_host; }
    void fit_plugin_children();

signals:
    void resized_by_user();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *m_host;
    QWidget *m_resize_handle;
    bool     m_resizing;
    QPoint   m_resize_press_global;
    QRect    m_resize_press_geometry;
};

#endif // ADDONPANEL_H
