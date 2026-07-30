#ifndef TOPBAR_H
#define TOPBAR_H

#include "util/util.h"
#include "widgets/dotindicator.h"

class TopButton : public QPushButton
{
    Q_OBJECT
public:
    explicit TopButton(QString icon, QWidget *parent);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
};

class TopBar : public QFrame
{
    Q_OBJECT
public:
    explicit TopBar(QWidget *parent = nullptr);
    void setup(QObject *ptr);
    void apply_color_mode(bool light_mode);

    enum class Mode { Setup, Monitor, Scan };
    Mode current_mode() const { return m_current_mode; }

public slots:
    void process_maximize();
    void set_pixel_depth(int depth);
    void set_resolution(int w, int h);
    void cancel_drag() { m_pressed = false; }

signals:
    void modeChanged(TopBar::Mode mode);
    void systemChanged(int idx);

public:
    // left slot — status dots + readouts (16.3)
    DotIndicator* cam_dot()   const { return m_cam_dot; }
    DotIndicator* tcu_dot()   const { return m_tcu_dot; }
    DotIndicator* lens_dot()  const { return m_lens_dot; }
    DotIndicator* laser_dot() const { return m_laser_dot; }
    DotIndicator* ptz_dot()   const { return m_ptz_dot; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

public:
    // right slot - action cluster + window controls
    TopButton* addon;
    TopButton* url;
    TopButton* settings;
    TopButton* capture;
    TopButton* cls;
    TopButton* lang;
    TopButton* color_mode;
    TopButton* min;
    TopButton* max;
    TopButton* exit;

    bool is_maximized;

protected:
    // layout slots — populated by 16.3 (left) and 16.4 (center)
    QFrame* m_left_slot;
    QFrame* m_center_slot;
    QFrame* m_right_slot;
    QFrame* m_window_controls;

private:
    void sync_window_control_state();

    // center slot — mode tabs, system selector, clock
    QButtonGroup* m_mode_group;
    QComboBox*    m_system_selector;
    QTimer*       m_clock_timer;
    QLabel*       m_clock_label;
    Mode          m_current_mode;

    // status dots (left slot)
    DotIndicator* m_cam_dot;
    DotIndicator* m_tcu_dot;
    DotIndicator* m_lens_dot;
    DotIndicator* m_laser_dot;
    DotIndicator* m_ptz_dot;
    // readout labels (left slot)
    QLabel* m_pixel_depth_label;
    QLabel* m_resolution_label;

    bool     m_pressed;
    bool     m_light_mode;
    QPoint   m_prev_pos;
    QRect    m_normal_geom;
    QObject* m_signal_receiver;
};

#endif // TOPBAR_H
