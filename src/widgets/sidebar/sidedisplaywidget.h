#ifndef SIDEDISPLAYWIDGET_H
#define SIDEDISPLAYWIDGET_H

#include <QFrame>

class Display;
class QLabel;
class QVBoxLayout;

// Left-side widget that mirrors the ALT1 / ALT2 displays as live thumbnails.
// Same pattern used by the SETUP / SCAN preview sidebars: a QLabel per
// channel that listens to the corresponding Display::set_pixmap signal and
// scales the pixmap to fit. When a channel isn't active no pixmap arrives
// and the slot stays dark.
class SideDisplayWidget : public QFrame
{
    Q_OBJECT
public:
    explicit SideDisplayWidget(QWidget *parent = nullptr);

    // Wire the two ALT Display widgets so their pixmaps stream into the
    // thumbnails. Either may be null (slot stays blank).
    void setup(Display *alt1, Display *alt2);

private:
    QVBoxLayout *m_layout;
    QLabel      *m_alt1_label;
    QLabel      *m_alt1_thumb;
    QLabel      *m_alt2_label;
    QLabel      *m_alt2_thumb;
};

#endif // SIDEDISPLAYWIDGET_H
