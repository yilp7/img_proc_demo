#ifndef SAVEOPTIONSWIDGET_H
#define SAVEOPTIONSWIDGET_H

#include <QFrame>

class QLineEdit;
class QToolButton;
class QPushButton;
class QLabel;

// Save options of the right stack: a path row, then capture and record rows with
// ORI and RES buttons. Re-parents the existing IMG_SAVE_STATIC widgets.
class SaveOptionsWidget : public QFrame
{
    Q_OBJECT
public:
    explicit SaveOptionsWidget(QWidget *parent = nullptr);

    void setup(QLineEdit  *file_path_edit,
               QToolButton *file_path_browse,
               QPushButton *save_bmp_btn,    // capture ORI
               QPushButton *save_final_btn,  // capture RES
               QPushButton *save_avi_btn,    // record  ORI
               QPushButton *save_result_btn, // record  RES
               QLabel      *capture_label,
               QLabel      *record_label);
};

#endif // SAVEOPTIONSWIDGET_H
