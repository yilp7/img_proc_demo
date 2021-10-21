#ifndef PROGSETTINGS_H
#define PROGSETTINGS_H

#include <QDialog>
#include <QKeyEvent>

namespace Ui {
class ProgSettings;
}

class ProgSettings : public QDialog
{
    Q_OBJECT

public:
    explicit ProgSettings(QWidget *parent = nullptr);
    ~ProgSettings();

    void data_exchange(bool read);

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void update_scan();

private:
    Ui::ProgSettings *ui;

public:
    int   start_pos;
    int   end_pos;
    int   frame_count;
    float step_size;

    int   kernel;
    float gamma;
    float log;
    float low_in;
    float high_in;
    float low_out;
    float high_out;

};

#endif // PROGSETTINGS_H
