#include "saveoptionswidget.h"

#include <QAbstractButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>

SaveOptionsWidget::SaveOptionsWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("SAVE_OPTIONS_WIDGET");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(92);
}

void SaveOptionsWidget::setup(QLineEdit  *file_path_edit,
                              QToolButton *file_path_browse,
                              QPushButton *save_bmp_btn,
                              QPushButton *save_final_btn,
                              QPushButton *save_avi_btn,
                              QPushButton *save_result_btn,
                              QLabel      *capture_label,
                              QLabel      *record_label)
{
    auto reparent_into = [](QWidget *w, QWidget *p) {
        if (!w || !p) return;
        w->setParent(p);
        w->setGeometry(QRect());
        w->show();
    };
    constexpr int row_h = 22;
    auto prep_button = [row_h](QAbstractButton *btn) {
        if (!btn) return;
        btn->setFixedHeight(row_h);
        btn->setMinimumWidth(0);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    };
    auto prep_label = [row_h](QLabel *label) {
        if (!label) return;
        label->setWordWrap(false);
        label->setFixedHeight(row_h);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    };

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    // Row 1: path edit + browse
    auto *path_row = new QHBoxLayout;
    path_row->setContentsMargins(0, 0, 0, 0);
    path_row->setSpacing(4);
    if (file_path_edit) {
        reparent_into(file_path_edit, this);
        file_path_edit->setFixedHeight(row_h);
        file_path_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        path_row->addWidget(file_path_edit, 1);
    }
    if (file_path_browse) {
        reparent_into(file_path_browse, this);
        prep_button(file_path_browse);
        path_row->addWidget(file_path_browse);
    }
    root->addLayout(path_row);

    // Rows 2-3: capture / record grid
    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(4);
    grid->setVerticalSpacing(4);
    grid->setColumnMinimumWidth(0, 58);
    grid->setRowMinimumHeight(0, row_h);
    grid->setRowMinimumHeight(1, row_h);

    if (capture_label) {
        reparent_into(capture_label, this);
        prep_label(capture_label);
        grid->addWidget(capture_label, 0, 0);
    }
    if (save_bmp_btn) {
        reparent_into(save_bmp_btn, this);
        prep_button(save_bmp_btn);
        grid->addWidget(save_bmp_btn, 0, 1);
    }
    if (save_final_btn) {
        reparent_into(save_final_btn, this);
        prep_button(save_final_btn);
        grid->addWidget(save_final_btn, 0, 2);
    }
    if (record_label) {
        reparent_into(record_label, this);
        prep_label(record_label);
        grid->addWidget(record_label, 1, 0);
    }
    if (save_avi_btn) {
        reparent_into(save_avi_btn, this);
        prep_button(save_avi_btn);
        grid->addWidget(save_avi_btn, 1, 1);
    }
    if (save_result_btn) {
        reparent_into(save_result_btn, this);
        prep_button(save_result_btn);
        grid->addWidget(save_result_btn, 1, 2);
    }
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    root->addLayout(grid);
}
