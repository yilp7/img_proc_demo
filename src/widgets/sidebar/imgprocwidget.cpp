#include "imgprocwidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

ImgProcWidget::ImgProcWidget(QWidget *parent)
    : QFrame(parent), m_tabs(nullptr),
      m_models_page(nullptr), m_general_page(nullptr),
      m_auto_follow_check(nullptr)
{
    setObjectName("IMG_PROC_WIDGET");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName("IMG_PROC_TABS");
    m_models_page  = new QFrame(m_tabs);
    m_general_page = new QFrame(m_tabs);
    m_tabs->addTab(m_models_page,  "MODELS");
    m_tabs->addTab(m_general_page, "GENERAL");
    root->addWidget(m_tabs);
}

void ImgProcWidget::setup(QComboBox  *main_model,     QComboBox  *alt1_model,
                          QComboBox  *alt2_model,
                          QLabel     *main_label,     QLabel     *alt1_label,
                          QLabel     *alt2_label,
                          QComboBox  *display_picker,
                          QCheckBox  *enhance_chk,    QComboBox  *enhance_options,
                          QCheckBox  *frame_avg_chk,  QComboBox  *frame_avg_options,
                          QCheckBox  *img_3d_chk,     QLineEdit  *range_thresh_edit,
                          QPushButton *reset_3d_btn,
                          QCheckBox  *pseudocolor_chk)
{
    auto reparent_into = [](QWidget *w, QWidget *p) {
        if (!w || !p) return;
        w->setParent(p);
        w->setGeometry(QRect());  // clear .ui-baked geometry; layout will size it
        w->show();
    };

    // MODELS tab: 3 rows, "label + model selector" per display
    // (MAIN / ALT1 / ALT2). Dropdown items are detection models — currently
    // VL / Thermal / Gated from YOLO_PAGE; future builds discover models
    // dynamically from the local models/ folder (fishnet included).
    auto *ml = new QGridLayout(m_models_page);
    ml->setContentsMargins(6, 6, 6, 6);
    ml->setHorizontalSpacing(6);
    ml->setVerticalSpacing(4);

    int row = 0;
    struct ModelRow { QLabel *label; QComboBox *combo; };
    ModelRow rows[] = {
        { main_label, main_model },
        { alt1_label, alt1_model },
        { alt2_label, alt2_model },
    };
    int idx = 0;
    for (const auto &r : rows) {
        if (!r.combo) { ++idx; continue; }
        if (r.label) {
            reparent_into(r.label, m_models_page);
            ml->addWidget(r.label, row, 0);
        }
        reparent_into(r.combo, m_models_page);
        ml->addWidget(r.combo, row, 1);

        // Created here (not .ui) because the tab is built by reparenting;
        // orphan widgets in YOLO_GRP never render. ID selector outranks the
        // global .QFrame rule in style_dark.qss.
        QFrame *dot = new QFrame(m_models_page);
        dot->setObjectName(QString("MODEL_STATUS_%1").arg(idx));
        dot->setFixedSize(11, 11);
        dot->setStyleSheet(QString(
            "QFrame#MODEL_STATUS_%1 { background-color: #888888;"
            " border: 1px solid #555555; border-radius: 5px; }").arg(idx));
        ml->addWidget(dot, row, 2);
        m_status[idx] = dot;

        ++row;
        ++idx;
    }

    m_auto_follow_check = new QCheckBox(tr("AUTO FOLLOW"), m_models_page);
    m_auto_follow_check->setObjectName("YOLO_AUTO_FOLLOW_CHECK");
    m_auto_follow_check->setChecked(false);
    ml->addWidget(m_auto_follow_check, row, 0, 1, 3);
    connect(m_auto_follow_check, &QCheckBox::toggled,
            this, &ImgProcWidget::auto_follow_changed);
    ++row;

    ml->setRowStretch(row, 1);
    ml->setColumnStretch(1, 1);

    // GENERAL tab: image-processing toggles (enhance / frame-avg / pseudocolor / 3D).
    // Row 0 is the display picker — selects which display's pcfg the controls
    // below it read from and write to.
    auto *gl = new QGridLayout(m_general_page);
    gl->setContentsMargins(6, 6, 6, 6);
    gl->setHorizontalSpacing(6);
    gl->setVerticalSpacing(4);

    int grow = 0;
    if (display_picker) {
        display_picker->setParent(m_general_page);
        display_picker->setGeometry(QRect());
        display_picker->show();
        auto *picker_label = new QLabel("Display", m_general_page);
        gl->addWidget(picker_label,   grow, 0);
        gl->addWidget(display_picker, grow, 1);
        ++grow;
    }
    if (enhance_chk && enhance_options) {
        reparent_into(enhance_chk,     m_general_page);
        reparent_into(enhance_options, m_general_page);
        gl->addWidget(enhance_chk,     grow, 0);
        gl->addWidget(enhance_options, grow, 1);
        ++grow;
    }
    if (frame_avg_chk && frame_avg_options) {
        reparent_into(frame_avg_chk,     m_general_page);
        reparent_into(frame_avg_options, m_general_page);
        gl->addWidget(frame_avg_chk,     grow, 0);
        gl->addWidget(frame_avg_options, grow, 1);
        ++grow;
    }
    if (pseudocolor_chk) {
        reparent_into(pseudocolor_chk, m_general_page);
        gl->addWidget(pseudocolor_chk, grow, 0, 1, 2);
        ++grow;
    }
    if (img_3d_chk) {
        // Single-row 3D layout, faithful to pre-sprint-15 user_panel.ui:
        //   [☐ 3D]   [threshold edit]   [16×16 icon-only RESET_3D_BTN]
        // Reset button is square, no text — icon set by UserPanel via theme.
        reparent_into(img_3d_chk, m_general_page);
        auto *row3d = new QHBoxLayout();
        row3d->setContentsMargins(0, 0, 0, 0);
        row3d->setSpacing(4);
        row3d->addWidget(img_3d_chk);
        if (range_thresh_edit) {
            reparent_into(range_thresh_edit, m_general_page);
            row3d->addWidget(range_thresh_edit, 1);
        }
        if (reset_3d_btn) {
            reparent_into(reset_3d_btn, m_general_page);
            reset_3d_btn->setText(QString());
            reset_3d_btn->setFixedSize(16, 16);
            row3d->addWidget(reset_3d_btn);
        }
        gl->addLayout(row3d, grow, 0, 1, 2);
        ++grow;
    }
    gl->setRowStretch(grow, 1);
    gl->setColumnStretch(1, 1);
}

bool ImgProcWidget::auto_follow_enabled() const
{
    return m_auto_follow_check && m_auto_follow_check->isChecked();
}

void ImgProcWidget::set_model_status(int display_idx, int state)
{
    if (display_idx < 0 || display_idx >= 3) return;
    QFrame *dot = m_status[display_idx];
    if (!dot) return;
    const char *bg, *brd, *tip;
    switch (state) {
    case 1:  bg = "#4CAF50"; brd = "#2E7D32"; tip = "Model running"; break;
    case 2:  bg = "#CD5C5C"; brd = "#8B3A3A"; tip = "Model load failed"; break;
    default: bg = "#888888"; brd = "#555555"; tip = "No model loaded"; break;
    }
    dot->setStyleSheet(QString("QFrame#%1 { background-color: %2;"
                               " border: 1px solid %3; border-radius: 5px; }")
                       .arg(dot->objectName(), bg, brd));
}
