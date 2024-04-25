#include "lasercontrol.h"
#include "ui_laser_control.h"

LaserControl::LaserControl(QWidget *parent, Lens *p_lens) :
    QDialog(parent),
    ui(new Ui::LaserControl),
    pressed(false),
    p_lens(p_lens),
    laser_control(nullptr),
    speed{32, 32, 32, 32}
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    set_theme();

    laser_control = new QButtonGroup(this);
    laser_control->addButton(ui->BTN_CONTROL_A_1, 0);
    laser_control->addButton(ui->BTN_CONTROL_A_2, 1);
    laser_control->addButton(ui->BTN_CONTROL_B_1, 2);
    laser_control->addButton(ui->BTN_CONTROL_B_2, 3);
    laser_control->addButton(ui->BTN_CONTROL_C_1, 4);
    laser_control->addButton(ui->BTN_CONTROL_C_2, 5);
    laser_control->addButton(ui->BTN_CONTROL_D_1, 6);
    laser_control->addButton(ui->BTN_CONTROL_D_2, 7);
    connect(laser_control, SIGNAL(buttonPressed(int)), this, SLOT(btn_control_pressed(int)));
    connect(laser_control, SIGNAL(buttonReleased(int)), this, SLOT(btn_control_released(int)));

    ui->SLIDER_SPEED_1->setRange(1, 64);
    ui->SLIDER_SPEED_2->setRange(1, 64);
    ui->SLIDER_SPEED_3->setRange(1, 64);
    ui->SLIDER_SPEED_4->setRange(1, 64);
    connect(ui->SLIDER_SPEED_1, &QSlider::valueChanged, this, [this](int val) { ui->EDT_SPEED_1->setText(QString::number(speed[0] = val)); });
    connect(ui->SLIDER_SPEED_2, &QSlider::valueChanged, this, [this](int val) { ui->EDT_SPEED_2->setText(QString::number(speed[1] = val)); });
    connect(ui->SLIDER_SPEED_3, &QSlider::valueChanged, this, [this](int val) { ui->EDT_SPEED_3->setText(QString::number(speed[2] = val)); });
    connect(ui->SLIDER_SPEED_4, &QSlider::valueChanged, this, [this](int val) { ui->EDT_SPEED_4->setText(QString::number(speed[3] = val)); });
    ui->SLIDER_SPEED_1->setValue(32);
    ui->SLIDER_SPEED_2->setValue(32);
    ui->SLIDER_SPEED_3->setValue(32);
    ui->SLIDER_SPEED_4->setValue(32);
    connect(ui->EDT_SPEED_1, &QLineEdit::editingFinished, this,
            [this]() {
                speed[0] = std::min(std::max(ui->EDT_SPEED_1->text().toInt(), 1), 64);
                ui->SLIDER_SPEED_1->setValue(speed[0]);
                ui->EDT_SPEED_1->setText(QString::number(speed[0]));
                if(!this->focusWidget()) return;
                this->focusWidget()->clearFocus();
            });
    connect(ui->EDT_SPEED_2, &QLineEdit::editingFinished, this,
            [this]() {
                speed[1] = std::min(std::max(ui->EDT_SPEED_2->text().toInt(), 1), 64);
                ui->SLIDER_SPEED_2->setValue(speed[1]);
                ui->EDT_SPEED_3->setText(QString::number(speed[1]));
                if(!this->focusWidget()) return;
                this->focusWidget()->clearFocus();
            });
    connect(ui->EDT_SPEED_3, &QLineEdit::editingFinished, this,
            [this]() {
                speed[2] = std::min(std::max(ui->EDT_SPEED_3->text().toInt(), 1), 64);
                ui->SLIDER_SPEED_3->setValue(speed[2]);
                ui->EDT_SPEED_3->setText(QString::number(speed[2]));
                if(!this->focusWidget()) return;
                this->focusWidget()->clearFocus();
            });
    connect(ui->EDT_SPEED_4, &QLineEdit::editingFinished, this,
            [this]() {
                speed[3] = std::min(std::max(ui->EDT_SPEED_4->text().toInt(), 1), 64);
                ui->SLIDER_SPEED_4->setValue(speed[3]);
                ui->EDT_SPEED_4->setText(QString::number(speed[3]));
                if(!this->focusWidget()) return;
                this->focusWidget()->clearFocus();
            });

    connect(this, SIGNAL(send_lens_msg_addr(qint32, uchar, uchar)), p_lens, SLOT(lens_control_addr(qint32, uchar, uchar)));
}

LaserControl::~LaserControl()
{
    delete ui;
}

void LaserControl::set_theme()
{
    ui->BTN_CONTROL_A_1->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_1"));
    ui->BTN_CONTROL_A_2->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_2"));
    ui->BTN_CONTROL_B_1->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_1"));
    ui->BTN_CONTROL_B_2->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_2"));
    ui->BTN_CONTROL_C_1->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_1"));
    ui->BTN_CONTROL_C_2->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_2"));
    ui->BTN_CONTROL_D_1->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_1"));
    ui->BTN_CONTROL_D_2->setIcon(QIcon(":/lens/" + QString(app_theme ? "light" : "dark") + "/iris_2"));
}

void LaserControl::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();

    QDialog::mousePressEvent(event);
}

void LaserControl::mouseMoveEvent(QMouseEvent *event)
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

void LaserControl::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = false;

    QDialog::mouseReleaseEvent(event);
}

#if 0
void LaserControl::adjustment_btn_pressed(int id)
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

void LaserControl::adjustment_btn_released(int id)
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
#endif

void LaserControl::btn_control_pressed(int id)
{
    qint32 control_command;
    switch (id) {
    case 0:  control_command = Lens::RADIUS_UP;   break;
    case 1:  control_command = Lens::RADIUS_DOWN; break;
    case 2:  control_command = Lens::ZOOM_IN;     break;
    case 3:  control_command = Lens::ZOOM_OUT;    break;
    case 4:  control_command = Lens::FOCUS_FAR;   break;
    case 5:  control_command = Lens::FOCUS_NEAR;  break;
    case 6:  control_command = Lens::RADIUS_UP;   break;
    case 7:  control_command = Lens::RADIUS_DOWN; break;
    default: control_command = Lens::STOP;        break;
    }

    emit send_lens_msg_addr(control_command, id > 1 ? 0x02: 0x01, speed[id / 2] - 1);
}

void LaserControl::btn_control_released(int id)
{
    emit send_lens_msg_addr(Lens::STOP, id > 1 ? 0x02: 0x01, speed[id / 2] - 1);
}
