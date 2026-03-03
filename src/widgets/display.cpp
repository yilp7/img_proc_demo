#include "display.h"

Display::Display(QWidget *parent) :
    QLabel(parent),
    lefttop(0, 0),
    center(0, 0),
    is_grabbing(false),
    mode(0),
    curr_scale(0),
//    scale{ QSize(640, 512), QSize(480, 384), QSize(320, 256), QSize(160, 128), QSize(80, 64)},
//    scale{ QSize(640, 400), QSize(480, 300), QSize(320, 200), QSize(160, 100), QSize(80, 50)},
    scale{ 1.0f, 2.0f / 3, 1.0f / 2, 1.0f / 4, 1.0f / 8},
    pressed(false)
{
    connect(this, SIGNAL(set_pixmap(const QPixmap&)), this, SLOT(setPixmap(const QPixmap&)));
    setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *add_to_roi = new QAction("add selection to roi", this);
    this->addAction(add_to_roi);
    connect(add_to_roi, &QAction::triggered, this, [this](){ emit add_roi(selection_v1, selection_v2); });
    QAction *clear_all_rois = new QAction("clear all rois", this);
    this->addAction(clear_all_rois);
    connect(clear_all_rois, &QAction::triggered, this, [this](){ emit clear_roi(); });
    update_roi(center);
}

void Display::update_roi(QPoint center)
{
    QPoint temp(scale[curr_scale] * this->width(), scale[curr_scale] * this->height());
    lefttop = center - temp / 2;
    if (lefttop.x() < 0) lefttop.setX(0);
    if (lefttop.y() < 0) lefttop.setY(0);
    if (lefttop.x() > this->width() - temp.x()) lefttop.setX(this->width() - temp.x());
    if (lefttop.y() > this->height() - temp.y()) lefttop.setY(this->height() - temp.y());

    display_region.x = lefttop.x();
    display_region.y = lefttop.y();
    display_region.width = scale[curr_scale] * this->width();
    display_region.height = scale[curr_scale] * this->height();
    this->center = lefttop + temp / 2;
}

void Display::clear()
{
    QLabel::clear();

    curr_scale = 0;

    QPoint roi_center = lefttop + QPoint((int)(scale[curr_scale] * this->width() / 2), (int)(scale[curr_scale] * this->height() / 2));
    update_roi(roi_center);

    selection_v2.x = selection_v1.x = 0;
    selection_v2.y = selection_v1.y = 0;
}

void Display::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);

    switch(event->button()) {
    case Qt::LeftButton: {
//        qDebug("pos: %d, %d\n", event->x(), event->y());
//        qDebug("pos: %d, %d\n", event->globalX(), event->globalY());
//        qDebug("%s pressed\n", qPrintable(this->objectName()));

        if (!is_grabbing) return;

        static QPoint curr_pos;
        curr_pos = event->pos();
        if (curr_pos.x() > this->rect().right()) curr_pos.setX(this->rect().right());
        if (curr_pos.y() > this->rect().bottom()) curr_pos.setY(this->rect().bottom());

        pressed = true;
        prev_pos = event->pos();
        ori_pos = lefttop;
        if (mode == 1) {
            selection_v2.x = selection_v1.x = curr_pos.x();
            selection_v2.y = selection_v1.y = curr_pos.y();
    //        emit start_pos(event->pos());
            emit updated_pos(1, curr_pos);
        }
        break;
    }
    case Qt::RightButton: {
        // TODO filter cursor position
//        this->actions().at(0)->setEnabled(false);
    }
    default: break;
    }
}

void Display::mouseMoveEvent(QMouseEvent *event)
{
    QLabel::mouseMoveEvent(event);

    if (!is_grabbing) return;
//    if (geometry().contains(event->pos())) emit curr_pos(event->pos());
    if (geometry().contains(event->pos())) emit updated_pos(0, event->pos());
    if (!pressed) return;
//    qDebug("pos: %d, %d\n", event->x(), event->y());
//    qDebug("pos: %d, %d\n", event->globalX(), event->globalY());
//    qDebug("%s selecting\n", qPrintable(this->objectName()));

    if (!mode) update_roi(ori_pos + QPoint((int)(scale[curr_scale] * this->width()), (int)(scale[curr_scale] * this->height())) / 2 - (event->pos() - prev_pos) * scale[curr_scale]);
//    prev_pos = event->pos();

    static QPoint curr_pos;
    curr_pos = event->pos();
    if (curr_pos.x() > this->rect().right()) curr_pos.setX(this->rect().right());
    if (curr_pos.y() > this->rect().bottom()) curr_pos.setY(this->rect().bottom());

    if (mode == 1) {
        selection_v2.x /*= selection_v1.x */= curr_pos.x();
        selection_v2.y /*= selection_v1.y */= curr_pos.y();
//        emit shape_size(event->pos() - QPoint(selection_v1.x, selection_v1.y));
        emit updated_pos(2, curr_pos - QPoint(selection_v1.x, selection_v1.y));
        emit updated_pos(4, (event->pos() + QPoint(selection_v1.x, selection_v1.y) - QPoint(this->width(), this->height())) / 2);
    }
}

void Display::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    if (!is_grabbing) return;

    pressed = false;
    if (mode == 1) {
        selection_v2.x = event->pos().x();
        selection_v2.y = event->pos().y();
//        emit shape_size(event->pos() - QPoint(selection_v1.x, selection_v1.y));
        emit updated_pos(2, event->pos() - QPoint(selection_v1.x, selection_v1.y));
        emit updated_pos(4, (event->pos() + QPoint(selection_v1.x, selection_v1.y) - QPoint(this->width(), this->height())) / 2);
    }
    else if (mode == 2) {
//        emit ptz_target(event->pos());
        emit updated_pos(3, event->pos());
    }
}

void Display::wheelEvent(QWheelEvent *event)
{
    QLabel::wheelEvent(event);

    if (!is_grabbing || mode) return;
    QPoint roi_center = lefttop + QPoint((int)(scale[curr_scale] * this->width() / 2), (int)(scale[curr_scale] * this->height() / 2));
    QLabel::wheelEvent(event);
    if(event->delta() > 0) {
        if (curr_scale >= 4) return;
        curr_scale++;
    }
    else {
        if (curr_scale <= 0) return;
        curr_scale--;
    }

//    if (curr_scale > 4) curr_scale = 4;
//    if (curr_scale < 0) curr_scale = 0;
    qDebug("current scale: %dx%d", (int)(scale[curr_scale] * this->width()), (int)(scale[curr_scale] * this->height()));

    update_roi(roi_center);
//    selection_v1 *= scale[curr_scale];
    //    selection_v2 *= scale[curr_scale];
}

void Display::mouseDoubleClickEvent(QMouseEvent *event)
{
    QLabel::mouseDoubleClickEvent(event);

    static bool i = 0;
    if (i ^= 1) {
        reserved_geometry = geometry();
        setWindowFlags(Qt::Dialog);
        showFullScreen();
    }
    else {
        setGeometry(reserved_geometry);
        setWindowFlags(Qt::Widget);
        showNormal();
    };
    qDebug() << this->width() << this->size();
}
