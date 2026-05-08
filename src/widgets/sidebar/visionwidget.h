#ifndef VISIONWIDGET_H
#define VISIONWIDGET_H

#include <QFrame>

class QCheckBox;
class QLabel;

// Right-stack vision widget. 3 channel rows (MAIN / ALT1 / ALT2). Each row
// has an enable checkbox and a placeholder slot for future detection
// configuration (threshold slider etc.). ALT rows auto-hide when their
// enable checkbox is off — until wired up to camera-active state in a
// later sprint, the toggle is the only visibility driver.
//
// Detection model selection still lives in the Console panel's YOLO tab
// (re-parented in 16.7). This widget is the *control surface*, not the model
// picker.
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
    QFrame *makeRow(const QString &label, QCheckBox **out_chk, QFrame *parent);

    QFrame *m_main_row;
    QFrame *m_alt1_row;
    QFrame *m_alt2_row;
    QCheckBox *m_main_chk;
    QCheckBox *m_alt1_chk;
    QCheckBox *m_alt2_chk;
};

#endif // VISIONWIDGET_H
