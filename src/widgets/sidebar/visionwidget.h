#ifndef VISIONWIDGET_H
#define VISIONWIDGET_H

#include <QFrame>

class QCheckBox;
class QLabel;

// Unfinished and not shown in the right stack: detections are drawn on the displays
// and the side display mirrors MAIN. It has one row per channel with an enable
// checkbox; the ALT rows show when their channel is active.
class VisionWidget : public QFrame
{
    Q_OBJECT
public:
    explicit VisionWidget(QWidget *parent = nullptr);

    // Enable / hide ALT rows externally (camera-active driver).
    void set_alt1_visible(bool visible);
    void set_alt2_visible(bool visible);

signals:
    void main_enabled_changed(bool enabled);
    void alt1_enabled_changed(bool enabled);
    void alt2_enabled_changed(bool enabled);

private:
    QFrame *make_row(const QString &key, const QString &label, QCheckBox **out_chk, QFrame *parent);

    QFrame *m_main_row;
    QFrame *m_alt1_row;
    QFrame *m_alt2_row;
    QCheckBox *m_main_chk;
    QCheckBox *m_alt1_chk;
    QCheckBox *m_alt2_chk;
};

#endif // VISIONWIDGET_H
