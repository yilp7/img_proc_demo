#include "saveoptionswidget.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

SaveOptionsWidget::SaveOptionsWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("SAVE_OPTIONS_WIDGET");
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

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    // Row 1: path edit + browse
    auto *path_row = new QHBoxLayout;
    path_row->setSpacing(4);
    if (file_path_edit) {
        reparent_into(file_path_edit, this);
        path_row->addWidget(file_path_edit, 1);
    }
    if (file_path_browse) {
        reparent_into(file_path_browse, this);
        path_row->addWidget(file_path_browse);
    }
    root->addLayout(path_row);

    // Rows 2-3: capture / record grid
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(4);
    grid->setVerticalSpacing(4);

    if (capture_label) {
        reparent_into(capture_label, this);
        grid->addWidget(capture_label, 0, 0);
    }
    if (save_bmp_btn) {
        reparent_into(save_bmp_btn, this);
        grid->addWidget(save_bmp_btn, 0, 1);
    }
    if (save_final_btn) {
        reparent_into(save_final_btn, this);
        grid->addWidget(save_final_btn, 0, 2);
    }
    if (record_label) {
        reparent_into(record_label, this);
        grid->addWidget(record_label, 1, 0);
    }
    if (save_avi_btn) {
        reparent_into(save_avi_btn, this);
        grid->addWidget(save_avi_btn, 1, 1);
    }
    if (save_result_btn) {
        reparent_into(save_result_btn, this);
        grid->addWidget(save_result_btn, 1, 2);
    }
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    root->addLayout(grid);
}
