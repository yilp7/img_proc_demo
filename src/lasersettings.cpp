#include "lasersettings.h"
#include "ui_laser_settings.h"

LaserSettings::LaserSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaserSettings),
    pressed(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    laser_lens = new QButtonGroup(this);
    laser_lens->addButton(ui->LASER_RADIO_1, 1);
    laser_lens->addButton(ui->LASER_RADIO_2, 2);
    laser_lens->setExclusive(true);
    ui->LASER_RADIO_1->setChecked(true);

    adjustments = new QButtonGroup(this);
    adjustments->addButton(ui->ENLARGE_BTN, 0);
    adjustments->addButton(ui->NARROW_BTN, 1);
    adjustments->addButton(ui->LEFT_BTN, 2);
    adjustments->addButton(ui->RIGHT_BTN, 3);
    adjustments->addButton(ui->UP_BTN, 4);
    adjustments->addButton(ui->DOWN_BTN, 5);
    adjustments->setExclusive(true);
    connect(adjustments, SIGNAL(buttonPressed(int)), this, SLOT(adjustment_btn_pressed(int)));
    connect(adjustments, SIGNAL(buttonReleased(int)), this, SLOT(adjustment_btn_released(int)));
}

LaserSettings::~LaserSettings()
{
    delete ui;
}

void LaserSettings::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();

    QDialog::mousePressEvent(event);
}

void LaserSettings::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed) {
        // use globalPos instead of pos to prevent window shaking
        window()->move(window()->pos() + event->globalPos() - prev_pos);
        prev_pos = event->globalPos();
    }
    else {
        setCursor(cursor_curr_pointer);
    }
    QDialog::mouseMoveEvent(event);
}

void LaserSettings::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = false;

    QDialog::mouseReleaseEvent(event);
}

void LaserSettings::adjustment_btn_pressed(int id)
{
    int grp = laser_lens->checkedId();
    QString send_str;
    bool ok;
    QByteArray cmd(7, 0x00);
    switch(id) {
    case 0:
        send_str = grp == 1 ? "FF 01 00 10 00 1F 30" : "FF 02 00 10 00 18 2A"; break;
    case 1:
        send_str = grp == 1 ? "FF 01 00 08 00 1F 28" : "FF 02 00 08 00 18 22"; break;
    case 2:
        send_str = grp == 1 ? "FF 01 02 00 00 1F 22" : "FF 02 20 00 00 18 3A"; break;
    case 3:
        send_str = grp == 1 ? "FF 01 04 00 00 1F 24" : "FF 02 40 00 00 18 5A"; break;
    case 4:
        send_str = grp == 1 ? "FF 01 20 00 00 1F 40" : "FF 02 02 00 00 18 1C"; break;
    case 5:
        send_str = grp == 1 ? "FF 01 40 00 00 1F 60" : "FF 02 04 00 00 18 1E"; break;
    default: break;
    }

    send_str.replace(" ", "");
    for (int i = 0; i < send_str.length() / 2; i++) cmd[i] = send_str.mid(i * 2, 2).toInt(&ok, 16);
    // TODO finishup share port settings
    emit com_write(2, cmd); // 2 for lens' port
//    emit com_write(0, cmd); // 0 for lens' port with TCU share
}

void LaserSettings::adjustment_btn_released(int id)
{
    QString send_str;
    bool ok;
    QByteArray cmd(7, 0x00);
    switch (laser_lens->checkedId()) {
    case 1: send_str = "FF 01 00 00 00 00 01"; break;
    case 2: send_str = "FF 02 00 00 00 00 02"; break;
    default: break;
    }

    send_str.replace(" ", "");
    for (int i = 0; i < send_str.length() / 2; i++) cmd[i] = send_str.mid(i * 2, 2).toInt(&ok, 16);
    // TODO finishup share port settings
    emit com_write(2, cmd); // 2 for lens' port
//    emit com_write(0, cmd); // 0 for lens' port with TUC share
}
