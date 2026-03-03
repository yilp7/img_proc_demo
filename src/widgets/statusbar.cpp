#include "statusbar.h"

StatusIcon::StatusIcon(QWidget *parent) :
    QFrame(parent),
    status_block(NULL),
    status_dot(NULL),
    status(StatusIcon::STATUS::NONE)
{
    status_block = new QLabel(this);
    status_block->setObjectName(this->objectName() + "_BLOCK");
    connect(this, SIGNAL(set_pixmap(QPixmap)), status_block, SLOT(setPixmap(QPixmap)), Qt::QueuedConnection);
    connect(this, SIGNAL(set_text(QString)),   status_block, SLOT(setText(QString)), Qt::QueuedConnection);

    status_dot = new QLabel(this);
    status_dot->setGeometry(22, 20, 8, 8);
    status_dot->setObjectName(this->objectName() + "_DOT");
    connect(this, SIGNAL(change_status(QPixmap)), status_dot, SLOT(setPixmap(QPixmap)), Qt::QueuedConnection);
    status_block->stackUnder(status_dot);
}

void StatusIcon::setup(QPixmap img)
{
    status_block->setGeometry(0, 0, this->width(), this->height());
    status_block->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    emit set_pixmap(img.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void StatusIcon::setup(QString str)
{
    status_block->setGeometry(0, 0, this->width(), this->height());
    status_block->setAlignment(Qt::AlignCenter);
    emit set_text(str);
}

void StatusIcon::update_status(STATUS status)
{
    this->status = status;
    emit change_status(get_status_dot(status));
}

const QPixmap StatusIcon::get_status_dot(STATUS status)
{
    const static QPixmap STATUS_NONE = QPixmap();
    const static QPixmap STATUS_NOT_CONNECTED = QPixmap(":/status/dots/not_connected").scaled(8, 8, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const static QPixmap STATUS_DISCONNECTED = QPixmap(":/status/dots/disconnected").scaled(8, 8, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const static QPixmap STATUS_RECONNECTING = QPixmap(":/status/dots/reconnecting").scaled(8, 8, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const static QPixmap STATUS_CONNECTED = QPixmap(":/status/dots/connected").scaled(8, 8, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    const static QPixmap STATUS_SET[5] = {STATUS_NONE, STATUS_NOT_CONNECTED, STATUS_DISCONNECTED, STATUS_RECONNECTING, STATUS_CONNECTED};

    return STATUS_SET[status];
}

StatusBar::StatusBar(QWidget *parent) : QFrame(parent)
{
    cam_status = new StatusIcon(this);
    cam_status->setGeometry(20, 0, 32, 32);
    cam_status->setObjectName("CAM_STATUS");
    cam_status->setup(QPixmap(":/status/dark/cam"));
    tcu_status = new StatusIcon(this);
    tcu_status->setGeometry(72, 0, 32, 32);
    tcu_status->setObjectName("TCU_STATUS");
    tcu_status->setup(QPixmap(":/status/dark/tcu"));
    lens_status = new StatusIcon(this);
    lens_status->setGeometry(124, 0, 32, 32);
    lens_status->setObjectName("LENS_STATUS");
    lens_status->setup(QPixmap(":/status/dark/lens"));
    laser_status = new StatusIcon(this);
    laser_status->setGeometry(176, 0, 32, 32);
    laser_status->setObjectName("LASER_STATUS");
    laser_status->setup(QPixmap(":/status/dark/laser"));
    ptz_status = new StatusIcon(this);
    ptz_status->setGeometry(228, 0, 32, 32);
    ptz_status->setObjectName("PTZ_STATUS");
    ptz_status->setup(QPixmap(":/status/dark/ptz"));

    img_pixel_depth = new StatusIcon(this);
    img_pixel_depth->setGeometry(280, 0, 48, 32);
    img_pixel_depth->setObjectName("IMG_PIXEL_DEPTH");
    img_pixel_depth->setup("8-bit");
    img_resolution = new StatusIcon(this);
    img_resolution->setGeometry(332, 0, 120, 32);
    img_resolution->setObjectName("IMG_RESOLUTION");
    img_resolution->setup("0 x 0");
    packet_lost = new StatusIcon(this);
    packet_lost->setGeometry(456, 0, 132, 32);
    packet_lost->setObjectName("PACKET_LOST");
    packet_lost->setup("packets lost: 0");
    result_cam_fps = new StatusIcon(this);
    result_cam_fps->setGeometry(592, 0, 48, 32);
    result_cam_fps->setObjectName("RESULT_CAM_FPS");
    result_cam_fps->setup("");
}
