#ifndef VISIONWIDGET_H
#define VISIONWIDGET_H

#include <QFrame>

class QCheckBox;
class QLabel;

// UNDER CONSTRUCTION — currently not shown in the right stack.
//
// As of Sprint 23 the right stack only shows IMG PROC + Save Options.
// Detection RESULTS are drawn as overlays on the displays themselves
// (draw_yolo_boxes at pipelineprocessor.cpp:606), and Side Display already
// mirrors MAIN, so a plain thumbnail/result panel here would be redundant.
//
// Kept around for a future role — candidates:
//   - per-channel inference enable toggle (skip YOLO on a display to save GPU)
//   - per-channel confidence-threshold override
//   - class filter (which detected classes get drawn / counted)
//   - detection statistics tally
//
// Current shape: 3 channel rows (MAIN / ALT1 / ALT2), each with an enable
// checkbox. ALT rows hidden by default; setAlt1Visible / setAlt2Visible
// expose them when an ALT channel is active.
class VisionWidget : public QFrame
{
    Q_OBJECT
public:
    explicit VisionWidget(QWidget *parent = nullptr);

    // Enable / hide ALT rows externally (camera-active driver).
    void setAlt1Visible(bool visible);
    void setAlt2Visible(bool visible);

signals:
    void mainEnabledChanged(bool enabled);
    void alt1EnabledChanged(bool enabled);
    void alt2EnabledChanged(bool enabled);

private:
    QFrame *makeRow(const QString &key, const QString &label, QCheckBox **out_chk, QFrame *parent);

    QFrame *m_main_row;
    QFrame *m_alt1_row;
    QFrame *m_alt2_row;
    QCheckBox *m_main_chk;
    QCheckBox *m_alt1_chk;
    QCheckBox *m_alt2_chk;
};

#endif // VISIONWIDGET_H
