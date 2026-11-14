#ifndef PANORAMAPANEL_H
#define PANORAMAPANEL_H

#include "widgets/floatingpanel.h"

#include <QPoint>
#include <QPixmap>
#include <QRect>

class QEvent;
class QLabel;
class QObject;
class QPushButton;
class QScrollArea;
class QResizeEvent;

class PanoramaPanel : public FloatingPanel
{
    Q_OBJECT
public:
    explicit PanoramaPanel(QWidget *parent = nullptr);

public slots:
    void clear_preview();
    void set_preview_pixmap(const QPixmap &pixmap);
    void set_generating();
    void set_generated();

signals:
    void resized_by_user();
    void reset_requested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPoint event_point_in_viewport(QObject *obj, const QPoint &pos) const;
    void reset_view();
    void update_preview_pixmap();

    QScrollArea *m_scroll;
    QLabel      *m_preview;
    QLabel      *m_status;
    QPushButton *m_reset_button;
    QWidget     *m_resize_handle;
    QPixmap      m_source_pixmap;
    bool         m_interactive;
    bool         m_panning;
    bool         m_resizing;
    double       m_zoom;
    QPoint       m_pan;
    QPoint       m_last_pan_pos;
    QPoint       m_resize_press_global;
    QRect        m_resize_press_geometry;
};

#endif // PANORAMAPANEL_H
