#ifndef LASERSETTINGS_H
#define LASERSETTINGS_H

#include "utils.h"

namespace Ui {
class LaserSettings;
}

class LaserSettings : public QDialog
{
    Q_OBJECT

public:
    explicit LaserSettings(QWidget *parent = nullptr);
    ~LaserSettings();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void adjustment_btn_pressed(int id);
    void adjustment_btn_released(int id);

signals:
    void com_write(int id, QByteArray cmd);

private:
    Ui::LaserSettings *ui;

    bool              pressed;
    QPoint            prev_pos;

    QButtonGroup      *laser_lens;
    QButtonGroup      *adjustments;
};

#endif // LASERSETTINGS_H
