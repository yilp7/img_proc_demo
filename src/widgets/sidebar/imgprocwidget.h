#ifndef IMGPROCWIDGET_H
#define IMGPROCWIDGET_H

#include <QFrame>

class QTabWidget;
class QCheckBox;
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;

// Image processing controls of the right stack, in two tabs: MODELS (a model
// selector per display) and GENERAL (enhance, frame average, pseudocolor, 3D).
// It re-parents existing widgets, so their controller and Config wiring is unchanged.
class ImgProcWidget : public QFrame
{
    Q_OBJECT
public:
    explicit ImgProcWidget(QWidget *parent = nullptr);

    // MODELS takes three model selectors with their labels. GENERAL takes the display
    // picker, created by UserPanel so it keeps the connection, and the per-display controls.
    void setup(QComboBox  *main_model,     QComboBox  *alt1_model,
               QComboBox  *alt2_model,
               QLabel     *main_label,     QLabel     *alt1_label,
               QLabel     *alt2_label,
               QComboBox  *display_picker,
               QCheckBox  *enhance_chk,    QComboBox  *enhance_options,
               QCheckBox  *frame_avg_chk,  QComboBox  *frame_avg_options,
               QCheckBox  *img_3d_chk,     QLineEdit  *range_thresh_edit,
               QPushButton *reset_3d_btn,
               QCheckBox  *pseudocolor_chk);

    // display_idx: 0=MAIN, 1=ALT1, 2=ALT2.
    // state: 0=none (gray), 1=loaded (green), 2=failed (red).
    void set_model_status(int display_idx, int state);
    bool is_auto_follow_enabled() const;

signals:
    void auto_follow_changed(bool enabled);

private:
    QTabWidget *m_tabs;
    QFrame     *m_models_page;
    QFrame     *m_general_page;
    QCheckBox  *m_auto_follow_check;
    QFrame     *m_status[3] = {nullptr, nullptr, nullptr};
};

#endif // IMGPROCWIDGET_H
