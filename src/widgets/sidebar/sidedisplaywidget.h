#ifndef SIDEDISPLAYWIDGET_H
#define SIDEDISPLAYWIDGET_H

#include <QFrame>

class Display;
class QLabel;
class QPushButton;
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

    // Re-apply themed icons on the swap buttons. Call from UserPanel::set_theme
    // alongside the other :/tools/<variant>/switch refreshes.
    void apply_theme(bool light_theme);

signals:
    // Emitted when the user clicks the swap-with-main button next to an alt
    // thumbnail. alt_idx is 1 for ALT 1, 2 for ALT 2 — the receiver should
    // call swap_grab_thread_display(0, alt_idx).
    void swap_requested(int alt_idx);

private:
    QVBoxLayout *m_layout;
    QLabel      *m_alt1_label;
    QPushButton *m_alt1_swap;
    QLabel      *m_alt1_thumb;
    QLabel      *m_alt2_label;
    QPushButton *m_alt2_swap;
    QLabel      *m_alt2_thumb;
};

#endif // SIDEDISPLAYWIDGET_H
