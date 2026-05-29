#ifndef IMGPROCWIDGET_H
#define IMGPROCWIDGET_H

#include <QFrame>

class QTabWidget;
class QCheckBox;
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;

// Image-processing controls for the right-side stack. Two tabs:
//   MODELS  — per-display model selectors (MAIN / ALT1 / ALT2). The
//             dropdown items are detection models (currently VL / Thermal /
//             Gated; future builds add fishnet etc. discovered from the
//             local models/ folder).
//   GENERAL — image-processing toggles: enhance, frame-avg, pseudocolor, 3D.
//
// Re-parents existing widgets out of YOLO_PAGE (model lists) and
// IMG_PROC_STATIC (processing toggles). Widget identity preserved, all
// existing controller / Config wiring still works.
class ImgProcWidget : public QFrame
{
    Q_OBJECT
public:
    explicit ImgProcWidget(QWidget *parent = nullptr);

    // MODELS inputs: 3 model selectors + labels (from YOLO_PAGE / YOLO_GRP).
    // GENERAL inputs: a display picker combobox at row 0 (created by
    // UserPanel and passed in so UserPanel keeps the signal connection)
    // and the per-display processing controls underneath.
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

private:
    QTabWidget *m_tabs;
    QFrame     *m_models_page;
    QFrame     *m_general_page;
    QFrame     *m_status[3] = {nullptr, nullptr, nullptr};
};

#endif // IMGPROCWIDGET_H
