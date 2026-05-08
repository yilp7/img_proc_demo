#ifndef SIDEDISPLAYWIDGET_H
#define SIDEDISPLAYWIDGET_H

#include <QFrame>

class Display;
class QLabel;
class QVBoxLayout;

// Fixed-position secondary camera display on the left side of the main
// window. Holds a Display* re-parented from the existing FloatingWindow.
// Auto-hide when the second camera isn't active is driven externally —
// this widget exposes setActive() and renders a placeholder label
// otherwise.
class SideDisplayWidget : public QFrame
{
    Q_OBJECT
public:
    explicit SideDisplayWidget(QWidget *parent = nullptr);

    // Re-parents the given display into this widget. Pass nullptr to clear.
    void setDisplay(Display *display);

    // Show the live display (true) or the placeholder (false).
    void setActive(bool active);

    bool isActive() const { return m_active; }

private:
    QVBoxLayout *m_layout;
    QLabel      *m_placeholder;
    Display     *m_display;
    bool         m_active;
};

#endif // SIDEDISPLAYWIDGET_H
