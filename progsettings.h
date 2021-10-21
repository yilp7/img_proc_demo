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

public:
    int   start_pos;
    int   end_pos;
    int   frame_count;
    float step_size;

private:
    Ui::ProgSettings *ui;

};

#endif // PROGSETTINGS_H
