#ifndef FLOATINGWINDOW_H
#define FLOATINGWINDOW_H

#include "display.h"

class FloatingWindow : public QWidget
{
    Q_OBJECT
public:
    explicit FloatingWindow();

    Display* get_display_widget();
    void resize_display(int width, int height);

protected:
    //@override
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    // ui-control
    bool            pressed;
    QPoint          prev_pos;
    QElapsedTimer   timer_mouse;

private:
    Display* disp;
    QFrame*  frame;

    int      w, h;
};

#endif // FLOATINGWINDOW_H
