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
      m_models_page(nullptr), m_general_page(nullptr)
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
    for (const auto &r : rows) {
        if (!r.combo) continue;
        if (r.label) {
            reparent_into(r.label, m_models_page);
            ml->addWidget(r.label, row, 0);
        }
        reparent_into(r.combo, m_models_page);
        ml->addWidget(r.combo, row, 1);
        ++row;
    }
    ml->setRowStretch(row, 1);
    ml->setColumnStretch(1, 1);

    // GENERAL tab: image-processing toggles (enhance / frame-avg / pseudocolor / 3D).
    auto *gl = new QGridLayout(m_general_page);
    gl->setContentsMargins(6, 6, 6, 6);
    gl->setHorizontalSpacing(6);
    gl->setVerticalSpacing(4);

    int grow = 0;
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
        reparent_into(img_3d_chk, m_general_page);
        gl->addWidget(img_3d_chk, grow, 0);
        if (range_thresh_edit) {
            reparent_into(range_thresh_edit, m_general_page);
            gl->addWidget(range_thresh_edit, grow, 1);
        }
        ++grow;
        if (reset_3d_btn) {
            reparent_into(reset_3d_btn, m_general_page);
            gl->addWidget(reset_3d_btn, grow, 0, 1, 2);
            ++grow;
        }
    }
    gl->setRowStretch(grow, 1);
    gl->setColumnStretch(1, 1);
}
