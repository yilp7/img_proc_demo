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
    emit view_changed();
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

        // Alt+drag in PTZ mode designates the follow gate. Held off the zoom
        // pan path below so dragging a gate does not also scroll the view.
        if (mode == 2 && event->modifiers().testFlag(Qt::AltModifier)) {
            gate_dragging = true;
            gate_origin = curr_pos;
            gate_rubber_band = QRect(curr_pos, curr_pos);
            update();
            return;
        }

        pressed = true;
        prev_pos = event->pos();
        ori_pos = lefttop;
        if (mode == 1) {
            selection_v2.x = selection_v1.x = curr_pos.x();
            selection_v2.y = selection_v1.y = curr_pos.y();
    //        emit start_pos(event->pos());
            emit updated_pos(1, curr_pos);
            emit view_changed();
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

    if (gate_dragging) {
        gate_rubber_band = QRect(gate_origin, event->pos()).normalized();
        update();
        return;
    }
    if (!pressed) return;
//    qDebug("pos: %d, %d\n", event->x(), event->y());
//    qDebug("pos: %d, %d\n", event->globalX(), event->globalY());
//    qDebug("%s selecting\n", qPrintable(this->objectName()));

    if (!mode) {
        update_roi(ori_pos + QPoint((int)(scale[curr_scale] * this->width()), (int)(scale[curr_scale] * this->height())) / 2 - (event->pos() - prev_pos) * scale[curr_scale]);
        emit view_changed();
    }
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
        emit view_changed();
    }
}

void Display::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    if (!is_grabbing) return;

    if (gate_dragging) {
        gate_dragging = false;
        const QRect gate = QRect(gate_origin, event->pos()).normalized().intersected(rect());
        gate_rubber_band = QRect();
        update();
        // Deliberately silent for tiny drags. An Alt+double-click sends
        // press/release pairs through here on its way to ptz_target, and
        // emitting an empty gate would tear down the follow it is engaging.
        if (gate.width() >= 8 && gate.height() >= 8) emit gate_selected(gate);
        return;
    }

    // Plain click in PTZ mode clears the gate. Alt is excluded so the
    // Alt+double-click follow gesture is not cancelled by its own first click,
    // and a drag is excluded so panning does not count as a click.
    if (mode == 2 && !event->modifiers().testFlag(Qt::AltModifier)
            && (event->pos() - prev_pos).manhattanLength() < 6) {
        emit gate_cleared();
    }

    pressed = false;
    if (mode == 1) {
        selection_v2.x = event->pos().x();
        selection_v2.y = event->pos().y();
//        emit shape_size(event->pos() - QPoint(selection_v1.x, selection_v1.y));
        emit updated_pos(2, event->pos() - QPoint(selection_v1.x, selection_v1.y));
        emit updated_pos(4, (event->pos() + QPoint(selection_v1.x, selection_v1.y) - QPoint(this->width(), this->height())) / 2);
        emit view_changed();
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
    emit view_changed();
//    selection_v1 *= scale[curr_scale];
    //    selection_v2 *= scale[curr_scale];
}

void Display::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    // Keep display_region in sync with widget size so the pipeline's crop
    // at pipelineprocessor.cpp:632 maps to the full source. Otherwise after
    // a resize (e.g. fullscreen toggle) the pipeline crops to the stale
    // pre-resize region and stretches that small area to fill the widget.
    QPoint roi_center = lefttop + QPoint((int)(scale[curr_scale] * width() / 2),
                                         (int)(scale[curr_scale] * height() / 2));
    update_roi(roi_center);
    emit widget_resized();
}

void Display::set_follow_gate(const QRect &gate)
{
    if (follow_gate == gate) return;
    follow_gate = gate;
    update();
}

void Display::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (follow_gate.isEmpty() && gate_rubber_band.isEmpty()) return;

    QPainter painter(this);
    if (!follow_gate.isEmpty()) {
        painter.setPen(QPen(QColor(255, 196, 0), 2));
        painter.drawRect(follow_gate);
    }
    if (!gate_rubber_band.isEmpty()) {
        painter.setPen(QPen(QColor(255, 196, 0), 1, Qt::DashLine));
        painter.drawRect(gate_rubber_band);
    }
}

void Display::mouseDoubleClickEvent(QMouseEvent *event)
{
    QLabel::mouseDoubleClickEvent(event);

    if (mode == 2) {
        pressed = false;
        // Alt selects closed-loop follow; plain click keeps the device's own.
        if (event->button() == Qt::LeftButton && is_grabbing)
            emit ptz_target(event->pos(), event->modifiers().testFlag(Qt::AltModifier));
        return; // PTZ tool: follow target instead of fullscreen on double-click
    }

    if (!is_fullscreen) {
        reserved_geometry = geometry();
        setWindowFlags(Qt::Dialog);
        showFullScreen();
        is_fullscreen = true;
    }
    else {
        setGeometry(reserved_geometry);
        setWindowFlags(Qt::Widget);
        showNormal();
        is_fullscreen = false;
    };
    qDebug() << this->width() << this->size();
}

void Display::closeEvent(QCloseEvent *event)
{
    if (is_fullscreen) {
        event->ignore();
        setGeometry(reserved_geometry);
        setWindowFlags(Qt::Widget);
        showNormal();
        is_fullscreen = false;
    }
}
