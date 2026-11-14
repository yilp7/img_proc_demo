#ifndef SIDEDISPLAYWIDGET_H
#define SIDEDISPLAYWIDGET_H

#include <QFrame>

class Display;
class QLabel;
class QPushButton;
class QVBoxLayout;

// Left-side widget that mirrors the ALT1 / ALT2 displays as live thumbnails: a label
// per channel follows Display::set_pixmap and scales the frame. An inactive channel stays dark.
class SideDisplayWidget : public QFrame
{
    Q_OBJECT
public:
    explicit SideDisplayWidget(QWidget *parent = nullptr);

    // Wire the two ALT Display widgets so their pixmaps stream into the
    // thumbnails. Either may be null (slot stays blank).
    void setup(Display *alt1, Display *alt2);

    // Re-apply color-mode icons on the swap buttons. Call from UserPanel::toggle_color_mode
    // alongside the other :/tools/<variant>/switch refreshes.
    void apply_color_mode(bool light_mode);

signals:
    // Emitted by the swap button next to an alt thumbnail. alt_idx is 1 or 2; the
    // receiver calls swap_grab_thread_display(0, alt_idx).
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
