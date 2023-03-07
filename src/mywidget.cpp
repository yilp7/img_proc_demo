#include "mywidget.h"

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

    if(event->button() != Qt::LeftButton) return;

//    qDebug("pos: %d, %d\n", event->x(), event->y());
//    qDebug("pos: %d, %d\n", event->globalX(), event->globalY());
//    qDebug("%s pressed\n", qPrintable(this->objectName()));

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
        selection_v2.x = selection_v1.x = curr_pos.x();
        selection_v2.y = selection_v1.y = curr_pos.y();
//        emit shape_size(event->pos() - QPoint(selection_v1.x, selection_v1.y));
        emit updated_pos(2, curr_pos - QPoint(selection_v1.x, selection_v1.y));
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

Ruler::Ruler(QWidget *parent) :
    QLabel(parent),
    vertical(false),
    interval(10)
{

}

void Ruler::draw_mark(QPainter *painter)
{
    float start = 0;
    if (vertical) {
        for (int i = 0; i <= 10000; i += 10) {
            if (i % 50) painter->drawRect(QRectF(5, start, 5, 0.1));
            else if (i % 100) painter->drawRect(QRectF(3, start, 7, 0.1));
            else if (i) {
                painter->drawRect(QRectF(0, start, 10, 0.1));
                QString str = QString::number(i);
                painter->rotate(90);
                painter->drawText(QPointF(start - painter->fontMetrics().width(str) / 2, -13), str);
                painter->rotate(-90);
//                painter->drawText(QPointF(13, start + fontMetrics().height() / 4), str);
            }
            start += interval;
        }

    }
    else {
        for (int i = 0; i <= 10000; i += 10) {
            if (i % 50) painter->drawRect(QRectF(start, 5, 0.1, 5));
            else if (i % 100) painter->drawRect(QRectF(start, 3, 0.1, 7));
            else if (i) {
                painter->drawRect(QRectF(start, 0, 0.1, 10));
                QString str = QString::number(i);
                painter->drawText(QPointF(start - painter->fontMetrics().width(str) / 2, 22), str);
            }
            start += interval;
        }
    }
}

void Ruler::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);

    painter.setRenderHint(QPainter::Antialiasing);
//    QFont temp = QFont(monaco);
//    temp.setPointSize(10);
//    temp.setWeight(QFont::Light);
    painter.setFont(monaco);
    draw_mark(&painter);

    painter.end();
}

InfoLabel::InfoLabel(QWidget *parent) : QLabel(parent) {}

TitleButton::TitleButton(QString icon, QWidget *parent) : QPushButton(QIcon(icon), "", parent) {}

void TitleButton::mouseMoveEvent(QMouseEvent *event) { setCursor(cursor_curr_pointer); }

TitleBar::TitleBar(QWidget *parent) :
    QFrame(parent),
    is_maximized(false),
    pressed(false),
    signal_receiver(NULL)
{
    icon = new InfoLabel(this);
    icon->setGeometry(10, 5, 20, 20);
    icon->setObjectName("ICON");
    title = new InfoLabel(this);
    title->setGeometry(30, 0, 80, 30);
    title->setObjectName("NAME");
    min = new TitleButton(":/tools/min", this);
    min->setObjectName("MIN_BTN");
    min->setMouseTracking(true);
    max = new TitleButton(":/tools/max", this);
    max->setObjectName("MAX_BTN");
    max->setMouseTracking(true);
    exit = new TitleButton(":/tools/exit", this);
    exit->setObjectName("EXIT_BTN");
    exit->setMouseTracking(true);
    connect(min, SIGNAL(clicked()), window(), SLOT(showMinimized()));
    connect(max, SIGNAL(clicked()), SLOT(process_maximize()));
    connect(exit, SIGNAL(clicked()), window(), SLOT(close()));
}

void TitleBar::setup(QObject *ptr)
{
    signal_receiver = ptr;

//    prog_settings = new ProgSettings();
    preferences = new Preferences();
    scan_config = new ScanConfig();

    url = new TitleButton("", this);
    url->setObjectName("URL_BTN");
    connect(url, SIGNAL(clicked()), signal_receiver, SLOT(prompt_for_input_file()));

    settings = new TitleButton("", this);
    settings->setObjectName("SETTINGS_BTN");
    QMenu *settings_menu = new QMenu();
    settings_menu->setFont(monaco);
    QAction *pref = new QAction("-- preferences", this);
    settings_menu->addAction(pref);
    pref->setShortcut(QKeySequence(Qt::ALT + Qt::Key_S));
/*
    connect(pref, SIGNAL(triggered()), prog_settings, SLOT(show()));
    connect(pref, SIGNAL(triggered()), prog_settings, SLOT(raise()));
    connect(prog_settings, SIGNAL(rep_freq_unit_changed(int)), signal_receiver, SLOT(setup_hz(int)));
    connect(prog_settings, SIGNAL(base_unit_changed(int)),     signal_receiver, SLOT(setup_stepping(int)));
    connect(prog_settings, SIGNAL(max_dist_changed(int)),      signal_receiver, SLOT(setup_max_dist(int)));
    connect(prog_settings, SIGNAL(laser_toggled(int)),         signal_receiver, SLOT(setup_laser(int)));
    connect(prog_settings, SIGNAL(change_baudrate(int, int)),  signal_receiver, SLOT(set_baudrate(int, int)));
    connect(prog_settings, SIGNAL(com_write(int, QByteArray)), signal_receiver, SLOT(com_write_data(int, QByteArray)));
    connect(prog_settings, SIGNAL(get_baudrate(int)),          signal_receiver, SLOT(display_baudrate(int)));
    connect(prog_settings, SIGNAL(share_serial_port(bool)),    signal_receiver, SLOT(set_serial_port_share(bool)));
    connect(prog_settings, SIGNAL(auto_mcp(bool)),             signal_receiver, SLOT(set_auto_mcp(bool)));
    connect(prog_settings, SIGNAL(set_dev_ip(int, int)),       signal_receiver, SLOT(set_dev_ip(int, int)));
    connect(prog_settings, SIGNAL(change_pixel_format(int)),   signal_receiver, SLOT(change_pixel_format(int)));
    connect(prog_settings, SIGNAL(reset_frame_a()),            signal_receiver, SLOT(reset_frame_a()));
*/
    connect(pref, SIGNAL(triggered()), preferences, SLOT(show()));
    connect(pref, SIGNAL(triggered()), preferences, SLOT(raise()));
    connect(preferences, SIGNAL(query_dev_ip()),                   signal_receiver, SLOT(update_dev_ip()));
    connect(preferences, SIGNAL(set_dev_ip(int, int)),             signal_receiver, SLOT(set_dev_ip(int, int)));
    connect(preferences, SIGNAL(change_pixel_format(int)),         signal_receiver, SLOT(change_pixel_format(int)));
    connect(preferences, SIGNAL(get_baudrate(int)),                signal_receiver, SLOT(display_baudrate(int)));
    connect(preferences, SIGNAL(change_baudrate(int, int)),        signal_receiver, SLOT(set_baudrate(int, int)));
    connect(preferences, SIGNAL(share_tcu_port(bool)),             signal_receiver, SLOT(set_tcu_as_shared_port(bool)));
    // TODO use_tcp_chk updated
    connect(preferences, SIGNAL(com_write(int, QByteArray)),       signal_receiver, SLOT(com_write_data(int, QByteArray)));
    connect(preferences, SIGNAL(tcu_type_changed(int)),            signal_receiver, SLOT(set_tcu_type(int)));
    connect(preferences, SIGNAL(rep_freq_unit_changed(int)),       signal_receiver, SLOT(setup_hz(int)));
    connect(preferences, SIGNAL(base_unit_changed(int)),           signal_receiver, SLOT(setup_stepping(int)));
    connect(preferences, SIGNAL(max_dist_changed(float)),          signal_receiver, SLOT(setup_max_dist(float)));
    connect(preferences, SIGNAL(delay_offset_changed(float)),      signal_receiver, SLOT(update_delay_offset(float)));
    connect(preferences, SIGNAL(gate_width_offset_changed(float)), signal_receiver, SLOT(update_gate_width_offset(float)));
    connect(preferences, SIGNAL(laser_offset_changed(float)),      signal_receiver, SLOT(update_laser_offset(float)));
    connect(preferences, SIGNAL(laser_toggled(int)),               signal_receiver, SLOT(setup_laser(int)));
    connect(preferences, SIGNAL(lower_3d_thresh_updated()),        signal_receiver, SLOT(update_lower_3d_thresh()));
    connect(preferences, SIGNAL(query_tcu_param()),                signal_receiver, SLOT(reset_custom_3d_params()));

    settings_menu->addAction(">> export pref.", signal_receiver, SLOT(export_config()), QKeySequence(Qt::ALT + Qt::Key_E));
    settings_menu->addAction("<< load pref.",   signal_receiver, SLOT(prompt_for_config_file()), QKeySequence(Qt::ALT + Qt::Key_R));
    // TODO congigure serial number should be exclusive to ICMOS only
    settings_menu->addAction("## config s.n.",  signal_receiver, SLOT(prompt_for_serial_file()), QKeySequence(Qt::ALT + Qt::Key_C));
    settings_menu->addAction("=> export video", signal_receiver, SLOT(save_current_video()), QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_S));
    settings->setMenu(settings_menu);

    capture = new TitleButton("", this);
    capture->setObjectName("CAPTURE_BTN");
    connect(capture, SIGNAL(clicked()), signal_receiver, SLOT(screenshot()));

    cls = new TitleButton("", this);
    cls->setObjectName("CLS_BTN");
    connect(cls, SIGNAL(clicked()), signal_receiver, SLOT(clean()));

    lang = new TitleButton("", this);
    lang->setObjectName("LANGUAGE_BTN");
    connect(lang, SIGNAL(clicked()), signal_receiver, SLOT(switch_language()));

    theme = new TitleButton("", this);
    theme->setObjectName("THEME_BTN");
    connect(theme, SIGNAL(clicked()), signal_receiver, SLOT(set_theme()));
}

void TitleBar::process_maximize()
{
    if (!is_maximized) normal_stat = window()->geometry();
    max->setIcon(is_maximized ? QIcon(":/tools/max") : QIcon(":/tools/restore"));
    is_maximized ? window()->setGeometry(normal_stat) : window()->setGeometry(QApplication::desktop()->availableGeometry());
    is_maximized ^= 1;
}

void TitleBar::resizeEvent(QResizeEvent *event)
{
    url->setGeometry(this->width() - 350, 5, 20, 20);
    settings->setGeometry(this->width() - 310, 5, 20, 20);
    capture->setGeometry(this->width() - 270, 5, 20, 20);
    cls->setGeometry(this->width() - 230, 5, 20, 20);
    lang->setGeometry(this->width() - 190, 5, 20, 20);
    theme->setGeometry(this->width() - 150, 5, 20, 20);
    min->setGeometry(this->width() - 120, 0, 40, 30);
    max->setGeometry(this->width() - 80, 0, 40, 30);
    exit->setGeometry(this->width() - 40, 0, 40, 30);

    QFrame::resizeEvent(event);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    QFrame::mousePressEvent(event);

    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    QFrame::mouseMoveEvent(event);

    if (is_maximized || !pressed) return;
    // use globalPos instead of pos to prevent window shaking
    window()->move(window()->pos() + event->globalPos() - prev_pos);
    prev_pos = event->globalPos();
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    QFrame::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    pressed = false;
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);

    if (event->button() != Qt::LeftButton) return;
    process_maximize();
}

AnimationLabel::AnimationLabel(QWidget *parent, QString path, QSize size, int interval) : QLabel(parent) {
    connect(&t, SIGNAL(timeout()), SLOT(draw()));
    setup_animation(path, size, interval);
}

void AnimationLabel::setup_animation(QString path, QSize size, int interval)
{
    source = QPixmap(path);
    partition_size = size;
    curr_pos = QPoint();
    t.stop();
    t.start(interval);
    draw();
}

void AnimationLabel::draw()
{
    if (curr_pos.x() > source.width() - 1) curr_pos -= QPoint(source.width(), -partition_size.height());
    if (curr_pos.y() > source.height() - 1) curr_pos = QPoint();
    QPixmap partition = source.copy(curr_pos.x(), curr_pos.y(), partition_size.width(), partition_size.height());
    this->setPixmap(partition.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    curr_pos += QPoint(partition_size.width(), 0);
}

Tools::Tools(QWidget *parent) : QRadioButton(parent) {}

Coordinate::Coordinate(QWidget *parent) : QFrame(parent), pair(0, 0), set_name(NULL), coord_x(NULL), coord_y(NULL) {}

void Coordinate::setup(QString name)
{
    if (set_name) delete set_name;
    if (coord_x) delete coord_x;
    if (coord_y) delete coord_y;
    set_name = new InfoLabel(this);
    set_name->setText(name);
    set_name->setFont(monaco);
//    set_name->setStyleSheet("color: #DEC4B0;");
    set_name->setGeometry(0, 0, this->fontMetrics().width(name) + 10, 13);
    coord_x = new InfoLabel(this);
    coord_x->setText("X: xxxx");
    coord_x->setFont(monaco);
//    coord_x->setStyleSheet("color: #DEC4B0;");
    coord_x->setGeometry(set_name->geometry().right() + 10, 0, this->fontMetrics().width("X: xxxx") + 10, 13);
    coord_y = new InfoLabel(this);
    coord_y->setText("Y: yyyy");
    coord_y->setFont(monaco);
//    coord_y->setStyleSheet("color: #DEC4B0;");
    coord_y->setGeometry(set_name->geometry().right() + 10, 17, this->fontMetrics().width("Y: yyyy") + 10, 13);

    this->resize(set_name->width() + 10 + coord_x->width(), 30);
}

void Coordinate::display_pos(QPoint p)
{
    pair.setX(p.x());
    pair.setY(p.y());
    coord_x->setText(QString::asprintf("X: %04d", abs(p.x())));
    coord_y->setText(QString::asprintf("Y: %04d", abs(p.y())));
}

IndexLabel::IndexLabel(QWidget *parent) : QLabel(parent) {}

void IndexLabel::setup(int idx, int pos_y)
{
    this->idx = idx;
    this->pos_y = pos_y;
}

void IndexLabel::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    emit index_label_clicked(pos_y);
}

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

    img_pixel_format = new StatusIcon(this);
    img_pixel_format->setGeometry(280, 0, 48, 32);
    img_pixel_format->setObjectName("IMG_PIXEL_FORMAT");
    img_pixel_format->setup("MONO8");
    img_pixel_depth = new StatusIcon(this);
    img_pixel_depth->setGeometry(332, 0, 48, 32);
    img_pixel_depth->setObjectName("IMG_PIXEL_DEPTH");
    img_pixel_depth->setup("8-bit");
    img_resolution = new StatusIcon(this);
    img_resolution->setGeometry(384, 0, 48, 32);
    img_resolution->setObjectName("IMG_RESOLUTION");
    img_resolution->setup("");
    result_cam_fps = new StatusIcon(this);
    result_cam_fps->setGeometry(436, 0, 48, 32);
    result_cam_fps->setObjectName("RESULT_CAM_FPS");
    result_cam_fps->setup("");
}
