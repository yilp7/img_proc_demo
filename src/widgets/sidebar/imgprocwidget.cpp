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

void ImgProcWidget::setup(QCheckBox  *enhance_chk,    QComboBox  *enhance_options,
                          QCheckBox  *frame_avg_chk,  QComboBox  *frame_avg_options,
                          QCheckBox  *img_3d_chk,     QLineEdit  *range_thresh_edit,
                          QPushButton *reset_3d_btn,
                          QCheckBox  *pseudocolor_chk)
{
    // MODELS tab: 3 rows, each is "checkbox + selector/options"
    auto *ml = new QGridLayout(m_models_page);
    ml->setContentsMargins(6, 6, 6, 6);
    ml->setHorizontalSpacing(6);
    ml->setVerticalSpacing(4);

    auto reparent_into = [](QWidget *w, QWidget *p) {
        if (!w || !p) return;
        w->setParent(p);
        w->setGeometry(QRect());  // clear .ui-baked geometry; layout will size it
        w->show();
    };

    int row = 0;
    if (enhance_chk && enhance_options) {
        reparent_into(enhance_chk,     m_models_page);
        reparent_into(enhance_options, m_models_page);
        ml->addWidget(enhance_chk,     row, 0);
        ml->addWidget(enhance_options, row, 1);
        ++row;
    }
    if (frame_avg_chk && frame_avg_options) {
        reparent_into(frame_avg_chk,     m_models_page);
        reparent_into(frame_avg_options, m_models_page);
        ml->addWidget(frame_avg_chk,     row, 0);
        ml->addWidget(frame_avg_options, row, 1);
        ++row;
    }
    if (img_3d_chk) {
        reparent_into(img_3d_chk, m_models_page);
        ml->addWidget(img_3d_chk, row, 0);
        if (range_thresh_edit) {
            reparent_into(range_thresh_edit, m_models_page);
            ml->addWidget(range_thresh_edit, row, 1);
        }
        ++row;
        if (reset_3d_btn) {
            reparent_into(reset_3d_btn, m_models_page);
            ml->addWidget(reset_3d_btn, row, 0, 1, 2);
            ++row;
        }
    }
    ml->setRowStretch(row, 1);
    ml->setColumnStretch(1, 1);

    // GENERAL tab: pseudocolor (more toggles can land here later)
    auto *gl = new QVBoxLayout(m_general_page);
    gl->setContentsMargins(6, 6, 6, 6);
    gl->setSpacing(4);
    if (pseudocolor_chk) {
        reparent_into(pseudocolor_chk, m_general_page);
        gl->addWidget(pseudocolor_chk);
    }
    gl->addStretch(1);
}
