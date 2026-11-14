#include "display.h"

Display::Display(QWidget *parent) :
    QLabel(parent),
    m_lefttop(0, 0),
    m_center(0, 0),
    m_is_grabbing(false),
    m_mode(0),
    m_curr_scale(0),
//    scale{ QSize(640, 512), QSize(480, 384), QSize(320, 256), QSize(160, 128), QSize(80, 64)},
//    scale{ QSize(640, 400), QSize(480, 300), QSize(320, 200), QSize(160, 100), QSize(80, 50)},
    m_scale{ 1.0f, 2.0f / 3, 1.0f / 2, 1.0f / 4, 1.0f / 8},
    m_pressed(false)
{
    connect(this, SIGNAL(set_pixmap(const QPixmap&)), this, SLOT(setPixmap(const QPixmap&)));
    setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *add_to_roi = new QAction("add selection to roi", this);
    this->addAction(add_to_roi);
    connect(add_to_roi, &QAction::triggered, this, [this](){ emit add_roi(m_selection_v1, m_selection_v2); });
    QAction *clear_all_rois = new QAction("clear all rois", this);
    this->addAction(clear_all_rois);
    connect(clear_all_rois, &QAction::triggered, this, [this](){ emit clear_roi(); });
    update_roi(m_center);
}

void Display::update_roi(QPoint center)
{
    QPoint temp(m_scale[m_curr_scale] * this->width(), m_scale[m_curr_scale] * this->height());
    m_lefttop = center - temp / 2;
    if (m_lefttop.x() < 0) m_lefttop.setX(0);
    if (m_lefttop.y() < 0) m_lefttop.setY(0);
    if (m_lefttop.x() > this->width() - temp.x()) m_lefttop.setX(this->width() - temp.x());
    if (m_lefttop.y() > this->height() - temp.y()) m_lefttop.setY(this->height() - temp.y());

    m_display_region.x = m_lefttop.x();
    m_display_region.y = m_lefttop.y();
    m_display_region.width = m_scale[m_curr_scale] * this->width();
    m_display_region.height = m_scale[m_curr_scale] * this->height();
    this->m_center = m_lefttop + temp / 2;
}

void Display::clear()
{
    QLabel::clear();

    m_curr_scale = 0;

    QPoint roi_center = m_lefttop + QPoint((int)(m_scale[m_curr_scale] * this->width() / 2), (int)(m_scale[m_curr_scale] * this->height() / 2));
    update_roi(roi_center);

    m_selection_v2.x = m_selection_v1.x = 0;
    m_selection_v2.y = m_selection_v1.y = 0;
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

        if (!m_is_grabbing) return;

        static QPoint curr_pos;
        curr_pos = event->pos();
        if (curr_pos.x() > this->rect().right()) curr_pos.setX(this->rect().right());
        if (curr_pos.y() > this->rect().bottom()) curr_pos.setY(this->rect().bottom());

        // Alt+drag in PTZ mode designates the follow gate. Held off the zoom
        // pan path below so dragging a gate does not also scroll the view.
        if (m_mode == 2 && event->modifiers().testFlag(Qt::AltModifier)) {
            m_gate_dragging = true;
            m_gate_origin = curr_pos;
            m_gate_rubber_band = QRect(curr_pos, curr_pos);
            update();
            return;
        }

        m_pressed = true;
        m_prev_pos = event->pos();
        m_ori_pos = m_lefttop;
        if (m_mode == 1) {
            m_selection_v2.x = m_selection_v1.x = curr_pos.x();
            m_selection_v2.y = m_selection_v1.y = curr_pos.y();
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

    if (!m_is_grabbing) return;
//    if (geometry().contains(event->pos())) emit curr_pos(event->pos());
    if (geometry().contains(event->pos())) emit updated_pos(0, event->pos());

    if (m_gate_dragging) {
        m_gate_rubber_band = QRect(m_gate_origin, event->pos()).normalized();
        update();
        return;
    }
    if (!m_pressed) return;
//    qDebug("pos: %d, %d\n", event->x(), event->y());
//    qDebug("pos: %d, %d\n", event->globalX(), event->globalY());
//    qDebug("%s selecting\n", qPrintable(this->objectName()));

    if (!m_mode) {
        update_roi(m_ori_pos + QPoint((int)(m_scale[m_curr_scale] * this->width()), (int)(m_scale[m_curr_scale] * this->height())) / 2 - (event->pos() - m_prev_pos) * m_scale[m_curr_scale]);
        emit view_changed();
    }
//    m_prev_pos = event->pos();

    static QPoint curr_pos;
    curr_pos = event->pos();
    if (curr_pos.x() > this->rect().right()) curr_pos.setX(this->rect().right());
    if (curr_pos.y() > this->rect().bottom()) curr_pos.setY(this->rect().bottom());

    if (m_mode == 1) {
        m_selection_v2.x /*= selection_v1.x */= curr_pos.x();
        m_selection_v2.y /*= selection_v1.y */= curr_pos.y();
//        emit shape_size(event->pos() - QPoint(m_selection_v1.x, m_selection_v1.y));
        emit updated_pos(2, curr_pos - QPoint(m_selection_v1.x, m_selection_v1.y));
        emit updated_pos(4, (event->pos() + QPoint(m_selection_v1.x, m_selection_v1.y) - QPoint(this->width(), this->height())) / 2);
        emit view_changed();
    }
}

void Display::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    if (!m_is_grabbing) return;

    if (m_gate_dragging) {
        m_gate_dragging = false;
        const QRect gate = QRect(m_gate_origin, event->pos()).normalized().intersected(rect());
        m_gate_rubber_band = QRect();
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
    if (m_mode == 2 && !event->modifiers().testFlag(Qt::AltModifier)
            && (event->pos() - m_prev_pos).manhattanLength() < 6) {
        emit gate_cleared();
    }

    m_pressed = false;
    if (m_mode == 1) {
        m_selection_v2.x = event->pos().x();
        m_selection_v2.y = event->pos().y();
//        emit shape_size(event->pos() - QPoint(m_selection_v1.x, m_selection_v1.y));
        emit updated_pos(2, event->pos() - QPoint(m_selection_v1.x, m_selection_v1.y));
        emit updated_pos(4, (event->pos() + QPoint(m_selection_v1.x, m_selection_v1.y) - QPoint(this->width(), this->height())) / 2);
        emit view_changed();
    }
}

void Display::wheelEvent(QWheelEvent *event)
{
    QLabel::wheelEvent(event);

    if (!m_is_grabbing || m_mode) return;
    QPoint roi_center = m_lefttop + QPoint((int)(m_scale[m_curr_scale] * this->width() / 2), (int)(m_scale[m_curr_scale] * this->height() / 2));
    QLabel::wheelEvent(event);
    if(event->delta() > 0) {
        if (m_curr_scale >= 4) return;
        m_curr_scale++;
    }
    else {
        if (m_curr_scale <= 0) return;
        m_curr_scale--;
    }

//    if (m_curr_scale > 4) m_curr_scale = 4;
//    if (m_curr_scale < 0) m_curr_scale = 0;
    qDebug("current scale: %dx%d", (int)(m_scale[m_curr_scale] * this->width()), (int)(m_scale[m_curr_scale] * this->height()));

    update_roi(roi_center);
    emit view_changed();
//    m_selection_v1 *= m_scale[m_curr_scale];
    //    m_selection_v2 *= m_scale[m_curr_scale];
}

void Display::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    // Keep display_region in step with the widget size so the pipeline crops the full
    // source. Otherwise a resize leaves the stale region stretched over the widget.
    QPoint roi_center = m_lefttop + QPoint((int)(m_scale[m_curr_scale] * width() / 2),
                                           (int)(m_scale[m_curr_scale] * height() / 2));
    update_roi(roi_center);
    emit widget_resized();
}

void Display::set_follow_gate(const QRect &gate)
{
    if (m_follow_gate == gate) return;
    m_follow_gate = gate;
    update();
}

void Display::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (m_follow_gate.isEmpty() && m_gate_rubber_band.isEmpty()) return;

    QPainter painter(this);
    if (!m_follow_gate.isEmpty()) {
        painter.setPen(QPen(QColor(255, 196, 0), 2));
        painter.drawRect(m_follow_gate);
    }
    if (!m_gate_rubber_band.isEmpty()) {
        painter.setPen(QPen(QColor(255, 196, 0), 1, Qt::DashLine));
        painter.drawRect(m_gate_rubber_band);
    }
}

void Display::mouseDoubleClickEvent(QMouseEvent *event)
{
    QLabel::mouseDoubleClickEvent(event);

    if (m_mode == 2) {
        m_pressed = false;
        // Alt selects closed-loop follow; plain click keeps the device's own.
        if (event->button() == Qt::LeftButton && m_is_grabbing)
            emit ptz_target(event->pos(), event->modifiers().testFlag(Qt::AltModifier));
        return; // PTZ tool: follow target instead of fullscreen on double-click
    }

    if (!m_is_fullscreen) {
        m_reserved_geometry = geometry();
        setWindowFlags(Qt::Dialog);
        showFullScreen();
        m_is_fullscreen = true;
    }
    else {
        setGeometry(m_reserved_geometry);
        setWindowFlags(Qt::Widget);
        showNormal();
        m_is_fullscreen = false;
    };
    qDebug() << this->width() << this->size();
}

void Display::closeEvent(QCloseEvent *event)
{
    if (m_is_fullscreen) {
        event->ignore();
        setGeometry(m_reserved_geometry);
        setWindowFlags(Qt::Widget);
        showNormal();
        m_is_fullscreen = false;
    }
}
