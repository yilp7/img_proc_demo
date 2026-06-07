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
    void clearPreview();
    void setPreviewPixmap(const QPixmap &pixmap);
    void setGenerating();
    void setGenerated();

signals:
    void resizedByUser();
    void resetRequested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPoint eventPointInViewport(QObject *obj, const QPoint &pos) const;
    void resetView();
    void updatePreviewPixmap();

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
