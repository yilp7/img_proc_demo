#ifndef IMGPROCWIDGET_H
#define IMGPROCWIDGET_H

#include <QFrame>

class QTabWidget;
class QCheckBox;
class QComboBox;
class QPushButton;
class QLineEdit;

// Image-processing controls for the right-side stack. Two tabs:
//   MODELS  — enhancement / frame avg / 3D mapping model toggles + selectors
//   GENERAL — pseudocolor and other generic toggles
//
// Re-parents existing widgets out of IMG_PROC_STATIC (no controller code
// changes — widget identity is preserved).
class ImgProcWidget : public QFrame
{
    Q_OBJECT
public:
    explicit ImgProcWidget(QWidget *parent = nullptr);

    // Inputs are existing widgets pulled from IMG_PROC_STATIC.
    void setup(QCheckBox  *enhance_chk,    QComboBox  *enhance_options,
               QCheckBox  *frame_avg_chk,  QComboBox  *frame_avg_options,
               QCheckBox  *img_3d_chk,     QLineEdit  *range_thresh_edit,
               QPushButton *reset_3d_btn,
               QCheckBox  *pseudocolor_chk);

private:
    QTabWidget *m_tabs;
    QFrame     *m_models_page;
    QFrame     *m_general_page;
};

#endif // IMGPROCWIDGET_H
