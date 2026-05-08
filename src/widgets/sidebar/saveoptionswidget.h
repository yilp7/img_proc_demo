#ifndef SAVEOPTIONSWIDGET_H
#define SAVEOPTIONSWIDGET_H

#include <QFrame>

class QLineEdit;
class QToolButton;
class QPushButton;
class QLabel;

// Right-stack save-options widget. Layout:
//   row 1: file path edit + browse
//   row 2: "Capture" label | ORI button | RES button
//   row 3: "Record"  label | ORI button | RES button
//
// Re-parents the existing IMG_SAVE_STATIC widgets (no controller changes).
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
