#include "userpanel.h"
#include "ui_user_panel.h"
#include "ui_preferences.h"

GrabThread::GrabThread(void *info)
{
    p_info = info;
}

void GrabThread::run()
{
    ((UserPanel*)p_info)->grab_thread_process();
}

UserPanel* wnd;

UserPanel::UserPanel(QWidget *parent)
    : QMainWindow(parent),
    mouse_pressed(false),
    ui(new Ui::UserPanel),
    status_bar(NULL),
    pref(NULL),
    scan_config(NULL),
    laser_settings(NULL),
    calc_avg_option(5),
    trigger_by_software(false),
    curr_cam(NULL),
    time_exposure_edit(5000),
    gain_analog_edit(0),
    frame_rate_edit(10),
    ptr_tcu(NULL),
    ptr_lens(NULL),
    ptr_laser(NULL),
    ptr_ptz(NULL),
    serial_port{NULL},
    serial_port_connected{false},
    tcp_port{NULL},
    use_tcp{false},
    share_serial_port(false),
    rep_freq(30),
    delay_n_n(0),
    stepping(10),
    hz_unit(0),
    base_unit(0),
    fps(10),
    duty(5000),
    mcp(5),
    laser_on(0b0001),
    zoom(0),
    focus(0),
    distance(0),
    max_dist(15000),
    laser_width(500),
    delay_dist(75),
    depth_of_view(75),
    focus_direction(0),
    curr_laser_idx(-1),
    display_option(1),
    updated(false),
    device_type(0),
    device_on(false),
    start_grabbing(false),
    record_original(false),
    record_modified(false),
    save_original(false),
    save_modified(false),
    image_3d(false),
    is_color(false),
    w(640),
    h(400),
    pixel_format(PixelType_Gvsp_Mono8),
    pixel_depth(8),
    grab_image(false),
    h_grab_thread(NULL),
    grab_thread_state(false),
    video_thread_state(false),
    h_joystick_thread(NULL),
    seq_idx(0),
    scan(false),
    scan_distance(200),
#ifdef LVTONG
    c(3e8 * 0.75),
#else
    c(3e8),
#endif
    frame_a_3d(false),
    auto_mcp(false),
    multi_laser_lenses(false),
    hide_left(false),
    resize_place(22),
    joybtn_A(false),
    joybtn_B(false),
    joybtn_X(false),
    joybtn_Y(false),
    joybtn_L1(false),
    joybtn_L2(false),
    joybtn_R1(false),
    joybtn_R2(false),
    lens_adjust_ongoing(0),
    ptz_adjust_ongoing(0),
    en(false),
    tp(40),
    offset_laser_width(0),
    offset_delay(0),
    offset_gatewidth(0),
    ptz_grp(NULL),
    ptz_speed(16)
{
    ui->setupUi(this);
    wnd = this;

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    // TODO complete rounded rect for main window
//    QBitmap bg(this->size());
//    bg.fill();
//    QPainter bg_painter(&bg);
//    bg_painter.setPen(Qt::transparent);
//    bg_painter.setBrush(Qt::black);
//    bg_painter.setRenderHint(QPainter::HighQualityAntialiasing);
//    bg_painter.drawRoundRect(bg.rect(), 1, 2);
//    setMask(bg);

    trans.load("project_zh.qm");
    ui->CENTRAL->setMouseTracking(true);
    ui->LEFT->setMouseTracking(true);
    ui->MID->setMouseTracking(true);
    ui->RIGHT->setMouseTracking(true);
    ui->TITLE->setMouseTracking(true);
    setMouseTracking(true);
    setCursor(cursor_curr_pointer);

    tp.start();

    dist_ns = c * 1e-9 / 2;

    // initialization
    // - default save path
    save_location += QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
//    qDebug() << QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section('/', 0, -1);
    TEMP_SAVE_LOCATION = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s/%s);", app_theme ? "light" : "dark", hide_left ? "right" : "left"));

    connect(this, SIGNAL(set_pixmap(QPixmap)), ui->SOURCE_DISPLAY, SLOT(setPixmap(QPixmap)), Qt::QueuedConnection);

    // - image operations
    QComboBox *enhance_options = ui->ENHANCE_OPTIONS;
    QComboBox *calc_avg_options = ui->FRAME_AVG_OPTIONS;
    enhance_options->addItem(tr("None"));
    enhance_options->addItem(tr("Histogram"));
    enhance_options->addItem(tr("Laplace"));
    enhance_options->addItem(tr("SP-5"));
    enhance_options->addItem(tr("Accumulative"));
    enhance_options->addItem(tr("Sigmoid-based"));
    enhance_options->addItem(tr("Adaptive"));
    enhance_options->addItem(tr("Dehaze_enh"));
    enhance_options->addItem(tr("DCP"));
    enhance_options->setCurrentIndex(0);
    calc_avg_options->addItem("a");
    calc_avg_options->addItem("b");
    calc_avg_options->setCurrentIndex(0);

    // - set-up COMs
//    foreach (const QSerialPortInfo & info, QSerialPortInfo::availablePorts()) qDebug("%s\n", qPrintable(info.portName()));
    com_label[0] = ui->TCU_COM;
    com_label[1] = ui->LENS_COM;
    com_label[2] = ui->LASER_COM;
    com_label[3] = ui->PTZ_COM;
    com_label[4] = ui->RANGE_COM;

    com_edit[0] = ui->TCU_COM_EDIT;
    com_edit[1] = ui->LENS_COM_EDIT;
    com_edit[2] = ui->LASER_COM_EDIT;
    com_edit[3] = ui->PTZ_COM_EDIT;
    com_edit[4] = ui->RANGE_COM_EDIT;

//    setup_com(com + 0, 0, com_edit[0]->text(), 9600);
//    setup_com(com + 1, 1, com_edit[1]->text(), 115200);
//    setup_com(com + 2, 2, com_edit[2]->text(), 9600);
//    setup_com(com + 3, 3, com_edit[3]->text(), 9600);

    // - set up sliders
    ui->MCP_SLIDER->setMinimum(0);
    ui->MCP_SLIDER->setMaximum(255);
    ui->MCP_SLIDER->setSingleStep(1);
    ui->MCP_SLIDER->setPageStep(10);
    ui->MCP_SLIDER->setValue(0);
    connect(ui->MCP_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_mcp(int)));

    ui->DELAY_SLIDER->setMinimum(0);
    ui->DELAY_SLIDER->setMaximum(max_dist);
    ui->DELAY_SLIDER->setSingleStep(10);
    ui->DELAY_SLIDER->setPageStep(100);
    ui->DELAY_SLIDER->setValue(0);
    connect(ui->DELAY_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_delay(int)));

    ui->FOCUS_SPEED_SLIDER->setMinimum(1);
    ui->FOCUS_SPEED_SLIDER->setMaximum(64);
    ui->FOCUS_SPEED_SLIDER->setSingleStep(1);
    ui->FOCUS_SPEED_SLIDER->setPageStep(4);
    ui->FOCUS_SPEED_SLIDER->setValue(32);
    connect(ui->FOCUS_SPEED_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_focus_speed(int)));

    ui->CONTINUE_SCAN_BUTTON->hide();
    ui->RESTART_SCAN_BUTTON->hide();

    // connect QLineEdit, QButton signals used in thread
    connect(this, &UserPanel::append_text, this,
            [this](QString text) {
                static QScrollBar *scroll_bar = ui->DATA_EXCHANGE->verticalScrollBar();
                ui->DATA_EXCHANGE->append(text);
                scroll_bar->setValue(scroll_bar->maximum());
    });
    connect(this, SIGNAL(update_scan(bool)), SLOT(enable_scan_options(bool)), Qt::QueuedConnection);
    connect(this, SIGNAL(update_delay_in_thread()), SLOT(update_delay()), Qt::QueuedConnection);
    connect(this, SIGNAL(update_mcp_in_thread(int)), SLOT(change_mcp(int)), Qt::QueuedConnection);

    // register signal when thread pool full
    connect(this, SIGNAL(task_queue_full()), SLOT(stop_image_writing()), Qt::UniqueConnection);

    connect(ui->RANGE_THRESH_EDIT, &QLineEdit::textChanged, this,
            [this](QString arg1){
                pref->lower_3d_thresh = arg1.toFloat() / (1 << pixel_depth);
                pref->ui->LOWER_3D_THRESH_EDT->setText(QString::number(pref->lower_3d_thresh, 'f', 3));
    });

    ui->BRIGHTNESS_SLIDER->setMinimum(-5);
    ui->BRIGHTNESS_SLIDER->setMaximum(5);
    ui->BRIGHTNESS_SLIDER->setSingleStep(1);
    ui->BRIGHTNESS_SLIDER->setPageStep(2);
    ui->BRIGHTNESS_SLIDER->setValue(0);
    ui->BRIGHTNESS_SLIDER->setTickInterval(5);

    ui->CONTRAST_SLIDER->setMinimum(-10);
    ui->CONTRAST_SLIDER->setMaximum(10);
    ui->CONTRAST_SLIDER->setSingleStep(1);
    ui->CONTRAST_SLIDER->setPageStep(3);
    ui->CONTRAST_SLIDER->setValue(0);
    ui->CONTRAST_SLIDER->setTickInterval(5);

    ui->GAMMA_SLIDER->setMinimum(0);
    ui->GAMMA_SLIDER->setMaximum(20);
    ui->GAMMA_SLIDER->setSingleStep(1);
    ui->GAMMA_SLIDER->setPageStep(3);
    ui->GAMMA_SLIDER->setValue(10);
    ui->GAMMA_SLIDER->setTickInterval(5);

    ui->RULER_V->vertical = true;
    ui->ZOOM_TOOL->click();
    ui->CURR_COORD->setup("current");
//    connect(ui->SOURCE_DISPLAY, SIGNAL(curr_pos(QPoint)), ui->CURR_COORD, SLOT(display_pos(QPoint)));
    ui->START_COORD->setup("start");
//    connect(ui->SOURCE_DISPLAY, SIGNAL(start_pos(QPoint)), ui->START_COORD, SLOT(display_pos(QPoint)));
    ui->SHAPE_INFO->setup("size");
//    connect(ui->SOURCE_DISPLAY, SIGNAL(shape_size(QPoint)), ui->SHAPE_INFO, SLOT(display_pos(QPoint)));
    connect(ui->SOURCE_DISPLAY, &Display::updated_pos, this,
        [this](int idx, QPoint pos) {
            static QPoint converted_pos(0, 0);
            static Display *disp = ui->SOURCE_DISPLAY;
            converted_pos.setX(pos.x() * img_mem.cols / disp->width());
            converted_pos.setY(pos.y() * img_mem.rows / disp->height());
//            converted_pos /= 4;
//            converted_pos *= 4;
            switch (idx) {
            case 0: ui->CURR_COORD->display_pos(converted_pos); break;
            case 1: ui->START_COORD->display_pos(converted_pos); break;
            case 2: ui->SHAPE_INFO->display_pos(converted_pos); break;
            case 3: point_ptz_to_target(pos); break;
            default: break;
            }
        });
//    ui->SOFTWARE_CHECK->hide();
//    ui->SOFTWARE_TRIGGER_BUTTON->hide();

    ptz_grp = new QButtonGroup(this);
    ui->UP_LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up_left"));
    ptz_grp->addButton(ui->UP_LEFT_BTN, 0);
    ui->UP_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up"));
    ptz_grp->addButton(ui->UP_BTN, 1);
    ui->UP_RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up_right"));
    ptz_grp->addButton(ui->UP_RIGHT_BTN, 2);
    ui->LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/left"));
    ptz_grp->addButton(ui->LEFT_BTN, 3);
    ui->SELF_TEST_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    ptz_grp->addButton(ui->SELF_TEST_BTN, 4);
    ui->RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/right"));
    ptz_grp->addButton(ui->RIGHT_BTN, 5);
    ui->DOWN_LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down_left"));
    ptz_grp->addButton(ui->DOWN_LEFT_BTN, 6);
    ui->DOWN_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down"));
    ptz_grp->addButton(ui->DOWN_BTN, 7);
    ui->DOWN_RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down_right"));
    ptz_grp->addButton(ui->DOWN_RIGHT_BTN, 8);
    connect(ptz_grp, SIGNAL(buttonPressed(int)), this, SLOT(ptz_button_pressed(int)));
    connect(ptz_grp, SIGNAL(buttonReleased(int)), this, SLOT(ptz_button_released(int)));
    ui->RESET_3D_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));

    QSlider *speed_slider = ui->PTZ_SPEED_SLIDER;
    speed_slider->setMinimum(1);
    speed_slider->setMaximum(64);
    speed_slider->setSingleStep(1);
    speed_slider->setValue(16);
    ui->PTZ_SPEED_EDIT->setText("16");

    // initialize gatewidth lut
    for (int i = 0; i < 100; i++) gw_lut[i] = i;

    scan_q.push_back(-1);
//    setup_stepping(0);

    // - write default params
    data_exchange(false);
    enable_controls(false);

    // set up display info (left bottom corner)
    ui->DATA_EXCHANGE->document()->setMaximumBlockCount(200);
    ui->COM_DATA_RADIO->click();
    QFont temp_f(consolas);
    temp_f.setPixelSize(11);
    ui->DATA_EXCHANGE->setFont(temp_f);
    ui->FILE_PATH_EDIT->setFont(consolas);

    display_grp = new QButtonGroup(this);
    display_grp->addButton(ui->COM_DATA_RADIO);
    display_grp->addButton(ui->HISTOGRAM_RADIO);
    display_grp->addButton(ui->PTZ_RADIO);
    display_grp->setExclusive(true);
    // TODO change continuous/trigger exclusive

    // connect title bar to the main window (UserPanel*)
    ui->TITLE->setup(this);
    pref = ui->TITLE->preferences;
    scan_config = ui->TITLE->scan_config;

    // connect status bar to the main window
    status_bar = ui->STATUS;

    // connect to joystick (windows xbox only)
    h_joystick_thread = new JoystickThread(this);
    h_joystick_thread->start();

    connect(h_joystick_thread, SIGNAL(button_pressed(int)), this, SLOT(joystick_button_pressed(int)));
    connect(h_joystick_thread, SIGNAL(button_released(int)), this, SLOT(joystick_button_released(int)));
    connect(h_joystick_thread, SIGNAL(direction_changed(int)), this, SLOT(joystick_direction_changed(int)));

//    qRegisterMetaType<QVideoFrame>("QVideoFrame&");
//    video_input = new QMediaPlayer(this);
//    video_surface = new VideoSurface(this);
//    video_input->setVideoOutput(video_surface);
//    connect(video_surface, &VideoSurface::frameAvailable, this,
//        [this](QVideoFrame &current_frame) {
//            video_surface->frame_count++;

//            if (video_surface->frame_count == 1) start_static_display(current_frame.width(), current_frame.height(), true, 8, -2);

//            current_frame.map(QAbstractVideoBuffer::ReadOnly);
//            static cv::Mat converted_rgb;
//            cv::cvtColor(cv::Mat(current_frame.height(), current_frame.width(), CV_8UC4, current_frame.bits(), current_frame.bytesPerLine()), converted_rgb, cv::COLOR_RGBA2BGR);
//            img_q.push(converted_rgb);
//            current_frame.unmap();
//        }, Qt::QueuedConnection);

    laser_settings = new LaserSettings();
    connect(laser_settings, SIGNAL(com_write(int, QByteArray)), this, SLOT(com_write_data(int, QByteArray)));

    // right before gui display (init state)
    // setup serial port with tcp socket
    ptr_tcu   = new TCU(  ui->TCU_COM,   ui->TCU_COM_EDIT,   0, ui->STATUS->tcu_status, this);
    ptr_lens  = new Lens( ui->LENS_COM,  ui->LENS_COM_EDIT,  1, ui->STATUS->lens_status, this);
    ptr_laser = new Laser(ui->LASER_COM, ui->LASER_COM_EDIT, 2, ui->STATUS->laser_status, this);
    ptr_ptz   = new PTZ(  ui->PTZ_COM,   ui->PTZ_COM_EDIT,   3, ui->STATUS->ptz_status, this);
//    qDebug() << QThread::currentThread();
    connect(ptr_tcu,   &ControlPort::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
    connect(ptr_lens,  &ControlPort::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
    connect(ptr_laser, &ControlPort::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
    connect(ptr_ptz,   &ControlPort::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);

    uchar com[5] = {0};
    FILE *f = fopen("user_default", "r");
    if (f) {
        fread(com, 1, 5, f);
        fclose(f);
    }
    for (int i = 0; i < 5; i++) com_edit[i]->setText(QString::number(com[i])), com_edit[i]->emit returnPressed();

    for (int i = 0; i < 5; i++) serial_port[i] = new QSerialPort(this)/*, setup_serial_port(serial_port + i, i, com_edit[i]->text(), 9600)*/;
    for (int i = 0; i < 5; i++) tcp_port[i] = new QTcpSocket(this);
    on_ENUM_BUTTON_clicked();
    if (serial_port[0]->isOpen() && serial_port[3]->isOpen()) on_LASER_BTN_clicked();

    // connect ptz targeting slots (moved to signal updated_pos)
//    connect(ui->SOURCE_DISPLAY, SIGNAL(ptz_target(QPoint)), this, SLOT(point_ptz_to_target(QPoint)));

    // - set startup focus
    (ui->START_BUTTON->isEnabled() ? ui->START_BUTTON : ui->ENUM_BUTTON)->setFocus();

#ifdef LVTONG
    ui->DISTANCE->hide();
    ui->DIST_BTN->hide();

    connect(this, SIGNAL(update_fishnet_result(int)), SLOT(display_fishnet_result(int)));
#else
    ui->FIRE_LASER_BTN->hide();
    ui->FISHNET_RESULT->hide();
#endif

#ifdef ICMOS
    ui->RANGE_COM->setText("R1");
    ui->LASER_COM->setText("R2");
    ui->RANGE_COM_EDIT->setEnabled(false);
    ui->LASER_COM_EDIT->setEnabled(false);

    ui->ESTIMATED->hide();
    ui->EST_DIST->hide();
    ui->ESTIMATED_2->hide();
    ui->GATE_WIDTH->hide();
    ui->DELAY_SLIDER->move(30, 153);

    ui->LOGO->move(10, 0);
    ui->INITIALIZATION->move(10, ui->INITIALIZATION->y() + 60);
    ui->IMG_GRAB_STATIC->move(10, ui->IMG_GRAB_STATIC->y() + 60);
    ui->PARAMS_STATIC->move(10, ui->PARAMS_STATIC->y() + 60);

    ui->LASER_STATIC->hide();
    ui->ALT_DISPLAY->setParent(ui->RIGHT);
    ui->ALT_DISPLAY->setGeometry(10, 365, ui->IMG_PROC_STATIC->width(), ui->ALT_DISPLAY->height());
    QSize temp = ui->DATA_EXCHANGE->size();
    temp.setWidth(ui->ALT_DISPLAY->width());
    ui->DATA_EXCHANGE->resize(temp);
    ui->HIST_DISPLAY->resize(temp);
    ui->LENS_STATIC->hide();
    ui->IMG_SAVE_STATIC->move(10, 265);
    ui->IMG_PROC_STATIC->hide();
    ui->SCAN_GRP->move(10, 560);
    ui->IMG_3D_CHECK->hide();
    ui->RANGE_THRESH_EDIT->hide();

    ui->DELAY_N->hide();
    ui->DELAY_N_EDIT_N->hide();
    ui->DELAY_N_UNIT_N->hide();
    ui->MCP_SLIDER->resize(ui->MCP_SLIDER->width() + 20, ui->MCP_SLIDER->height());
    ui->MCP_EDIT->move(170, 65);
    ui->DELAY_B_EDIT_U->setEnabled(false);
    ui->DELAY_B_EDIT_N->setEnabled(false);

    max_dist = 6000;
//    ui->TITLE->prog_settings->max_dist = 6000;
//    ui->TITLE->prog_settings->data_exchange(false);
//    ui->TITLE->prog_settings->max_dist_changed(6000);
    pref->max_dist = 6000;
    pref->data_exchange(false);
    pref->max_dist_changed(6000);

    ui->DUAL_LIGHT_BTN->hide();
#else
    ui->LOGO->hide();
#endif
}

UserPanel::~UserPanel()
{
    tp.stop();
    delete laser_settings;

    delete ui;
}

void UserPanel::data_exchange(bool read){
    if (read) {
//        device_idx = ui->DEVICE_SELECTION->currentIndex();
        calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 4 + 4;

        trigger_by_software = ui->SOFTWARE_CHECK->isChecked();
        image_3d = ui->IMG_3D_CHECK->isChecked();

        gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
        time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
        frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();
        save_location = ui->FILE_PATH_EDIT->text();

        switch (hz_unit) {
        // kHz
        case 0: rep_freq = ui->FREQ_EDIT->text().toFloat(); break;
        // Hz
        case 1: rep_freq = ui->FREQ_EDIT->text().toFloat() / 1000; break;
        default: break;
        }
        laser_width_u = ui->LASER_WIDTH_EDIT_U->text().toInt();
        laser_width_n = ui->LASER_WIDTH_EDIT_N->text().toInt();
        gate_width_a_u = ui->GATE_WIDTH_A_EDIT_U->text().toInt();
        gate_width_a_n = ui->GATE_WIDTH_A_EDIT_N->text().toInt();
        delay_a_u = ui->DELAY_A_EDIT_U->text().toInt();
        delay_a_n = ui->DELAY_A_EDIT_N->text().toInt();
//        gate_width_b_u = ui->GATE_WIDTH_B_EDIT_U->text().toInt();
//        gate_width_b_n = ui->GATE_WIDTH_B_EDIT_N->text().toInt();
        switch (base_unit) {
        // ns
        case 0: stepping = ui->STEPPING_EDIT->text().toFloat(); break;
        // μs
        case 1: stepping = ui->STEPPING_EDIT->text().toFloat() * 1000; break;
        // m
        case 2: stepping = ui->STEPPING_EDIT->text().toFloat() / dist_ns; break;
        default: break;
        }
        delay_b_u = ui->DELAY_B_EDIT_U->text().toInt();
        delay_b_n = ui->DELAY_B_EDIT_N->text().toInt();
        delay_n_n = ui->DELAY_N_EDIT_N->text().toInt();
        // use toFloat() since float representation in line edit
        fps = ui->CCD_FREQ_EDIT->text().toFloat();
        duty = ui->DUTY_EDIT->text().toFloat() * 1000;
        mcp = ui->MCP_EDIT->text().toInt();

        zoom = ui->ZOOM_EDIT->text().toInt();
        focus = ui->FOCUS_EDIT->text().toInt();

        laser_width = laser_width_u * 1000 + laser_width_n;
        delay_dist = std::round((delay_a_u * 1000 + delay_a_n) * dist_ns);
        depth_of_view = std::round((gate_width_a_u * 1000 + gate_width_a_n) * dist_ns);
    }
    else {
//        ui->DEVICE_SELECTION->setCurrentIndex(device_idx);
        ui->FRAME_AVG_OPTIONS->setCurrentIndex(calc_avg_option / 4 - 1);

        ui->SOFTWARE_CHECK->setChecked(trigger_by_software);
        ui->IMG_3D_CHECK->setChecked(image_3d);

        ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)gain_analog_edit));
        ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
        ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));
        ui->FILE_PATH_EDIT->setText(save_location);

        delay_a_u = std::round(delay_dist / dist_ns) / 1000;
        delay_a_n = (int)std::round(delay_dist / dist_ns) % 1000;
        delay_b_u = std::round(delay_dist / dist_ns + delay_n_n) / 1000;
        delay_b_n = (int)std::round(delay_dist / dist_ns + delay_n_n) % 1000;
        gate_width_a_u = std::round(depth_of_view / dist_ns) / 1000;
        gate_width_a_n = (int)std::round(depth_of_view / dist_ns) % 1000;
        laser_width_u = laser_width / 1000;
        laser_width_n = laser_width % 1000;

        setup_hz(hz_unit);
        ui->LASER_WIDTH_EDIT_U->setText(QString::asprintf("%d", laser_width_u));
        ui->LASER_WIDTH_EDIT_N->setText(QString::asprintf("%d", laser_width_n));
        ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf("%d", gate_width_a_u));
        ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%d", gate_width_a_n));
        ui->DELAY_A_EDIT_U->setText(QString::asprintf("%d", delay_a_u));
        ui->DELAY_A_EDIT_N->setText(QString::asprintf("%d", delay_a_n));
//        ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf("%d", gate_width_b_u));
//        ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%d", gate_width_b_n));
        ui->DELAY_B_EDIT_U->setText(QString::asprintf("%d", delay_b_u));
        ui->DELAY_B_EDIT_N->setText(QString::asprintf("%d", delay_b_n));
        ui->DELAY_N_EDIT_N->setText(QString::asprintf("%d", delay_n_n));
        ui->DELAY_SLIDER->setValue(delay_dist);
        ui->MCP_SLIDER->setValue(mcp);
        ui->MCP_EDIT->setText(QString::number(mcp));

        setup_stepping(base_unit);

        ui->ZOOM_EDIT->setText(QString::asprintf("%d", zoom));
        ui->FOCUS_EDIT->setText(QString::asprintf("%d", focus));
    }
}

int UserPanel::grab_thread_process() {
    grab_thread_state = true;
    Display *disp = ui->SOURCE_DISPLAY;
//    ProgSettings *settings = ui->TITLE->prog_settings;
    QImage stream;
    cv::Mat sobel;
    int ww, hh, scan_img_count = -1;
    float weight = h / 1024.0; // font scale & thickness
    prev_3d = cv::Mat(h, w, CV_8UC3);
    prev_img = cv::Mat(h, w, CV_MAKETYPE(pixel_depth == 8 ? CV_8U : CV_16U, is_color ? 3 : 1));
//    double *range = (double*)calloc(w * h, sizeof(double));
#ifdef LVTONG
    cv::Mat fishnet_res;
    cv::dnn::Net net = cv::dnn::readNet("model/resnet18.onnx");
#endif
    while (grab_image) {
        while (img_q.size() > 5) img_q.pop();

        if (img_q.empty()) {
#ifdef LVTONG
            QThread::msleep(10);
            continue;
#else
            img_mem = prev_img.clone();
            updated = false;
#endif
        }
        else {
            img_mem = img_q.front();
            img_q.pop();
            updated = true;
            if (device_type == -1) img_q.push(img_mem.clone());
        }

        image_mutex.lock();
//        qDebug () << QDateTime::currentDateTime().toString() << updated << img_mem.data;

        if (updated) {
            // calc histogram (grayscale)
            memset(hist, 0, 256 * sizeof(uint));
            if (!is_color) for (int i = 0; i < h; i++) for (int j = 0; j < w; j++) hist[(img_mem.data + i * img_mem.cols)[j]]++;

            // if the image needs flipping
            if (pref->symmetry) cv::flip(img_mem, img_mem, pref->symmetry - 2);

            // mcp self-adaptive
            if (pref->auto_mcp && !ui->MCP_SLIDER->hasFocus()) {
                int thresh_num = img_mem.total() / 200, thresh = 1 << pixel_depth;
                while (thresh && thresh_num > 0) thresh_num -= hist[thresh--];
                if (thresh > (1 << pixel_depth) * 0.94) emit update_mcp_in_thread(mcp - sqrt(thresh - (1 << pixel_depth) * 0.94));
//                qDebug() << "high" << thresh;
//                thresh_num = img_mem.total() / 100, thresh = 255;
//                while (thresh && thresh_num > 0) thresh_num -= hist[thresh--];
                if (thresh < (1 << pixel_depth) * 0.39) emit update_mcp_in_thread(mcp + sqrt((1 << pixel_depth) * 0.39 - thresh));
//                qDebug() << "low" << thresh;
            }
        }
/*
        // tenengrad (sobel) auto-focus
        cv::Sobel(img_mem, sobel, CV_16U, 1, 1);
        clarity[2] = clarity[1], clarity[1] = clarity[0], clarity[0] = cv::mean(sobel)[0] * 1000;
        if (focus_direction) {
            if (clarity[2] > clarity[1] && clarity[1] > clarity[0]) {
                focus_direction *= -2;
                // FIXME auto focus by traversing
                if (abs(focus_direction) > 7) lens_stop(), focus_direction = 0;
                else if (focus_direction > 0) lens_stop(), change_focus_speed(8), focus_far();
                else if (focus_direction < 0) lens_stop(), change_focus_speed(16), focus_near();
            }
        }
*/
#ifdef LVTONG
        double is_net;
        if (pref->fishnet_recog) {
            cv::cvtColor(img_mem, fishnet_res, cv::COLOR_GRAY2RGB);
            fishnet_res.convertTo(fishnet_res, CV_32FC3, 1.0 / 255);
            cv::resize(fishnet_res, fishnet_res, cv::Size(224, 224));

            cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(224, 224));
            net.setInput(blob);
            cv::Mat prob = net.forward("195");
//            std::cout << cv::format(prob, cv::Formatter::FMT_C) << std::endl;

            static double min, max;
            cv::minMaxLoc(prob, &min, &max);

            prob -= max;
            is_net = exp(prob.at<float>(1)) / (exp(prob.at<float>(0)) + exp(prob.at<float>(1)));
//            is_net = exp(prob.at<float>(1)) / exp(prob.at<float>(0) + prob.at<float>(1));

            emit update_fishnet_result(is_net > pref->fishnet_thresh);
        }
        else emit update_fishnet_result(-1);
#endif

        // process frame average
        if (seq_sum.empty()) seq_sum = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
        if (frame_a_sum.empty()) frame_a_sum = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
        if (frame_b_sum.empty()) frame_b_sum = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
        if (ui->FRAME_AVG_CHECK->isChecked()) {
            if (updated) {
                calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 4 + 4;
                if (seq[7].empty()) for (auto& m: seq) m = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));

                seq_sum -= seq[(seq_idx + 4) & 7];
//                seq_sum -= seq[seq_idx];
                if (frame_a_3d) frame_a_sum -= seq[seq_idx];
                else            frame_b_sum -= seq[seq_idx];
                img_mem.convertTo(seq[seq_idx], CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
                seq_sum += seq[seq_idx];
                if (frame_a_3d) frame_a_sum += seq[seq_idx];
                else            frame_b_sum += seq[seq_idx];
//                for(int i = 0; i < calc_avg_option; i++) seq_sum += seq[i];

//                seq_idx = (seq_idx + 1) % calc_avg_option;
                seq_idx = (seq_idx + 1) & 7;
            }
//            seq_sum.convertTo(modified_result, CV_8U, 1. / (calc_avg_option * (1 << (pixel_depth - 8))));
            seq_sum.convertTo(modified_result, CV_MAKETYPE(CV_8U, is_color ? 3 : 1), 1. / (4 * (1 << (pixel_depth - 8))));
        }
        else {
            img_mem.convertTo(modified_result, CV_MAKETYPE(CV_8U, is_color ? 3 : 1), 1. / (1 << (pixel_depth - 8)));
        }

        // process 3d image construction from ABN frames
        if (ui->IMG_3D_CHECK->isChecked()) {
            ww = w + 104;
            hh = h;
//            modified_result = frame_a_3d ? prev_3d : ImageProc::gated3D(prev_img, img_mem, delay_dist / dist_ns, depth_of_view / dist_ns, range_threshold);
//            if (prev_3d.empty()) cv::cvtColor(img_mem, prev_3d, cv::COLOR_GRAY2RGB);
            if (updated) {
                // TODO try to reduce if statements
                if (ui->FRAME_AVG_CHECK->isChecked()) {
                    static cv::Mat frame_a_avg, frame_b_avg;
                    frame_a_sum.convertTo(frame_a_avg, pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
                    frame_b_sum.convertTo(frame_b_avg, pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
                    ImageProc::gated3D_v2(frame_b_avg, frame_a_avg, modified_result, (delay_dist - pref->delay_dist_offset) / dist_ns,
                                          depth_of_view / dist_ns, pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d);
                }
                else {
                    if (frame_a_3d) ImageProc::gated3D_v2(prev_img, img_mem, modified_result, (delay_dist - pref->delay_dist_offset) / dist_ns,
                                                          depth_of_view / dist_ns, pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d);
                    else            ImageProc::gated3D_v2(img_mem, prev_img, modified_result, (delay_dist - pref->delay_dist_offset) / dist_ns,
                                                          depth_of_view / dist_ns, pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d);
                }
                frame_a_3d ^= 1;
                prev_3d = modified_result.clone();
            }
            else modified_result = prev_3d;

            cv::resize(modified_result, img_display, cv::Size(disp->width(), disp->height()), 0, 0, cv::INTER_AREA);
        }
        // process ordinary image enhance
        else {
            ww = w;
            hh = h;
            if (ui->IMG_ENHANCE_CHECK->isChecked()) {
                switch (ui->ENHANCE_OPTIONS->currentIndex()) {
                // histogram
                case 1: {
                    cv::equalizeHist(modified_result, modified_result);
                    break;
                }
                // laplace
                case 2: {
                    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, 0, 5, 0, 0, -1, 0);
                    cv::filter2D(modified_result, modified_result, CV_8U, kernel);
                    break;
                }
//                // log
//                case 3: {
//                    cv::Mat img_log;
//                    modified_result.convertTo(img_log, CV_32F);
//                    modified_result += 1.0;
//                    cv::log(img_log, img_log);
//                    img_log *= settings->log;
//                    cv::normalize(img_log, img_log, 0, 255, cv::NORM_MINMAX);
//                    cv::convertScaleAbs(img_log, modified_result);
//                    break;
//                }
                // SP
                case 3: {
                    ImageProc::plateau_equl_hist(&modified_result, &modified_result, 4);
                    break;
                }
                // accumulative
                case 4: {
//                    uchar *img = modified_result.data;
//                    for (int i = 0; i < h; i++) {
//                        for (int j = 0; j < w; j++) {
//                            uchar p = img[i * modified_result.step + j];
//                            if      (p < 64)  {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 2.4);}
//                            else if (p < 112) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.8);}
//                            else if (p < 144) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.6);}
//                            else if (p < 160) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.5);}
//                            else if (p < 176) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.4);}
//                            else if (p < 192) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.25);}
//                            else if (p < 200) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.2);}
//                            else if (p < 208) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.15);}
//                            else if (p < 216) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.125);}
//                            else if (p < 224) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.1);}
//                            else if (p < 240) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1.05);}
//                            else if (p < 256) {img[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * settings->accu_base * 1);}
//                        }
//                    }
                    ImageProc::accumulative_enhance(modified_result, modified_result, pref->accu_base);
                    break;
                }
                // TODO rewrite sigmoid enchance
                // sigmoid (nonlinear) (mergw log w/ 1/(1+exp))
                case 5: {
                    uchar *img = modified_result.data;
                    cv::Mat img_log, img_nonLT = cv::Mat(h, w, CV_8U);
                    modified_result.convertTo(img_log, CV_32F);
                    modified_result += 1.0;
                    cv::log(img_log, img_log);
//                    img_log *= settings->log;
                    img_log *= 1.2;
                    double m = 0, kv = 0, mean = cv::mean(modified_result)[0];
                    uchar p;
                    for (int i = 0; i < h; i++) {
                        for (int j = 0; j < w; j++) {
                            p = img[i * modified_result.step + j];
                            if (!p) {
                                img_nonLT.data[i * img_nonLT.step + j] = 0;
                                continue;
                            }
                            if      (p <=  60) kv = 7;
                            else if (p <= 200) kv = 7 + (p - 60) / 70;
                            else if (p <= 255) kv = 9 + (p - 200) / 55;
                            m = kv * (p / (p + mean));
                            img_nonLT.data[i * img_nonLT.step + j] = (int)(2 / (1 + exp(-m)) - 1) * 255 - p;
                        }
                    }
                    cv::normalize(img_log, img_log, 0, 255, cv::NORM_MINMAX);
                    cv::convertScaleAbs(img_log, img_log);
                    modified_result = 0.05 * img_log + 0.05 * img_nonLT + 0.8 * modified_result;
                    break;
                }
                // adaptive
                case 6: {
    //                double low = settings->low_in * 255, high = settings->high_in * 255; // (0, 12.75)
    //                double bottom = settings->low_out * 255, top = settings->high_out * 255; // (0, 255)
    //                double err_in = high - low, err_out = top - bottom; // (12.75, 255)
    //                // cv::pow((modified_result - low) / err_in, gamma, modified_result);
    //                cv::Mat temp;
    //                modified_result.convertTo(temp, CV_32F);
    //                cv::pow((temp - low) / err_in, settings->gamma, temp);
    //                temp = temp * err_out + bottom;
    //                cv::normalize(temp, temp, 0, 255, cv::NORM_MINMAX);
    //                cv::convertScaleAbs(temp, modified_result);
                    ImageProc::adaptive_enhance(modified_result, modified_result, pref->low_in, pref->high_in, pref->low_out, pref->high_out, pref->gamma);
                    break;
                }
                // enhance_dehaze
                case 7: {
                    modified_result = ~modified_result;
                    ImageProc::haze_removal(modified_result, modified_result, 7, pref->dehaze_pct, 0.1, 60, 0.01);
                    modified_result = ~modified_result;
                    break;
                }
                // dcp
                case 8: {
                    ImageProc::haze_removal(modified_result, modified_result, 7, pref->dehaze_pct, 0.1, 60, 0.01);
                }
                // none
                default:
                    break;
                }
            }
            // process special image enhance
//            if (ui->SP_CHECK->isChecked()) ImageProc::plateau_equl_hist(&modified_result, &modified_result, ui->SP_OPTIONS->currentIndex());

            // brightness & contrast
////            float brightness = ui->BRIGHTNESS_SLIDER->value() / 10., contrast = ui->CONTRAST_SLIDER->value() / 10.;
////            modified_result = (modified_result - 127.5 * (1 - brightness)) * std::tan((45 - 45 * contrast) / 180. * M_PI) + 127.5 * (1 + brightness);
//            int val = ui->BRIGHTNESS_SLIDER->value() * 12.8;
//            modified_result *= std::exp(ui->CONTRAST_SLIDER->value() / 10.);
//            modified_result += val;
//            // do not change pixel of value 0 when adjusting brightness
////            int temp = 127.5 * brightness * (1 + std::tan((45 - 45 * contrast) / 180. * M_PI)) + 127.5 * (1 - std::tan((45 - 45 * contrast) / 180. * M_PI));
//            for (int i = 0; i < hh; i++) for (int j = 0; j < ww; j++) {
//                if (modified_result.data[i * ww + j] == val) modified_result.data[i * ww + j] = 0;
//            }
            ImageProc::brightness_and_contrast(modified_result, modified_result, std::exp(ui->CONTRAST_SLIDER->value() / 10.), ui->BRIGHTNESS_SLIDER->value() * 12.8);
            // map [0, 20] to [0, +inf) using tan
            ImageProc::brightness_and_contrast(modified_result, modified_result, tan((20 - ui->GAMMA_SLIDER->value()) / 40. * M_PI));

            // display grayscale histogram of current image
            if (ui->HISTOGRAM_RADIO->isChecked() && modified_result.channels() == 1) {
                uchar *img = modified_result.data;
                int step = modified_result.step;
                memset(hist, 0, 256 * sizeof(uint));
                for (int i = 0; i < hh; i++) for (int j = 0; j < ww; j++) hist[(img + i * step)[j]]++;
                uint max = 0;
                for (int i = 1; i < 256; i++) {
                    // discard abnormal value
//                    if (hist[i] > 50000) hist[i] = 0;
                    if (hist[i] > max) max = hist[i];
                }
                cv::Mat hist_image(225, 256, CV_8UC3, cv::Scalar(56, 64, 72));
                for (int i = 0; i < 256; i++) {
                    cv::rectangle(hist_image, cv::Point(i, 225), cv::Point(i + 1, 225 - hist[i] * 225.0 / max), cv::Scalar(202, 225, 255));
                }
                // TODO change to signal/slots
                ui->HIST_DISPLAY->setPixmap(QPixmap::fromImage(QImage(hist_image.data, hist_image.cols, hist_image.rows, hist_image.step, QImage::Format_RGB888).scaled(ui->HIST_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            }
            // FIXME possible crash when move scaled image to bottom-right corner
            // crop the region to display
//            cv::Rect region = cv::Rect(disp->display_region.tl() * (image_3d ? ww + 104 : ww) / disp->width(), disp->display_region.br() * (image_3d ? ww + 104 : ww) / disp->width());
            cv::Rect region = cv::Rect(disp->display_region.tl() * ww / disp->width(), disp->display_region.br() * ww / disp->width());
            if (region.height > hh) region.height = hh;
//            qDebug("region x: %d y: %d, w: %d, h: %d\n", region.x, region.y, region.width, region.height);
//            qDebug("image  w: %d, h: %d\n", modified_result.cols, modified_result.rows);
            modified_result = modified_result(region);
            cv::resize(modified_result, modified_result, cv::Size(ww, hh));

            // put info (dist, dov, time) as text on image
            if (ui->INFO_CHECK->isChecked()) {
                if (base_unit == 2) cv::putText(modified_result, QString::asprintf("DIST %05d m DOV %04d m", (int)delay_dist, (int)depth_of_view).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                else cv::putText(modified_result, QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(delay_dist / dist_ns), (int)std::round(depth_of_view / dist_ns)).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                cv::putText(modified_result, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
                static int baseline = 0;
                if (pref->fishnet_recog) {
//                    cv::putText(modified_result, "FISHNET", cv::Point(ww - 40 - cv::getTextSize("FISHNET", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                    cv::putText(modified_result, is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                }
#endif
            }

            // resize to display size
            cv::resize(modified_result, img_display, cv::Size(disp->width(), disp->height()), 0, 0, cv::INTER_AREA);
            // draw the center cross
            if (!image_3d && ui->CENTER_CHECK->isChecked()) {
                for (int i = img_display.cols / 2 - 9; i < img_display.cols / 2 + 10; i++) img_display.at<uchar>(img_display.rows / 2, i) = img_display.at<uchar>(img_display.rows / 2, i) > 127 ? 0 : 255;
                for (int i = img_display.rows / 2 - 9; i < img_display.rows / 2 + 10; i++) img_display.at<uchar>(i, img_display.cols / 2) = img_display.at<uchar>(i, img_display.cols / 2) > 127 ? 0 : 255;
            }
            if (ui->SELECT_TOOL->isChecked() && disp->selection_v1 != disp->selection_v2) cv::rectangle(img_display, disp->selection_v1, disp->selection_v2, cv::Scalar(255));
        }

        // image display
//        stream = QImage(cropped_img.data, cropped_img.cols, cropped_img.rows, cropped_img.step, QImage::Format_RGB888);
        stream = QImage(img_display.data, img_display.cols, img_display.rows, img_display.step, image_3d || is_color ? QImage::Format_RGB888 : QImage::Format_Indexed8);
//        ui->SOURCE_DISPLAY->setPixmap(QPixmap::fromImage(stream.scaled(ui->SOURCE_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        // use signal->slot instead of directly call
        emit set_pixmap(QPixmap::fromImage(stream.scaled(disp->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        // process scan
        if (scan && updated) {
            static cv::Mat temp;
            if (scan_img_count < 0) scan_img_count = ui->FRAME_AVG_CHECK->isChecked() ? 5 : 1;
            scan_img_count -= 1;
            if (!scan_img_count) {
                save_scan_img();

                delay_dist += scan_step;
                scan_img_count = ui->FRAME_AVG_CHECK->isChecked() ? 5 : 1;
//                filter_scan();

                modified_result.convertTo(temp, CV_64F);
                cv::threshold(temp, temp, 20, 255, cv::THRESH_TOZERO);
                scan_3d += temp * (++scan_idx);
                scan_sum += temp;

                emit update_delay_in_thread();
            }
        }
        if (scan && std::round(delay_dist / dist_ns) > scan_stopping_delay) {on_SCAN_BUTTON_clicked();}

        // image write / video record
        if (updated && save_original) {
            save_to_file(false);
            if (device_type == -1) save_original = 0;
        }
        if (updated && save_modified) {
            save_to_file(true);
            if (device_type == -1) save_modified = 0;
        }
        if (updated && record_original) {
            cv::Mat temp = img_mem.clone();
            if (base_unit == 2) cv::putText(temp, QString::asprintf("DIST %05d m DOV %04d m", (int)delay_dist, (int)depth_of_view).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            else cv::putText(temp, QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(delay_dist / dist_ns), (int)std::round(depth_of_view / dist_ns)).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            cv::putText(temp, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(ww - 240, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), weight * 2);
            if (is_color) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
            vid_out[0].write(temp);
        }
        if (updated && record_modified) {
            cv::Mat temp = modified_result.clone();
            if (!image_3d && !ui->INFO_CHECK->isChecked()) {
                if (base_unit == 2) cv::putText(modified_result, QString::asprintf("DIST %05d m DOV %04d m", (int)delay_dist, (int)depth_of_view).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                else cv::putText(modified_result, QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(delay_dist / dist_ns), (int)std::round(depth_of_view / dist_ns)).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                cv::putText(modified_result, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
                static int baseline = 0;
                if (pref->fishnet_recog) {
//                    cv::putText(modified_result, "FISHNET", cv::Point(ww - 40 - cv::getTextSize("FISHNET", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                    cv::putText(modified_result, is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                }
#endif
            }
            if (is_color || image_3d) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
            vid_out[1].write(temp);
        }

        if (updated) prev_img = img_mem.clone();
        image_mutex.unlock();
    }
//    free(range);
    grab_thread_state = false;
    return 0;
}

inline bool UserPanel::is_maximized()
{
    return ui->TITLE->is_maximized;
}

// used when moving temp recorded vid to destination
void UserPanel::move_to_dest(QString src, QString dst)
{
    QFile::rename(src, dst);
}

void UserPanel::save_image_bmp(cv::Mat img, QString filename)
{
//    QPixmap::fromImage(QImage(img.data, img.cols, img.rows, img.step, img.channels() == 3 ? QImage::Format_RGB888 : QImage::Format_Indexed8)).save(filename + "_qt", "BMP", 100);

    if (img.channels() == 3) cv::cvtColor(img, img, cv::COLOR_BGRA2RGB);
/*
    // using qt file io
    QFile f(filename);
    f.open(QIODevice::WriteOnly);
    if (f.isOpen()) {
//        qDebug() << filename;
        QDataStream out(&f);
        out.setByteOrder(QDataStream::LittleEndian);
        quint32_le offset((img.channels() == 1 ? 4 * (1 << img.channels() * 8) : 0) + 54);
        // bmp file header
        out << (quint16_le)'MB' << (quint32_le)(img.total() * img.channels() + offset) << (quint16_le)0 << (quint16_le)0 << offset;
        //  windows signature,  total file size,                                       reserved1,       reserved2,       offset to data
        // DIB header
        out << (quint32_le)40 << (quint32_le)img.cols << (quint32_le)img.rows << (quint16_le)1 << (quint16_le)(img.channels() * 8) << (quint32_le)0 << (quint32_le)(img.total() * img.channels()) << (quint32_le)0 << (quint32_le)0 << (quint32_le)0 << (quint32_le)0;
        //  dib header size,  width,                  height,                 planes,          bit per pixel,                      compression,     image size,                                   x pixels in meter, y,             color important, color used
        // color table
        if (img.channels() == 1) {
            for (int i = 0; i < 1 << img.channels() * 8; i++) out << (quint8)i << (quint8)i << (quint8)i << (quint8)0;
        //                                                        rgb blue,    rgb green,   rgb red,     reserved
        }
        for(int i = img.rows - 1; i >= 0; i--) f.write((char*)img.data + i * img.step, img.step);
        f.close();
    }
*/
    // using std file io
    FILE *f = fopen(filename.toLocal8Bit().constData(), "wb");
    if (f) {
        static ushort signature = 'MB';
        fwrite(&signature, 2, 1, f);
        uint offset = (img.channels() == 1 ? 4 * (1 << img.channels() * 8) : 0) + 54;
        uint file_size = img.total() * img.channels() + offset;
        fwrite(&file_size, 4, 1, f);
        static ushort reserved1 = 0, reserved2 = 0;
        fwrite(&reserved1, 2, 1, f);
        fwrite(&reserved2, 2, 1, f);
        fwrite(&offset, 4, 1, f);
        static uint dib_size = 40;
        fwrite(&dib_size, 4, 1, f);
        uint w = img.cols, h = img.rows;
        fwrite(&w, 4, 1, f);
        fwrite(&h, 4, 1, f);
        static ushort planes = 1;
        fwrite(&planes, 2, 1, f);
        ushort bit_per_pixel = 8 * img.channels();
        fwrite(&bit_per_pixel, 2, 1, f);
        static uint compression = 0;
        fwrite(&compression, 4, 1, f);
        uint img_size = img.total() * img.channels();
        fwrite(&img_size, 4, 1, f);
        static uint pixels_in_meter_x = 0, pixels_in_meter_y = 0;
        fwrite(&pixels_in_meter_x, 4, 1, f);
        fwrite(&pixels_in_meter_y, 4, 1, f);
        static uint colors_important = 0, colors_used = 0;
        fwrite(&colors_important, 4, 1, f);
        fwrite(&colors_used, 4, 1, f);

        // color table
        static uchar empty = 0;
        if (img.channels() == 1) {
            for (int i = 0; i < 1 << img.channels() * 8; i++) {
                fwrite(&i, 1, 1, f);     // r
                fwrite(&i, 1, 1, f);     // g
                fwrite(&i, 1, 1, f);     // b
                fwrite(&empty, 1, 1, f); // null
            }
        }
        for(int i = img.rows - 1; i >= 0; i--) fwrite(img.data + i * img.step, 1, img.step, f);
        fclose(f);
    }
}

void UserPanel::save_image_tif(cv::Mat img, QString filename)
{
    uint32_t w = img.cols, h = img.rows, channel = img.channels();
    uint32_t depth = 8 * (img.depth() / 2 + 1);
    FILE *f = fopen(filename.toLocal8Bit().constData(), "wb");
    static uint16_t type_short = 3, type_long = 4, type_fraction = 5;
    static uint32_t count_1 = 1, count_3 = 3;
    static uint32_t TWO = 2;
    static uint32_t next_ifd_offset = 0;
    uint64_t block_size = w * channel, sum = h * block_size * depth / 8;
    uint i;

    // Offset=w*h*d + 8(eg:Img[1000][1000][3] --> 3000008)
    // RGB  Color:W H BitsPerSample Compression Photometric StripOffset Orientation SamplePerPixle RowsPerStrip StripByteCounts PlanarConfiguration
    // Gray Color:W H BitsPerSample Compression Photometric StripOffset Orientation SamplePerPixle RowsPerStrip StripByteCounts XResolution YResoulution PlanarConfiguration ResolutionUnit
    // DE_N ID:   0 1 2             3           4           5           6           7              8            9               10          11           12                  13

    fseek(f, 0, SEEK_SET);
    static uint16_t endian = 'II';
    fwrite(&endian, 2, 1, f);
    static uint16_t tiff_magic_number = 42;
    fwrite(&tiff_magic_number, 2, 1, f);
    uint32_t ifd_offset = sum + 8; // 8 (IFH size)
    fwrite(&ifd_offset, 4, 1, f);

    // Insert the image data
    fwrite(img.data, 2, h * block_size, f);

//    fseek(f, ifd_offset, SEEK_SET);
    static uint16_t num_de = 14;
    fwrite(&num_de, 2, 1, f);
    // 256 image width
    static uint16_t tag_w = 256;
    fwrite(&tag_w, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&w, 4, 1, f);

    // 257 image height
    static uint16_t tag_h = 257;
    fwrite(&tag_h, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&h, 4, 1, f);

    // 258 bits per sample
    static uint16_t tag_bits_per_sample = 258;
    fwrite(&tag_bits_per_sample, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    switch (channel) {
    case 1: {
        fwrite(&count_1, 4, 1, f);
        fwrite(&depth, 4, 1, f);
        break;
    }
    case 3: {
        fwrite(&count_3, 4, 1, f);
        // 8 (IFH size) + (2 + 14 * 12 + 4) (0th IFD size: 2->size of the num of DE = 14, 12->size of one DE, 4->size of offset of next IFD) = 182
        uint32_t tag_bits_per_sample_offset = sum + 182;
        fwrite(&tag_bits_per_sample_offset, 4, 1, f);
        break;
    }
    default: break;
    }

    // 259 compression
    static uint16_t tag_compression = 259;
    fwrite(&tag_compression, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // use 1 for uncompressed

    // 262 photometric interpretation
    static uint16_t tag_photometric_interpretation = 262;
    fwrite(&tag_photometric_interpretation, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(channel == 1 ? &count_1 : &TWO, 4, 1, f); // use 1 for black is zero, use 2 for RGB

    // 273 strip offsets
    static uint16_t tag_strip_offsets = 273;
    fwrite(&tag_strip_offsets, 2, 1, f);
    fwrite(&type_long, 2, 1, f);
/*  treat every row as a single strip
    fwrite(&h, 4, 1, f);
    // 1 channel
    // 8 (IFH size) + (2 + 14 * 12 + 4) (0th IFD size: 2->size of the num of DE = 14, 12->size of one DE, 4->size of offset of next IFD) = 182
    // 3 channels
    // 182 (bits per sample offsets) + 3 * 2 (3 channels * 2 bytes) = 188
    uint tag_strip_offsets_offset = channel == 1 ? sum + 182 : sum + 188;
    fwrite(&tag_strip_offsets_offset, 4, 1, f);
*/
    fwrite(&count_1, 4, 1, f);
    static uint32_t strip_offset = 8;
    fwrite(&strip_offset, 4, 1, f);

    // 274 orientation
    static uint16_t tag_orientation = 274;
    fwrite(&tag_orientation, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // 1 = The 0th row represents the visual top of the image, and the 0th column represents the visual left-hand side.

    // 277 samples per pixel
    static uint16_t tag_samples_per_pixel = 277;
    fwrite(&tag_samples_per_pixel, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(channel == 1 ? &count_1 : &count_3, 4, 1, f); // use 1 for grayscale image, use 3 for 3-channel RGB image

    // 278 rows per strip
    static uint16_t tag_rows_per_strip = 278;
    fwrite(&tag_rows_per_strip, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&h, 4, 1, f); // all rows in one strip

    // 279 strip byte counts
    static uint16_t tag_strip_byte_counts = 279;
    fwrite(&tag_strip_byte_counts, 2, 1, f);
    fwrite(&type_long, 2, 1, f);
/*  treat every row as a single strip
    fwrite(&h, 4, 1, f);
    // 1 channel
    // 182 (offset of strip offsets), 4 * h (length of tag 273, strip_offsets)
    // 3 channels
    // 188 (offset of strip offsets), 4 * h (length of tag 273, strip_offsets)
    uint tag_strip_byte_counts_offset = channel == 1 ? sum + 182 + 4 * h : sum + 188 + 4 * h;
    fwrite(&tag_strip_byte_counts_offset, 4, 1, f);
*/
    fwrite(&count_1, 4, 1, f);
    fwrite(&sum, 4, 1, f);

    // 282 X resolution
    static uint16_t tag_x_resolution = 282;
    fwrite(&tag_x_resolution, 2, 1, f);
    fwrite(&type_fraction, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
/*  treat every row as a single strip
    uint tag_x_resolution_offset = channel == 1 ? sum + 182 + 8 * h : sum + 188 + 8 * h;
    fwrite(&tag_x_resolution_offset, 4, 1, f);
*/
    uint32_t tag_x_resolution_offset = channel == 1 ? sum + 182 : sum + 188;
    fwrite(&tag_x_resolution_offset, 4, 1, f);

    // 283 Y resolution
    static uint16_t tag_y_resolution = 283;
    fwrite(&tag_y_resolution, 2, 1, f);
    fwrite(&type_fraction, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
/*  treat every row as a single strip
    uint tag_y_resolution_offset = channel == 1 ? sum + 190 + 8 * h : sum + 196 + 8 * h;
    fwrite(&tag_y_resolution_offset, 4, 1, f);
*/
    uint32_t tag_y_resolution_offset = channel == 1 ? sum + 190 : sum + 196;
    fwrite(&tag_y_resolution_offset, 4, 1, f);

    // 284 planar configuration
    static uint16_t tag_planar_configuration = 284;
    fwrite(&tag_planar_configuration, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // use 1 for chunky format

    // 296 resolution unit
    static uint16_t tag_resolution_unit = 296;
    fwrite(&tag_resolution_unit, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&TWO, 4, 1, f); // use 2 for inch

    fwrite(&next_ifd_offset, 4, 1, f);

    // (rgb only) value for tag 258, offset: sum + 182
    if (channel == 3) for(i = 0; i < 3; i++) fwrite(&depth, 2, 1, f);

/*  treat every row as a single strip
    // value for tag 273, offset: sum + 182 (gray), sum + 188 (rgb)
    uint strip_offset = 8;
    for(i = 0; i < h; i++, strip_offset += block_size) fwrite(&strip_offset, 4, 1, f);

    // value for tag 279, offset: sum + 182 + 4 * h (gray), sum + 188 + 4 * h (rgb)
    for(i = 0; i < h; i++) fwrite(&block_size, 4, 1, f);
*/

    // value for tag 282, offset: sum + 182 + 8 * h (gray), sum + 188 + 8 * h (rgb)
    static uint32_t res_physical = 1, res_pixel = 72;
    fwrite(&res_pixel, 4, 1, f); // fraction w-pixel
    fwrite(&res_physical, 4, 1, f); // denominator PhyWidth-inch

    // value for tag 283, offset: sum + 190 + 8 * h (gray), sum + 196 + 8 * h (rgb)
    fwrite(&res_pixel, 4, 1, f); // fraction h-pixel
    fwrite(&res_physical,4, 1, f); // denominator PhyHeight-inch

    fclose(f);
}

bool UserPanel::load_image_tif(cv::Mat &img, QString filename)
{
    // TODO test motorola tiff read function
    static uint16_t byte_order;
    static bool is_big_endian;
    static uint32_t ifd_offset;
    static uint16_t num_de;
    static uint16_t tag_number, tag_data_type;
    static uint32_t tag_data_num, tag_data;
    static int width, height, depth;
    FILE *f = fopen(filename.toLocal8Bit().constData(), "rb");

    // tiff header
    fseek(f, 0, SEEK_SET);
    fread(&byte_order, 2, 1, f);
    is_big_endian = byte_order == 'MM';
    fread(&ifd_offset, 2, 1, f); // magic number 42
    fread(&ifd_offset, 4, 1, f);
    if (is_big_endian) ifd_offset = qFromBigEndian(ifd_offset);

    // skip image data before fetching image info
    fseek(f, ifd_offset, SEEK_SET);
//    qDebug() << "ifd_offset " << ifd_offset;

    // IFH data
    fread(&num_de, 2, 1, f);
    if (is_big_endian) num_de = qFromBigEndian(num_de);
    for (uint16_t c = 0; c < num_de; c++) {
        // get tag #
        fread(&tag_number, 2, 1, f);
        if (is_big_endian) tag_number = qFromBigEndian(tag_number);
        // get tag type
        fread(&tag_data_type, 2, 1, f);
        if (is_big_endian) tag_data_type = qFromBigEndian(tag_data_type);
        // get tag data num
        fread(&tag_data_num, 4, 1, f);
        if (is_big_endian) tag_data_num = qFromBigEndian(tag_data_num);
        // get tag data (or ptr offset)
        fread(&tag_data, 4, 1, f);
        if (is_big_endian) tag_data = qFromBigEndian(tag_data) / (tag_data_type == 3 ? 1 << 16 : 1);

//        qDebug() << tag_number << tag_data_type << tag_data_num << hex << tag_data;
        switch (tag_number) {
        case 256: width = tag_data; break;
        case 257: height = tag_data; break;
        case 258:
            if (tag_data_num != 1) return -1;
            depth = tag_data;
            break;
        case 259:
            if (tag_data != 1) return -1;
            break;
        case 279:
            if (width * height * depth / 8 != tag_data) return -1;
            break;
        default: break;
        }
    }

    img = cv::Mat(height, width, depth == 8 ? CV_8U : CV_16U);
    fseek(f, 8, SEEK_SET);
    fread(img.data, 2, height * width, f);
    if (is_big_endian) img = (img & 0xFF00) / (1 << 8) + (img & 0x00FF) * (1 << 8);

    return 0;
}

void UserPanel::set_theme()
{
    app_theme ^= 1;

    if (app_theme) qApp->setStyleSheet(theme_light);
    else qApp->setStyleSheet(theme_dark);

    cursor_curr_pointer   = app_theme ? cursor_light_pointer   : cursor_dark_pointer;
    cursor_curr_resize_h  = app_theme ? cursor_light_resize_h  : cursor_dark_resize_h;
    cursor_curr_resize_v  = app_theme ? cursor_light_resize_v  : cursor_dark_resize_v;
    cursor_curr_resize_md = app_theme ? cursor_light_resize_md : cursor_dark_resize_md;
    cursor_curr_resize_sd = app_theme ? cursor_light_resize_sd : cursor_dark_resize_sd;

    ui->UP_LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up_left"));
    ui->UP_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up"));
    ui->UP_RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up_right"));
    ui->LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/left"));
    ui->SELF_TEST_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    ui->RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/right"));
    ui->DOWN_LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down_left"));
    ui->DOWN_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down"));
    ui->DOWN_RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down_right"));
    ui->RESET_3D_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));

//    for (int i = 0 ; i < 5; i++) if (serial_port_connected[i]) com_label[i]->setStyleSheet(app_theme ? "color: #180D1C;" : "color: #B0C4DE;");
    ptr_tcu->set_theme();
    ptr_lens->set_theme();
    ptr_laser->set_theme();
    ptr_ptz->set_theme();

    QFont temp_f(consolas);
    temp_f.setPixelSize(11);
    ui->DATA_EXCHANGE->setFont(temp_f);

    setCursor(cursor_curr_pointer);

    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s/%s);", app_theme ? "light" : "dark", hide_left ? "right" : "left"));
}

void UserPanel::stop_image_writing()
{
    if (save_original) on_SAVE_BMP_BUTTON_clicked();
    if (save_modified) on_SAVE_RESULT_BUTTON_clicked();
    QMessageBox::warning(wnd, "PROMPT", "too much memory used, stopping writing images");
//    QElapsedTimer t;
//    t.start();
//    while (t.elapsed() < 1000) ((QApplication*)QApplication::instance())->processEvents();
//    QThread::msleep(1000);
}

// FIXME font change when switching language
void UserPanel::switch_language()
{
    en ? (qApp->removeTranslator(&trans), qApp->setFont(monaco)) : (qApp->installTranslator(&trans), qApp->setFont(consolas));
    ui->retranslateUi(this);
//    ui->TITLE->prog_settings->switch_language(en, &trans);
    pref->switch_language(en, &trans);
//    int idx = ui->ENHANCE_OPTIONS->currentIndex();
//    ui->ENHANCE_OPTIONS->clear();
//    ui->ENHANCE_OPTIONS->addItem(tr("None"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Histogram"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Laplace"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Log-based"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Gamma-based"));
//    ui->ENHANCE_OPTIONS->setCurrentIndex(idx);
    en ^= 1;
}

void UserPanel::closeEvent(QCloseEvent *event)
{
    shut_down();
    event->accept();
//    ui->TITLE->prog_settings->reject();
    pref->reject();
    laser_settings->reject();
    scan_config->reject();
    if (QFile::exists("HQVSDK.xml")) QFile::remove("HQVSDK.xml");
}

int UserPanel::shut_down() {
    if (record_original) on_SAVE_AVI_BUTTON_clicked();
    if (record_modified) on_SAVE_FINAL_BUTTON_clicked();

    grab_image = false;
    if (h_grab_thread) {
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
    }

    if (!img_q.empty()) std::queue<cv::Mat>().swap(img_q);

    if (curr_cam) curr_cam->shut_down();
    if (curr_cam) delete curr_cam, curr_cam = NULL;

    start_grabbing = false;
    device_on = false;

    return 0;
}

// enable buttons
void UserPanel::enable_controls(bool cam_rdy) {
//    return;
    ui->ENUM_BUTTON->setEnabled(!device_on);
    ui->START_BUTTON->setEnabled(!device_on && cam_rdy);
    ui->SHUTDOWN_BUTTON->setEnabled(device_on && cam_rdy);
    ui->START_GRABBING_BUTTON->setEnabled(!(start_grabbing && cam_rdy) && device_on);
    ui->STOP_GRABBING_BUTTON->setEnabled(start_grabbing);
    ui->SAVE_BMP_BUTTON->setEnabled(start_grabbing);
    ui->SAVE_RESULT_BUTTON->setEnabled(start_grabbing);
    ui->SAVE_AVI_BUTTON->setEnabled(start_grabbing);
    ui->SAVE_FINAL_BUTTON->setEnabled(start_grabbing);
    ui->GAIN_EDIT->setEnabled(device_on);
//    ui->DUTY_EDIT->setEnabled(device_on);
//    ui->CCD_FREQ_EDIT->setEnabled(device_on);
    ui->GAIN_SLIDER->setEnabled(device_on);
    ui->GET_PARAMS_BUTTON->setEnabled(device_on);
    ui->SET_PARAMS_BUTTON->setEnabled(device_on);
    ui->CONTINUOUS_RADIO->setEnabled(device_on && !start_grabbing);
    ui->TRIGGER_RADIO->setEnabled(device_on && !start_grabbing);
    ui->BINNING_CHECK->setEnabled(device_on && !start_grabbing);
    ui->SOFTWARE_CHECK->setEnabled(device_on && !start_grabbing && trigger_mode_on);
    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(start_grabbing && trigger_mode_on && trigger_by_software);
    ui->IMG_REGION_BTN->setEnabled(device_on);
    ui->IMG_3D_CHECK->setEnabled(start_grabbing);
    ui->IMG_ENHANCE_CHECK->setEnabled(start_grabbing);
    ui->FRAME_AVG_CHECK->setEnabled(start_grabbing);
    ui->ENHANCE_OPTIONS->setEnabled(start_grabbing);
    ui->FRAME_AVG_OPTIONS->setEnabled(start_grabbing);
    ui->RANGE_THRESH_EDIT->setEnabled(start_grabbing);
//    ui->BRIGHTNESS_LABEL->setEnabled(start_grabbing);
    ui->BRIGHTNESS_SLIDER->setEnabled(start_grabbing);
//    ui->CONTRAST_LABEL->setEnabled(start_grabbing);
    ui->CONTRAST_SLIDER->setEnabled(start_grabbing);
    ui->GAMMA_SLIDER->setEnabled(start_grabbing);
    ui->SCAN_BUTTON->setEnabled(start_grabbing);
//    ui->SCAN_BUTTON->setEnabled(start_grabbing || device_type > 0);
    ui->INFO_CHECK->setEnabled(start_grabbing);
    ui->CENTER_CHECK->setEnabled(start_grabbing);
}

void UserPanel::save_to_file(bool save_result) {
//    QString temp = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"),
//            dest = QString(save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//    cv::imwrite(temp.toLatin1().data(), save_result ? modified_result : img_mem);
//    QFile::rename(temp, dest);

    cv::Mat *temp = save_result ? &modified_result : &img_mem;
//    QPixmap::fromImage(QImage(temp->data, temp->cols, temp->rows, temp->step, temp->channels() == 3 ? QImage::Format_RGB888 : QImage::Format_Indexed8)).save(save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp", "BMP", 100);

//    std::thread t_save(UserPanel::save_image_bmp, *temp, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//    t_save.detach();

//    if (!tp.append_task(std::bind(UserPanel::save_image_tif, tif_16, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full();
//    if (device_type < 0) {
//        UserPanel::save_image_bmp(temp->clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//        return;
//    }
    // TODO implement 16bit result image processing/writing
    if (save_result) {
        if (!tp.append_task(std::bind(UserPanel::save_image_bmp, temp->clone(), save_location + "/res_bmp/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"))) emit task_queue_full();
    } else {
        switch (pixel_depth) {
        case  8: if (!tp.append_task(std::bind(UserPanel::save_image_bmp, temp->clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"))) emit task_queue_full(); break;
        case 10:
        case 12:
        case 16: if (!tp.append_task(std::bind(UserPanel::save_image_tif, temp->clone() * (1 << 16 - pixel_depth), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full(); break;
        default: break;
        }
    }
}

void UserPanel::save_scan_img() {
//    QString temp = QString(TEMP_SAVE_LOCATION + "/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp"),
//            dest = QString(save_location + "/" + scan_name + "/ori_bmp/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp");
//    cv::imwrite(temp.toLatin1().data(), img_mem);
//    QFile::rename(temp, dest);
//    if (ui->TITLE->prog_settings->save_scan_ori) QPixmap::fromImage(QImage(img_mem.data, img_mem.cols, img_mem.rows, img_mem.step, QImage::Format_Indexed8)).save(save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp", "BMP");
    if (scan_config->capture_scan_ori) {
//        std::thread t_ori(save_image_bmp, img_mem, save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp");
//        t_ori.detach();
        tp.append_task(std::bind(save_image_bmp, img_mem.clone(), save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp"));
    }
//    dest = QString(save_location + "/" + scan_name + "/res_bmp/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp");
//    cv::imwrite(temp.toLatin1().data(), modified_result);
//    QFile::rename(temp, dest);
//    if (ui->TITLE->prog_settings->save_scan_res) QPixmap::fromImage(QImage(modified_result.data, modified_result.cols, modified_result.rows, modified_result.step, QImage::Format_Indexed8)).save(save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp", "BMP");
    if (scan_config->capture_scan_res) {
//        std::thread t_res(save_image_bmp, modified_result, save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp");
//        t_res.detach();
        tp.append_task(std::bind(save_image_bmp, modified_result.clone(), save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp"));
    }
}

void UserPanel::setup_serial_port(QSerialPort **port, int id, QString port_num, int baud_rate) {
    (*port)->setPortName("COM" + port_num);
//    qDebug("%p", *com);

    if ((*port)->open(QIODevice::ReadWrite)) {
        QThread::msleep(10);
        (*port)->clear();
        qDebug("COM%s connected\n", qPrintable(port_num));
        serial_port_connected[id] = true;
        com_label[id]->setStyleSheet(app_theme ? "color: #180D1C;" : "color: #B0C4DE;");

        (*port)->setBaudRate(baud_rate);
        (*port)->setDataBits(QSerialPort::Data8);
        (*port)->setParity(QSerialPort::NoParity);
        (*port)->setStopBits(QSerialPort::OneStop);
        (*port)->setFlowControl(QSerialPort::NoFlowControl);

        // send initial data
        switch (id) {
        case 0:
//            convert_to_send_tcu(0x01, (laser_width_u * 1000 + laser_width_n + 8) / 8);
            ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
            update_delay();
            update_gate_width();
            change_mcp(5);
            break;
        case 2:
            on_GET_LENS_PARAM_BTN_clicked();
            change_focus_speed(ui->FOCUS_SPEED_EDIT->text().toInt());
            break;
        default:
            break;
        }
        pref->display_baudrate(id, baud_rate);
    }
    else {
        serial_port_connected[id] = false;
        com_label[id]->setStyleSheet("color: #CD5C5C;");
    }

    // TODO: test for user utilities
    FILE *f = fopen("user_default", "rb+");
    if (!f) return;
    uchar port_num_uchar = port_num.toUInt() & 0xFF;
    fseek(f, id, SEEK_SET);
    fwrite(&port_num_uchar, 1, 1, f);
    fclose(f);
}

void UserPanel::on_ENUM_BUTTON_clicked()
{
    if (curr_cam) delete curr_cam;
    curr_cam = new Cam;
//    if (ui->TITLE->prog_settings->cameralink) curr_cam->cameralink = true;
    curr_cam->cameralink = pref->cameralink;
    enable_controls(device_type = curr_cam->search_for_devices());
//    ui->TITLE->prog_settings->enable_ip_editing(device_type == 1);
    pref->enable_ip_editing(device_type == 1);
    if (device_type) {
        int ip, gateway, nic_address;
        curr_cam->ip_address(true, &ip, &gateway, &nic_address);
//        ui->TITLE->prog_settings->config_ip(true, ip, gateway);
        pref->config_ip(true, ip, gateway, nic_address);
    }
}

void UserPanel::on_START_BUTTON_clicked()
{
    if (device_on) return;
    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
    duty = time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
    fps = frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();

    if (curr_cam->start()) {
        QMessageBox::warning(this, "PROMPT", tr("start failed"));
        return;
    }
    device_on = true;
    enable_controls(true);

    curr_cam->set_frame_callback(&img_q);

    on_SET_PARAMS_BUTTON_clicked();
    ui->GAIN_SLIDER->setMinimum(0);
    ui->GAIN_SLIDER->setMaximum(curr_cam->device_type == 1 ? 23 : 480);
    ui->GAIN_SLIDER->setSingleStep(1);
    ui->GAIN_SLIDER->setPageStep(4);
    ui->GAIN_SLIDER->setValue(gain_analog_edit);
    connect(ui->GAIN_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_gain(int)));

    on_CONTINUOUS_RADIO_clicked();

    curr_cam->time_exposure(true, &time_exposure_edit);
    curr_cam->gain_analog(true, &gain_analog_edit);
    curr_cam->frame_rate(true, &frame_rate_edit);
    ui->GAIN_SLIDER->setValue((int)gain_analog_edit);
    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)gain_analog_edit));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));

    // TODO complete BayerGB process & display
    curr_cam->pixel_type(true, &pixel_format);
    switch (pixel_format) {
    case PixelType_Gvsp_Mono8:       is_color = false; update_pixel_depth( 8); break;
    case PixelType_Gvsp_Mono10:      is_color = false; update_pixel_depth(10); break;
    case PixelType_Gvsp_Mono12:      is_color = false; update_pixel_depth(12); break;
    case PixelType_Gvsp_BayerGB8:    is_color =  true; update_pixel_depth( 8); break;
    case PixelType_Gvsp_RGB8_Packed: is_color =  true; update_pixel_depth( 8); break;
    default:                         is_color = false; update_pixel_depth( 8); break;
    }

    // adjust display size according to frame size
    curr_cam->frame_size(true, &w, &h);
    qInfo("frame w: %d, h: %d", w, h);
//    QRect region = ui->SOURCE_DISPLAY->geometry();
//    region.setHeight(ui->SOURCE_DISPLAY->width() * h / w);
//    ui->SOURCE_DISPLAY->setGeometry(region);
//    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
//    ui->SOURCE_DISPLAY->update_roi(QPoint());
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);

//    ui->TITLE->prog_settings->set_pixel_format(0);
    pref->set_pixel_format(0);
}

void UserPanel::on_SHUTDOWN_BUTTON_clicked()
{
    ui->STOP_GRABBING_BUTTON->click();
    shut_down();
    ui->IMG_ENHANCE_CHECK->setChecked(false);
    ui->FRAME_AVG_CHECK->setChecked(false);
    ui->IMG_3D_CHECK->setChecked(false);
    on_ENUM_BUTTON_clicked();
    clean();
}

void UserPanel::on_CONTINUOUS_RADIO_clicked()
{
    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
    duty = time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
    fps = frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();

    ui->CONTINUOUS_RADIO->setChecked(true);
    ui->TRIGGER_RADIO->setChecked(false);
    ui->SOFTWARE_CHECK->setEnabled(false);
    trigger_mode_on = false;
    curr_cam->trigger_mode(false, &trigger_mode_on);

    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(false);
}

void UserPanel::on_TRIGGER_RADIO_clicked()
{
    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
    duty = time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
    fps = frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();

    ui->CONTINUOUS_RADIO->setChecked(false);
    ui->TRIGGER_RADIO->setChecked(true);
    ui->SOFTWARE_CHECK->setEnabled(true);
    trigger_mode_on = true;
    curr_cam->trigger_mode(false, &trigger_mode_on);

    if (start_grabbing && trigger_by_software) ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(true);
}

// 0: continuous, 1: software, 2: external
void UserPanel::on_SOFTWARE_CHECK_stateChanged(int arg1)
{
    trigger_by_software = arg1;
    trigger_source = arg1 ? 1 : 2;
    curr_cam->trigger_source(false, &trigger_by_software);
    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(start_grabbing && arg1);
}

void UserPanel::on_SOFTWARE_TRIGGER_BUTTON_clicked()
{
    curr_cam->trigger_once();
}

void UserPanel::on_START_GRABBING_BUTTON_clicked()
{
    if (!device_on || start_grabbing || curr_cam == NULL) return;

    if (grab_image || h_grab_thread) {
        grab_image = false;
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
    }

    grab_image = true;
    h_grab_thread = new GrabThread((void*)this);
    if (h_grab_thread == NULL) {
        grab_image = false;
        QMessageBox::warning(this, "PROMPT", tr("Create thread fail"));
        return;
    }
    h_grab_thread->start();

    if (curr_cam->start_grabbing()) {
        grab_image = false;
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
        return;
    }
    start_grabbing = true;
    ui->SOURCE_DISPLAY->is_grabbing = true;
    enable_controls(true);

#ifdef ICMOS
    ui->HIDE_BTN->click();
#endif
}

void UserPanel::on_STOP_GRABBING_BUTTON_clicked()
{
//    if (!device_on || !start_grabbing || curr_cam == NULL) return;

    if (record_original) on_SAVE_AVI_BUTTON_clicked();

    if (!img_q.empty()) std::queue<cv::Mat>().swap(img_q);

    grab_image = false;
    if (h_grab_thread) {
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
    }

    start_grabbing = false;
    ui->SOURCE_DISPLAY->is_grabbing = false;
    QTimer::singleShot(10, this, SLOT(clean()));
    enable_controls(device_type);
    if (device_type == -1) ui->ENUM_BUTTON->click();
//    if (device_type == -2) ui->ENUM_BUTTON->click(), video_input->stop(), video_surface->stop();
    if (device_type == -2) ui->ENUM_BUTTON->click();

    curr_cam->stop_grabbing();
}

void UserPanel::on_SAVE_BMP_BUTTON_clicked()
{
    if (!QDir(save_location + "/ori_bmp").exists()) QDir().mkdir(save_location + "/ori_bmp");
    if (device_type == -1) save_original = 1;
    else{
        save_original = !save_original;
        ui->SAVE_BMP_BUTTON->setText(save_original ? tr("Stop") : tr("ORI"));
    }
}

void UserPanel::on_SAVE_FINAL_BUTTON_clicked()
{
    static QString res_avi;
//    cv::imwrite(QString(save_location + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".jpg").toLatin1().data(), modified_result);
    if (record_modified) {
//        curr_cam->stop_recording(0);
        image_mutex.lock();
        vid_out[1].release();
        image_mutex.unlock();
        QString dest = save_location + "/" + res_avi.section('/', -1, -1);
        std::thread t(UserPanel::move_to_dest, QString(res_avi), QString(dest));
        t.detach();
//        QFile::rename(res_avi, dest);
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        image_mutex.lock();
        res_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_res.avi");
        vid_out[1].open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(image_3d ? w + 104 : w, h), is_color || image_3d);
        image_mutex.unlock();
    }
    record_modified = !record_modified;
    ui->SAVE_FINAL_BUTTON->setText(record_modified ? tr("Stop") : tr("RES"));
}

void UserPanel::on_SET_PARAMS_BUTTON_clicked()
{
    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
    duty = time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
    fps = frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();

    // CCD FREQUENCY
    ptr_tcu->communicate_display(convert_to_send_tcu(0x06, 1.25e8 / fps), 7, 1, false);

    // DUTY RATIO -> EXPO. TIME
    ptr_tcu->communicate_display(convert_to_send_tcu(0x07, duty * 1.25e2), 7, 1, false);

    time_exposure_edit = duty;
    frame_rate_edit = fps;
    if (device_on) {
        curr_cam->time_exposure(false, &time_exposure_edit);
        curr_cam->frame_rate(false, &frame_rate_edit);
//        curr_cam->gain_analog(false, &gain_analog_edit);
        change_gain(gain_analog_edit);
    }
    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)gain_analog_edit));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));
}

void UserPanel::on_GET_PARAMS_BUTTON_clicked()
{
    curr_cam->trigger_mode(true, &trigger_mode_on);
    trigger_mode_on ? on_TRIGGER_RADIO_clicked() : on_CONTINUOUS_RADIO_clicked();
    curr_cam->trigger_source(true, &trigger_by_software);
    trigger_source = trigger_by_software ? 1 : 2;

    curr_cam->time_exposure(true, &time_exposure_edit);
    curr_cam->gain_analog(true, &gain_analog_edit);
    curr_cam->frame_rate(true, &frame_rate_edit);

    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)gain_analog_edit));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));
}

void UserPanel::on_FILE_PATH_BROWSE_clicked()
{
    QString temp = QFileDialog::getExistingDirectory(this, tr("Select folder"), save_location);
    if (!temp.isEmpty()) save_location = temp;
    if (save_original && !QDir(temp + "/ori_bmp").exists()) QDir().mkdir(temp + "/ori_bmp");
    if (save_modified && !QDir(temp + "/res_bmp").exists()) QDir().mkdir(temp + "/res_bmp");

    ui->FILE_PATH_EDIT->setText(save_location);
}

void UserPanel::clean()
{
    ui->SOURCE_DISPLAY->clear();
    ui->DATA_EXCHANGE->clear();
    if (grab_image) return;
    std::queue<cv::Mat>().swap(img_q);
    img_mem.release();
    modified_result.release();
    img_display.release();
    prev_img.release();
    prev_3d.release();
    for (cv::Mat &m: seq) m.release();
    seq_sum.release();
    frame_a_sum.release();
    frame_b_sum.release();
}

void UserPanel::setup_hz(int hz_unit)
{
    this->hz_unit = hz_unit;
    switch (hz_unit) {
    // kHz
    case 0: ui->FREQ_UNIT->setText("kHz"); ui->FREQ_EDIT->setText(QString::number((int)rep_freq)); break;
    // Hz
    case 1: ui->FREQ_UNIT->setText("Hz"); ui->FREQ_EDIT->setText(QString::number((int)(rep_freq * 1000))); break;
    default: break;
    }
}

void UserPanel::setup_stepping(int base_unit)
{
    this->base_unit = base_unit;
    switch (base_unit) {
    // ns
    case 0: ui->STEPPING_UNIT->setText("ns"); ui->STEPPING_EDIT->setText(QString::number((int)stepping)); break;
    // μs
//    case 1: ui->STEPPING_UNIT->setText(QString::fromLocal8Bit("μs")); ui->STEPPING_EDIT->setText(QString::number(stepping / 1000, 'f', 2)); break;
    case 1: ui->STEPPING_UNIT->setText("μs"); ui->STEPPING_EDIT->setText(QString::number(stepping / 1000, 'f', 2)); break;
    // m
    case 2: ui->STEPPING_UNIT->setText("m"); ui->STEPPING_EDIT->setText(QString::number(stepping * dist_ns, 'f', 2)); break;
    default: break;
    }
}

void UserPanel::setup_max_dist(float max_dist)
{
    this->max_dist = max_dist;
    ui->DELAY_SLIDER->setMaximum(max_dist);
}

void UserPanel::update_delay_offset(float delay_dist_offset)
{
    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist - pref->delay_dist_offset));
}

// e.g. laser_on: 0b1110 -> laser[1], laser[2] and laser[3] are on, laser[0] is off
void UserPanel::setup_laser(int laser_on)
{
    int change = laser_on ^ this->laser_on;
//    qDebug() << QString::number(laser_on, 2);
//    qDebug() << QString::number(change, 2);
    for(int i = 0; i < 4; i++) if ((change >> i) & 1) ptr_tcu->communicate_display(convert_to_send_tcu(0x1A + i, laser_on & (1 << i) ? 8 : 4), 7, 1, false);
    this->laser_on = laser_on;
}

void UserPanel::set_baudrate(int idx, int baudrate)
{
//    if (serial_port[idx]->isOpen()) serial_port[idx]->setBaudRate(baudrate);
    switch (idx) {
    case 0: ptr_tcu->set_baudrate(baudrate); break;
    case 1: break;
    case 2: ptr_lens->set_baudrate(baudrate); break;
    case 3: ptr_laser->set_baudrate(baudrate); break;
    case 4: ptr_ptz->set_baudrate(baudrate); break;
    default: break;
    }
}

void UserPanel::set_tcu_as_shared_port(bool share)
{
    ptr_lens->share_port_from(share ? ptr_tcu : NULL);
}

void UserPanel::com_write_data(int com_idx, QByteArray data)
{
//    communicate_display(com_idx, data, data.length(), 0, true);
    ControlPort *temp_port = NULL;
    switch (com_idx) {
    case 0: temp_port = ptr_tcu; break;
    case 1: temp_port = ptr_lens; break;
//    case 2: temp_port = ptr_laser; break;
    case 3: temp_port = ptr_ptz; break;
    default: break;
    }
    if (temp_port) temp_port->communicate_display(data, data.length(), 0, true);
}

void UserPanel::display_baudrate(int idx)
{
//    if (serial_port[idx]->isOpen()) ui->TITLE->prog_settings->display_baudrate(idx, serial_port[idx]->baudRate());
//    if (serial_port[idx]->isOpen()) pref->display_baudrate(idx, serial_port[idx]->baudRate());
    switch (idx) {
    case 0: pref->display_baudrate(idx,   ptr_tcu->get_baudrate()); break;
    case 1: pref->display_baudrate(idx, 0); break;
    case 2: pref->display_baudrate(idx,  ptr_lens->get_baudrate()); break;
    case 3: pref->display_baudrate(idx, ptr_laser->get_baudrate()); break;
    case 4: pref->display_baudrate(idx,   ptr_ptz->get_baudrate()); break;
    default: break;
    }
}

void UserPanel::set_dev_ip(int ip, int gateway)
{
    qDebug() << "modify ip: " << hex << curr_cam->ip_address(false, &ip, &gateway);
}

void UserPanel::change_pixel_format(int pixel_format)
{
    bool was_playing = start_grabbing;
    if (was_playing) ui->STOP_GRABBING_BUTTON->click();

    this->pixel_format = pixel_format;
    int ret = curr_cam->pixel_type(false, &pixel_format);
    switch (pixel_format) {
    case PixelType_Gvsp_Mono8:       is_color = false; update_pixel_depth( 8); break;
    case PixelType_Gvsp_Mono10:      is_color = false; update_pixel_depth(10); break;
    case PixelType_Gvsp_Mono12:      is_color = false; update_pixel_depth(12); break;
    case PixelType_Gvsp_BayerGB8:    is_color =  true; update_pixel_depth( 8); break;
    case PixelType_Gvsp_RGB8_Packed: is_color =  true; update_pixel_depth( 8); break;
    default:                         is_color = false; update_pixel_depth( 8); break;
    }

    if (was_playing) ui->START_GRABBING_BUTTON->click();
}

void UserPanel::update_lower_3d_thresh()
{
    ui->RANGE_THRESH_EDIT->setText(QString::number((int)(pref->lower_3d_thresh * (1 << pixel_depth))));
}

void UserPanel::joystick_button_pressed(int btn)
{
    switch (btn) {
    case JoystickThread::BUTTON::A:  joybtn_A  = true; break;
    case JoystickThread::BUTTON::B:  joybtn_B  = true; break;
    case JoystickThread::BUTTON::X:  joybtn_X  = true; break;
    case JoystickThread::BUTTON::Y:  joybtn_Y  = true; break;
    case JoystickThread::BUTTON::L1: joybtn_L1 = true; break;
    case JoystickThread::BUTTON::L2:
        joybtn_L2 = true;
        if      (joybtn_X) on_ZOOM_IN_BTN_pressed(),       lens_adjust_ongoing = 0b001;
        else if (joybtn_Y) on_FOCUS_NEAR_BTN_pressed(),    lens_adjust_ongoing = 0b010;
        else if (joybtn_A) on_IRIS_OPEN_BTN_pressed(), lens_adjust_ongoing = 0b100;
        break;
    case JoystickThread::BUTTON::R1: joybtn_R1 = true; break;
    case JoystickThread::BUTTON::R2:
        joybtn_R2 = true;
        if      (joybtn_X) on_ZOOM_OUT_BTN_pressed(),       lens_adjust_ongoing = 0b001;
        else if (joybtn_Y) on_FOCUS_FAR_BTN_pressed(),      lens_adjust_ongoing = 0b010;
        else if (joybtn_A) on_IRIS_CLOSE_BTN_pressed(), lens_adjust_ongoing = 0b100;
        break;
    case JoystickThread::BUTTON::HOME:
    case JoystickThread::BUTTON::SELECT:
    case JoystickThread::BUTTON::MINUS:
    case JoystickThread::BUTTON::PLUS:
    default: break;
    }
}

void UserPanel::joystick_button_released(int btn)
{
    switch (btn) {
    case JoystickThread::BUTTON::A:
        joybtn_A = false;
        if (ptz_adjust_ongoing) ptz_button_released(-1), ptz_adjust_ongoing = false;
        break;
    case JoystickThread::BUTTON::B:  joybtn_B  = false; break;
    case JoystickThread::BUTTON::X:  joybtn_X  = false; break;
    case JoystickThread::BUTTON::Y:  joybtn_Y  = false; break;
    case JoystickThread::BUTTON::L1: joybtn_L1 = false; break;
    case JoystickThread::BUTTON::L2:
        joybtn_L2 = false;
        if (lens_adjust_ongoing & 0b001) on_ZOOM_IN_BTN_released(),       lens_adjust_ongoing = 0;
        if (lens_adjust_ongoing & 0b010) on_FOCUS_NEAR_BTN_released(),    lens_adjust_ongoing = 0;
        if (lens_adjust_ongoing & 0b100) on_IRIS_OPEN_BTN_released(), lens_adjust_ongoing = 0;
        break;
    case JoystickThread::BUTTON::R1: joybtn_R1 = false; break;
    case JoystickThread::BUTTON::R2:
        joybtn_R2 = false;
        if (lens_adjust_ongoing & 0b001) on_ZOOM_OUT_BTN_released(),       lens_adjust_ongoing = 0;
        if (lens_adjust_ongoing & 0b010) on_FOCUS_FAR_BTN_released(),      lens_adjust_ongoing = 0;
        if (lens_adjust_ongoing & 0b100) on_IRIS_CLOSE_BTN_released(), lens_adjust_ongoing = 0;
        break;
    case JoystickThread::BUTTON::HOME: ui->SHUTDOWN_BUTTON->click(); ui->ENUM_BUTTON->click(); break;
    case JoystickThread::BUTTON::SELECT: ui->START_BUTTON->click(); break;
    case JoystickThread::BUTTON::MINUS:
        if (joybtn_L1) stepping /= 10, setup_stepping(base_unit);
        if (joybtn_R1) change_focus_speed(ui->FOCUS_SPEED_EDIT->text().toInt() - 8);
        if (joybtn_L2) ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed - 8)), on_PTZ_SPEED_EDIT_editingFinished();
        break;
    case JoystickThread::BUTTON::PLUS:
        if (joybtn_L1) stepping *= 10, setup_stepping(base_unit);
        if (joybtn_R1) change_focus_speed(ui->FOCUS_SPEED_EDIT->text().toInt() + 8);
        if (joybtn_L2) ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed + 8)), on_PTZ_SPEED_EDIT_editingFinished();
        break;
    default: break;
    }
}

void UserPanel::joystick_direction_changed(int direction)
{
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    // DIST
    if (joybtn_X) {
        switch (direction) {
        case JoystickThread::DIRECTION::UP:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_W, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        case JoystickThread::DIRECTION::DOWN:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_S, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        case JoystickThread::DIRECTION::LEFT:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        case JoystickThread::DIRECTION::RIGHT:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_D, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        default: break;
        }
    }
    // DOV
    else if (joybtn_Y) {
        switch (direction) {
        case JoystickThread::DIRECTION::UP:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_I, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        case JoystickThread::DIRECTION::DOWN:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_K, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        case JoystickThread::DIRECTION::LEFT:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_J, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        case JoystickThread::DIRECTION::RIGHT:
            e = QKeyEvent(QEvent::KeyPress, Qt::Key_L, Qt::NoModifier);
            keyPressEvent(&e);
            break;
        default: break;
        }
    }
    // PTZ
    else if (joybtn_A) {
        switch (direction) {
        case JoystickThread::DIRECTION::UPLEFT:    ptz_button_pressed(0);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::UP:        ptz_button_pressed(1);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::UPRIGHT:   ptz_button_pressed(2);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::LEFT:      ptz_button_pressed(3);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::RIGHT:     ptz_button_pressed(5);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::DOWNLEFT:  ptz_button_pressed(6);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::DOWN:      ptz_button_pressed(7);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::DOWNRIGHT: ptz_button_pressed(8);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::STOP:      ptz_button_released(-1); ptz_adjust_ongoing = false; break;
        default:                                                                                        break;
        }
    }
}

void UserPanel::append_data(QString str)
{
    static QScrollBar *scroll_bar = ui->DATA_EXCHANGE->verticalScrollBar();
    ui->DATA_EXCHANGE->append(str);
    scroll_bar->setValue(scroll_bar->maximum());
}

void UserPanel::update_port_status(int connected_status)
{
    qDebug() << connected_status;
}

// FIXME update config i/o
void UserPanel::export_config()
{
//    QString config_name = QFileDialog::getSaveFileName(this, tr("Save Configuration"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/config.ssy", tr("*.ssy"));
    QString config_name = QFileDialog::getSaveFileName(this, tr("Save Configuration"), save_location + "/config.ssy", tr("YJS config(*.ssy);;All Files()"));
    if (config_name.isEmpty()) return;
    if (!config_name.endsWith(".ssy")) config_name.append(".ssy");
    QFile config(config_name);
    config.open(QIODevice::WriteOnly);
    if (config.isOpen()) {
        QDataStream out(&config);
        convert_write(out, WIN_PREF);
        convert_write(out, TCU_PARAMS);
        convert_write(out, SCAN);
        convert_write(out, IMG);
        convert_write(out, TCU_PREF);
        config.close();
    }
    else {
        QMessageBox::warning(this, "PROMPT", tr("cannot create config file"));
    }
}

void UserPanel::prompt_for_config_file()
{
//    QString config_name = QFileDialog::getOpenFileName(this, tr("Load Configuration"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("*.ssy"));
    QString config_filename = QFileDialog::getOpenFileName(this, tr("Load Configuration"), save_location, tr("YJS config(*.ssy);;All Files()"));
    load_config(config_filename);
}

void UserPanel::load_config(QString config_name)
{
    if (config_name.isEmpty()) return;
    QFile config(config_name);
    config.open(QIODevice::ReadOnly);
    bool read_success = true;
    if (config.isOpen()) {
        QDataStream out(&config);
        read_success &= convert_read(out, WIN_PREF);
        read_success &= convert_read(out, TCU_PARAMS);
        read_success &= convert_read(out, SCAN);
        read_success &= convert_read(out, IMG);
        read_success &= convert_read(out, TCU_PREF);
        config.close();
    }
    else {
        QMessageBox::warning(this, "PROMPT", tr("cannot open config file"));
    }
    if (read_success) {
//        data_exchange(false);
        update_delay();
        update_gate_width();
        ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
        ui->MCP_SLIDER->setValue(mcp);
        on_SET_PARAMS_BUTTON_clicked();
//        ui->TITLE->prog_settings->data_exchange(false);
        pref->data_exchange(false);
    }
    else {
        data_exchange(true);
//        ui->TITLE->prog_settings->data_exchange(true);
        pref->data_exchange(true);
        QMessageBox::warning(this, "PROMPT", tr("cannot read config file"));
    }
}

void UserPanel::prompt_for_serial_file()
{
    QString serial_filename = QFileDialog::getOpenFileName(this, tr("Load SN Config"), save_location, tr("(*.csv);;All Files()"));
    config_gatewidth(serial_filename);
}

void UserPanel::prompt_for_input_file()
{
    QDialog input_file_dialog(this, Qt::FramelessWindowHint);
    QFormLayout form(&input_file_dialog);
    form.setRowWrapPolicy(QFormLayout::WrapAllRows);

    QLineEdit *file_path = new QLineEdit("rtsp://192.168.2.100/mainstream", &input_file_dialog);
    form.addRow(new QLabel("video URL :  ", &input_file_dialog), file_path);

    QDialogButtonBox button_box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &input_file_dialog);
    form.addRow(&button_box);
    QObject::connect(&button_box, SIGNAL(accepted()), &input_file_dialog, SLOT(accept()));
    QObject::connect(&button_box, SIGNAL(rejected()), &input_file_dialog, SLOT(reject()));

    input_file_dialog.setModal(true);
    button_box.button(QDialogButtonBox::Ok)->setFocus();
    button_box.button(QDialogButtonBox::Ok)->setDefault(true);

    // Process when OK button is clicked
    if (input_file_dialog.exec() == QDialog::Accepted) load_video_file(file_path->text(), true);
}

// convert data to be sent to TCU-COM to hex buffer
QByteArray UserPanel::convert_to_send_tcu(uchar num, unsigned int send) {
    QByteArray out(7, 0x00);
    out[0] = 0x88;
    out[1] = num;
    out[6] = 0x99;

    out[5] = send & 0xFF; send >>= 8;
    out[4] = send & 0xFF; send >>= 8;
    out[3] = send & 0xFF; send >>= 8;
    out[2] = send & 0xFF;
    return out;
}

inline QByteArray UserPanel::generate_ba(uchar *data, int len)
{
    QByteArray ba((char*)data, len);
    delete[] data;
    return ba;
}

// send and receive data from serial port, and display
QByteArray UserPanel::communicate_display(int id, QByteArray write, int write_size, int read_size, bool fb) {
    port_mutex.lock();
//    image_mutex.lock();
    QByteArray read;
    QString str_s("sent    "), str_r("received");

    for (char i = 0; i < write_size; i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
    emit append_text(str_s);

    if (!use_tcp[id]) {
        if (!serial_port[id]->isOpen()) {
            port_mutex.unlock();
//            image_mutex.unlock();
            return QByteArray();
        }
        serial_port[id]->clear();
        serial_port[id]->write(write, write_size);
        while (serial_port[id]->waitForBytesWritten(10)) ;

        if (fb) while (serial_port[id]->waitForReadyRead(100)) ;

        read = read_size ? serial_port[id]->read(read_size) : serial_port[id]->readAll();
        for (char i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
        emit append_text(str_r);
    }
    else {
        if (!tcp_port[id]->isOpen()) {
            port_mutex.unlock();
//            image_mutex.unlock();
            return QByteArray();
        }
        tcp_port[id]->write(write, write_size);
        while (tcp_port[id]->waitForBytesWritten(10)) ;

        if (fb) while (tcp_port[id]->waitForReadyRead(100)) ;

        read = read_size ? tcp_port[id]->read(read_size) : tcp_port[id]->readAll();
        for (char i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
        emit append_text(str_r);
    }

    port_mutex.unlock();
//    image_mutex.unlock();
    QThread().msleep(10);
    return read;
}

void UserPanel::update_delay()
{
//    qDebug() << sender();
//    static QElapsedTimer t;
//    if (t.elapsed() < (fps > 9 ? 900 / fps : 100)) return;
//    t.start();

    // REPEATED FREQUENCY
    if (delay_dist < 0) delay_dist = 0;
    if (delay_dist > max_dist) delay_dist = max_dist;
//    qDebug("estimated distance: %f\n", delay_dist);

    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist - pref->delay_dist_offset));
    if (!qobject_cast<QSlider*>(sender())) ui->DELAY_SLIDER->setValue(delay_dist);

    int delay = std::round(delay_dist / dist_ns);
    delay_a_u = delay / 1000;
    delay_a_n = delay % 1000;
    delay_b_u = (delay + delay_n_n) / 1000;
    delay_b_n = (delay + delay_n_n) % 1000;

    if (scan) {
        rep_freq = scan_config->rep_freq;
    }
    else {
        // change repeated frequency according to delay: rep frequency (kHz) <= 1s / delay (μs)
//        if (ui->TITLE->prog_settings->auto_rep_freq) {
        if (pref->auto_rep_freq) {
            rep_freq = delay_dist ? 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000) : 30;
            if (rep_freq > 30) rep_freq = 30;
//            if (rep_freq < 10) rep_freq = 10;
        }

        ptr_tcu->communicate_display(convert_to_send_tcu(0x00, 1.25e5 / rep_freq), 7, 1, false);
    }

/*
    // DAC1
    send = dac1;
    convert_to_send_tcu(0x09, send);
    communicate_display(com_tcu, 1, 7);

    // DAC2
    send = mcp;
    convert_to_send_tcu(0x0A, send);
    communicate_display(com[0], 1, 7);
*/
    // DELAY A
    ptr_tcu->communicate_display(convert_to_send_tcu(0x02, (delay_a_u * 1000 + delay_a_n) + offset_delay), 7, 1, false);

    // DELAY B
    ptr_tcu->communicate_display(convert_to_send_tcu(0x04, (delay_b_u * 1000 + delay_b_n) + offset_delay), 7, 1, false);

    setup_hz(hz_unit);
    ui->DELAY_A_EDIT_U->setText(QString::asprintf("%d", delay_a_u));
    ui->DELAY_A_EDIT_N->setText(QString::asprintf("%d", delay_a_n));
    ui->DELAY_B_EDIT_U->setText(QString::asprintf("%d", delay_b_u));
    ui->DELAY_B_EDIT_N->setText(QString::asprintf("%d", delay_b_n));
}

void UserPanel::update_gate_width() {
#ifndef LVTONG
    static QElapsedTimer t;
    if (t.elapsed() < (fps > 9 ? 900 / fps : 100)) return;
    t.restart();
#endif

    if (depth_of_view < 0) depth_of_view = 0;
    if (depth_of_view > 1500) depth_of_view = 1500;

    int gw = std::round(depth_of_view / dist_ns), gw_corrected = gw + offset_gatewidth;
    if (gw < 100) gw_corrected = gw_lut[gw];
    if (gw_corrected == -1) {
        ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf("%d", gate_width_a_u));
        ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%d", gate_width_a_n));
        QMessageBox::warning(this, "PROMPT", tr("gatewidth not supported"));
        return;
    }
    // GATE WIDTH A
    ptr_tcu->communicate_display(convert_to_send_tcu(0x03, gw_corrected), 7, 1, false);

    // GATE WIDTH B
//    send = gw - 18;
    ptr_tcu->communicate_display(convert_to_send_tcu(0x05, gw_corrected), 7, 1, false);

    ui->GATE_WIDTH->setText(QString::asprintf("%.2f m", depth_of_view));
//    gate_width_a_n = gate_width_b_n = laser_width_n = gw % 1000;
//    gate_width_a_u = gate_width_b_u = laser_width_u = gw / 1000;
    gate_width_a_n = gw % 1000;
    gate_width_a_u = gw / 1000;

    ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf("%d", gate_width_a_u));
    ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%d", gate_width_a_n));
//        ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf("%d", gate_width_b_u));
//        ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%d", gate_width_b_n));
}

void UserPanel::filter_scan()
{
    static cv::Mat filter = img_mem.clone();
    filter = img_mem / 64;
//    qDebug("ratio %f\n", cv::countNonZero(filter) / (float)filter.total());
    if (cv::countNonZero(filter) / (float)filter.total() > 0.3) /*scan = !scan, */emit update_scan(true);
}

void UserPanel::update_current()
{
//    if (!serial_port[3]) return;
//    QString send = "DIOD1:CURR " + ui->CURRENT_EDIT->text() + "\r";
    QString send = "PCUR " + ui->CURRENT_EDIT->text() + "\r";
//    serial_port[3]->write(send.toLatin1().data(), send.length());
//    serial_port[3]->waitForBytesWritten(100);
//    serial_port[3]->readAll();
    ptr_laser->communicate_display(send.toLatin1(), send.length(), 0, true, false);
    qDebug() << send;
}

void UserPanel::convert_write(QDataStream &out, const int TYPE)
{
    data_exchange(true);
    out << uchar(0xAA) << uchar(0xAA);
//    static ProgSettings *ps = ui->TITLE->prog_settings;
    switch (TYPE) {
    case WIN_PREF:
    {
        out << "WIN_PREF" << uchar('.');
//        out << com_edit[0]->text().toInt() << com_edit[1]->text().toInt() << com_edit[2]->text().toInt() << com_edit[3]->text().toInt() << save_location.toUtf8().constData();
        out << save_location.toUtf8().constData();
    }
    case TCU_PARAMS:
    {
        out << "TCU" << uchar('.');
        out << fps << duty << rep_freq << laser_width << mcp << delay_dist << delay_n_n << depth_of_view << stepping;
    }
    case SCAN:
    {
        out << "SCAN" << uchar('.');
//        out << ps->start_pos << ps->end_pos << ps->frame_count << ps->step_size << ps->rep_freq << (int)(ps->save_scan_ori) << (int)(ps->save_scan_res);
        out << scan_config->starting_delay << scan_config->ending_delay << scan_config->starting_gw << scan_config->ending_gw
            << scan_config->frame_count << scan_config->step_size_delay << scan_config->step_size_gw << scan_config->rep_freq
            << (int)(scan_config->capture_scan_ori) << (int)(scan_config->capture_scan_res)
            << (int)(scan_config->record_scan_ori) << (int)(scan_config->record_scan_res);
    }
    case IMG:
    {
        out << "IMG" << uchar('.');
//        out << ps->kernel << ps->gamma << ps->log << ps->low_in << ps->low_out << ps->high_in << ps->high_out << ps->dehaze_pct << ps->sky_tolerance << ps->fast_gf;
        out << pref->accu_base << pref->gamma << pref->low_in << pref->low_out << pref->high_in << pref->high_out << pref->dehaze_pct << pref->sky_tolerance << pref->fast_gf;
    }
    case TCU_PREF:
    {
//        out << "TCU_PREF" << uchar('.');
//        out << (int)(ps->auto_rep_freq) << ps->hz_unit << ps->base_unit << ps->max_dist;
//        out << (int)(pref->auto_rep_freq) << pref->hz_unit << pref->base_unit << pref->max_dist;
    }
    default: break;
    }
    out << uchar('.') << uchar(0xFF) << uchar(0xFF);
}

bool UserPanel::convert_read(QDataStream &out, const int TYPE)
{
    uchar temp_uchar = 0x00;
    bool result = false;
    char *temp_str = (char*)malloc(100 * sizeof(char));
    memset(temp_str, 0, 100);
    out >> temp_uchar; if (temp_uchar != 0xAA) return false;
    out >> temp_uchar; if (temp_uchar != 0xAA) return false;
    switch (TYPE) {
    case WIN_PREF:
    {
        out >> temp_str; if (std::strcmp(temp_str, "WIN_PREF")) return false;
        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
//        int temp_int = 0;
//        for (int i = 0; i < 4; i++) {
//            out >> temp_int;
//            com_edit[i]->setText(QString::number(temp_int));
//        }
        out >> temp_str; save_location = QString::fromUtf8(temp_str);
    }
    case TCU_PARAMS:
    {
        out >> temp_str; if (std::strcmp(temp_str, "TCU")) return false;
        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
        out >> fps >> duty >> rep_freq >> laser_width >> mcp >> delay_dist >> delay_n_n >> depth_of_view >> stepping;
    }
    case SCAN:
    {
        out >> temp_str; if (std::strcmp(temp_str, "SCAN")) return false;
        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
        int temp_bool;
//        out >> ps->start_pos >> ps->end_pos >> ps->frame_count >> ps->step_size >> ps->rep_freq >> temp_bool;
//        ps->save_scan_ori = temp_bool;
//        out >> temp_bool;
//        ps->save_scan_res = temp_bool;
        out >> scan_config->starting_delay >> scan_config->ending_delay >> scan_config->starting_gw >> scan_config->ending_gw
            >> scan_config->frame_count >> scan_config->step_size_delay >> scan_config->step_size_gw >> scan_config->rep_freq;
        out >> temp_bool;
        scan_config->capture_scan_ori = temp_bool;
        out >> temp_bool;
        scan_config->capture_scan_res = temp_bool;
        out >> temp_bool;
        scan_config->record_scan_ori = temp_bool;
        out >> temp_bool;
        scan_config->record_scan_res = temp_bool;
        scan_config->data_exchange(false);
    }
    case IMG:
    {
        out >> temp_str; if (std::strcmp(temp_str, "IMG")) return false;
        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
//        out >> ps->kernel >> ps->gamma >> ps->log >> ps->low_in >> ps->low_out >> ps->high_in >> ps->high_out >> ps->dehaze_pct >> ps->sky_tolerance >> ps->fast_gf;
        out >> pref->accu_base >> pref->gamma >> pref->low_in >> pref->low_out >> pref->high_in >> pref->high_out >> pref->dehaze_pct >> pref->sky_tolerance >> pref->fast_gf;
    }
    case TCU_PREF:
    {
//        out >> temp_str; if (std::strcmp(temp_str, "TCU_PREF")) return false;
//        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
//        int temp_bool;
//        out >> temp_bool >> ps->hz_unit >> ps->base_unit >> ps->max_dist;
//        ps->auto_rep_freq = temp_bool;
    }
    default: break;
    }
    out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
    out >> temp_uchar; if (temp_uchar != 0xFF) return false;
    out >> temp_uchar; if (temp_uchar != 0xFF) return false;
    result = true;
    free(temp_str);
    return result;
}

void UserPanel::read_gatewidth_lookup_table(QFile *fp)
{
    if (!fp || !fp->isOpen()) return;
    QList<QByteArray> line_sep;
    while (!fp->atEnd()) {
        line_sep = fp->readLine().split(',');
        gw_lut[line_sep[0].toInt()] = line_sep[1].toInt();
    }
}

void UserPanel::connect_to_serial_server_tcp()
{
    QString ip = "192.168.1.233";
    bool connected = false;
    tcp_port[0]->connectToHost(ip, 10001);
    tcp_port[2]->connectToHost(ip, 10002);
    connected |= tcp_port[0]->waitForConnected(3000);
    connected |= tcp_port[2]->waitForConnected(3000);
    tcp_port[4] = tcp_port[2];
    for (int i = 0; i < 5; i++) use_tcp[i] = connected;
    if (connected) QMessageBox::information(this, "serial port server", "connected to server");
    else           QMessageBox::information(this, "serial port server", "cannot connect to server");
}

void UserPanel::disconnect_from_serial_server_tcp()
{
    for (int i = 0; i < 4; i++) {
        tcp_port[i]->disconnectFromHost();
    }
    QMessageBox::information(this, "serial port server", "disconnected from server");
    for (int i = 0; i < 5; i++) use_tcp[i] = false;
}

void UserPanel::on_DIST_BTN_clicked() {
    if (serial_port[1]->isOpen()) {
        QByteArray read = communicate_display(1, QByteArray(1, 0xA5), 1, 6, true);
//        qDebug("%s", read_dist.toLatin1().data());

        read.clear();
        read = communicate_display(1, generate_ba(new uchar[6]{0xEE, 0x16, 0x02, 0x03, 0x02, 0x05}, 6), 6, 10, true);
//        qDebug("%s", read_dist.toLatin1().data());
//        communicate_display(com[1], 7, 7, true);
//        QString ascii;
//        for (int i = 0; i < 7; i++) ascii += QString::asprintf(" %2c", in_buffer[i]);
//        emit append_text("received " + ascii);

        distance = read[7] + (read[6] << 8);
    }
    else {
        bool ok = false;
        distance = QInputDialog::getInt(this, "DISTANCE INPUT", "DETECTED DISTANCE: ", 100, 100, max_dist, 100, &ok, Qt::FramelessWindowHint);
        if (!ok) return;
    }

    if (distance < 100) distance = 100;
    ui->DISTANCE->setText(QString::asprintf("%d m", distance));
//    data_exchange(true);

    // change delay and gate width according to distance
    delay_dist = distance + pref->delay_dist_offset;
    update_delay();
//    update_gate_width();
//    change_mcp(150);

    rep_freq = 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000);
    if (rep_freq > 30) rep_freq = 30;
    if      (distance < 1000) depth_of_view =  500 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);
    else if (distance < 3000) depth_of_view = 1000 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);
    else if (distance < 6000) depth_of_view = 2000 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);
    else                      depth_of_view = 3500 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);

    ptr_tcu->communicate_display(convert_to_send_tcu(0x1E, distance), 7, 1, true);
    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist - pref->delay_dist_offset));

    setup_hz(hz_unit);
    ui->LASER_WIDTH_EDIT_U->setText(QString::asprintf("%d", laser_width_u));
    ui->LASER_WIDTH_EDIT_N->setText(QString::asprintf("%d", laser_width_n));
    ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf("%d", gate_width_a_u));
    ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%d", gate_width_a_n));
    ui->DELAY_A_EDIT_U->setText(QString::asprintf("%d", delay_a_u));
    ui->DELAY_A_EDIT_N->setText(QString::asprintf("%d", delay_a_n));
//        ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf("%d", gate_width_b_u));
//        ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%d", gate_width_b_n));
    ui->DELAY_B_EDIT_U->setText(QString::asprintf("%d", delay_b_u));
    ui->DELAY_B_EDIT_N->setText(QString::asprintf("%d", delay_b_n));
}

void UserPanel::on_IMG_3D_CHECK_stateChanged(int arg1)
{
//    QRect region = ui->SOURCE_DISPLAY->geometry();
//    region.setHeight(ui->SOURCE_DISPLAY->width() * h / (arg1 ? w + 104 : w));
//    ui->SOURCE_DISPLAY->setGeometry(region);
//    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
//    ui->SOURCE_DISPLAY->update_roi(QPoint());

//    image_mutex.lock();
//    w = arg1 ? w + 104 : w - 104;
//    image_mutex.unlock();
    image_3d = ui->IMG_3D_CHECK->isChecked();

    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
    if (!arg1) frame_a_3d = 0, prev_3d = cv::Mat(w, h, CV_8UC3);
}

void UserPanel::on_ZOOM_IN_BTN_pressed()
{
    ui->ZOOM_IN_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x40, 0x00, 0x00, 0x41}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x40, 0x00, 0x00, 0x41}, 7), 7, 1, false);
}

void UserPanel::on_ZOOM_OUT_BTN_pressed()
{
    ui->ZOOM_OUT_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x20, 0x00, 0x00, 0x21}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x20, 0x00, 0x00, 0x21}, 7), 7, 1, false);
}

void UserPanel::on_FOCUS_NEAR_BTN_pressed()
{
    ui->FOCUS_NEAR_BTN->setText("x");
    focus_near();
}

inline void UserPanel::focus_near()
{
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
}

inline void UserPanel::set_laser_preset_target(int *pos)
{
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x81, uchar((pos[0] >> 8) & 0xFF), uchar(pos[0] & 0xFF), uchar((((pos[0] >> 8) & 0xFF) + (pos[0] & 0xFF) + 0x82) & 0xFF)}, 7), 7, 1, false);
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4F, uchar((pos[1] >> 8) & 0xFF), uchar(pos[1] & 0xFF), uchar((((pos[1] >> 8) & 0xFF) + (pos[1] & 0xFF) + 0x51) & 0xFF)}, 7), 7, 1, false);
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4E, uchar((pos[2] >> 8) & 0xFF), uchar(pos[2] & 0xFF), uchar((((pos[2] >> 8) & 0xFF) + (pos[2] & 0xFF) + 0x50) & 0xFF)}, 7), 7, 1, false);
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x81, uchar((pos[3] >> 8) & 0xFF), uchar(pos[3] & 0xFF), uchar((((pos[3] >> 8) & 0xFF) + (pos[3] & 0xFF) + 0x83) & 0xFF)}, 7), 7, 1, false);

    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x81, uchar((pos[0] >> 8) & 0xFF), uchar(pos[0] & 0xFF), uchar((((pos[0] >> 8) & 0xFF) + (pos[0] & 0xFF) + 0x82) & 0xFF)}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4F, uchar((pos[1] >> 8) & 0xFF), uchar(pos[1] & 0xFF), uchar((((pos[1] >> 8) & 0xFF) + (pos[1] & 0xFF) + 0x51) & 0xFF)}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4E, uchar((pos[2] >> 8) & 0xFF), uchar(pos[2] & 0xFF), uchar((((pos[2] >> 8) & 0xFF) + (pos[2] & 0xFF) + 0x50) & 0xFF)}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x81, uchar((pos[3] >> 8) & 0xFF), uchar(pos[3] & 0xFF), uchar((((pos[3] >> 8) & 0xFF) + (pos[3] & 0xFF) + 0x83) & 0xFF)}, 7), 7, 1, false);

    delete[] pos;
}

void UserPanel::goto_laser_preset(char target)
{
    // check sum = sum(byte1 to byte5) & 0xFF
    if (curr_laser_idx < 0) {
        lens_stop();
        set_laser_preset_target(new int[4]{0});
        target = 1;
        goto quick_break;
    }
    if (target > curr_laser_idx) {
        switch(curr_laser_idx | target) {
        case 0x3:
            set_laser_preset_target(new int[4]{1008, 1008, 1008, 1008});
            goto quick_break;
        case 0x5:
            set_laser_preset_target(new int[4]{2016, 2016, 2016, 2016});
            goto quick_break;
        case 0x9:
            set_laser_preset_target(new int[4]{3024, 3024, 3024, 3024});
            goto quick_break;
        case 0x6:
            set_laser_preset_target(new int[4]{2016, 2016, 2016, 2016});
            goto quick_break;
        case 0xA:
            set_laser_preset_target(new int[4]{3024, 3024, 3024, 3024});
            goto quick_break;
        case 0xC:
            set_laser_preset_target(new int[4]{3024, 3024, 3024, 3024});
            goto quick_break;
        default: return;
        }
    }
    else {
        switch(curr_laser_idx | target) {
        case 0x3:
            set_laser_preset_target(new int[4]{0000, 0000, 0000, 0000});
            goto quick_break;
        case 0x5:
            set_laser_preset_target(new int[4]{0000, 0000, 0000, 0000});
            goto quick_break;
        case 0x9:
            set_laser_preset_target(new int[4]{0000, 0000, 0000, 0000});
            goto quick_break;
        case 0x6:
            set_laser_preset_target(new int[4]{1008, 1008, 1008, 1008});
            goto quick_break;
        case 0xA:
            set_laser_preset_target(new int[4]{1008, 1008, 1008, 1008});
            goto quick_break;
        case 0xC:
            set_laser_preset_target(new int[4]{2016, 2016, 2016, 2016});
            goto quick_break;
        default: return;
        }
    }
quick_break:
    curr_laser_idx = -target;
    QTimer::singleShot(3000, this, SLOT(laser_preset_reached()));
}

void UserPanel::on_FOCUS_FAR_BTN_pressed()
{
    ui->FOCUS_FAR_BTN->setText("x");
    focus_far();
}

inline void UserPanel::focus_far()
{
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x80, 0x00, 0x00, 0x81}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x80, 0x00, 0x00, 0x81}, 7), 7, 1, false);
}

inline void UserPanel::lens_stop() {
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
}

void UserPanel::set_zoom()
{
    zoom = ui->ZOOM_EDIT->text().toInt();

    unsigned sum = 0;
    uchar out[7];
    out[0] = 0xFF;
    out[1] = 0x01;
    out[2] = 0x00;
    out[3] = 0x4F;
    out[4] = (zoom >> 8) & 0xFF;
    out[5] = zoom & 0xFF;
    for (int i = 1; i < 6; i++)
        sum += out[i];
    out[6] = sum & 0xFF;

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(QByteArray((char*)out, 7), 7, 1, false);
    ptr_lens->communicate_display(QByteArray((char*)out, 7), 7, 1, false);
}

void UserPanel::set_focus()
{
    focus = ui->FOCUS_EDIT->text().toInt();

    unsigned sum = 0;
    uchar out[7];
    out[0] = 0xFF;
    out[1] = 0x01;
    out[2] = 0x00;
    out[3] = 0x4E;
    out[4] = (focus >> 8) & 0xFF;
    out[5] = focus & 0xFF;
    for (int i = 1; i < 6; i++)
        sum += out[i];
    out[6] = sum & 0xFF;

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(QByteArray((char*)out, 7), 7, 1, false);
    ptr_lens->communicate_display(QByteArray((char*)out, 7), 7, 1, false);
}


void UserPanel::start_laser()
{
    ptr_tcu->communicate_display(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 7, 0, false);
    QTimer::singleShot(100000, this, SLOT(init_laser()));
}

void UserPanel::init_laser()
{
    ui->LASER_BTN->setEnabled(true);
    ui->LASER_BTN->setText(tr("OFF"));
    ui->CURRENT_EDIT->setEnabled(true);
//    if (serial_port[3]) serial_port[3]->close();
//    setup_serial_port(serial_port + 3, 3, com_edit[3]->text(), 9600);
//    if (!serial_port[3]) return;

    // enable laser com
//    serial_port[3]->write("MODE:RMT 1\r", 11);
//    serial_port[3]->waitForBytesWritten(100);
//    QThread::msleep(100);
//    serial_port[3]->readAll();
    ptr_laser->communicate_display(QByteArray("MODE:RMT 1\r"), 11, 0, true, false);
    qDebug() << QString("MODE:RMT 1\r");

    // start
//    serial_port[3]->write("ON\r", 3);
//    serial_port[3]->waitForBytesWritten(100);
//    QThread::msleep(100);
//    serial_port[3]->readAll();
    ptr_laser->communicate_display(QByteArray("ON\r"), 3, 0, true, false);
    qDebug() << QString("ON\r");

    // enable external trigger
//    serial_port[3]->write("QSW:PRF 0\r", 10);
//    serial_port[3]->waitForBytesWritten(100);
//    QThread::msleep(100);
//    serial_port[3]->readAll();
    ptr_laser->communicate_display(QByteArray("QSW:PRF 0\r"), 10, 0, true, false);
    qDebug() << QString("QSW:PRF 0\r");

    update_current();

    ui->FIRE_LASER_BTN->setEnabled(true);
    ui->FIRE_LASER_BTN->click();
}

void UserPanel::change_mcp(int val)
{
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    ui->MCP_EDIT->setText(QString::number(val));
    ui->MCP_SLIDER->setValue(val);

    if (val == mcp) return;
    mcp = val;

//    convert_to_send_tcu(0x0A, mcp);
//    static QElapsedTimer t;
//    if (!h_grab_thread || t.elapsed() > (fps > 9 ? 900 / fps : 100)) communicate_display(0, convert_to_send_tcu(0x0A, mcp), 7, 1, false), t.start();
    ptr_tcu->communicate_display(convert_to_send_tcu(0x0A, mcp), 7, 1, false);
}

void UserPanel::change_gain(int val)
{
    if (val < 0) val = 0;
    switch (curr_cam->device_type) {
    case 1:
        if (val > 23) val = 23; break;
    case 2:
        if (val > 480) val = 480; break;
    default:
        break;
    }

    gain_analog_edit = val;
    curr_cam->gain_analog(false, &gain_analog_edit);
    ui->GAIN_EDIT->setText(QString::number((int)gain_analog_edit));
    ui->GAIN_SLIDER->setValue(gain_analog_edit);
}

void UserPanel::change_delay(int val)
{
    if (abs(delay_dist - val) < 1) return;
    delay_dist = val;
    update_delay();
}

void UserPanel::change_focus_speed(int val)
{
    if (val < 1)  val = 1;
    if (val > 64) val = 64;
    ui->FOCUS_SPEED_EDIT->setText(QString::asprintf("%d", val));
    ui->FOCUS_SPEED_SLIDER->setValue(val);
    if (val > 1) val -= 1;

#ifndef LVTONG
    static QElapsedTimer t;
    if (t.elapsed() < 100) return;
    t.restart();
#endif

//    int temp_serial_port_id = pref->share_port && serial_port[0]->isOpen() ? 0 : 2;

    uchar out_data[9] = {0};
    out_data[0] = 0xB0;
    out_data[1] = 0x01;
    out_data[2] = 0xA0;
    out_data[3] = 0x01;
    out_data[4] = val;
    out_data[5] = val;
    out_data[6] = val;
    out_data[7] = val;

    out_data[8] = (4 * (uint)val + 0xA2) & 0xFF;
//    communicate_display(temp_serial_port_id, QByteArray((char*)out_data, 9), 9, 0, false);
    ptr_lens->communicate_display(QByteArray((char*)out_data, 9), 9, 0, false);

    out_data[0] = 0xB0;
    out_data[1] = 0x02;
    out_data[2] = 0xA0;
    out_data[3] = 0x01;
    out_data[4] = val;
    out_data[5] = val;
    out_data[6] = val;
    out_data[7] = val;

    out_data[8] = (4 * (uint)val + 0xA3) & 0xFF;
//    if (multi_laser_lenses) communicate_display(temp_serial_port_id, QByteArray((char*)out_data, 9), 9, 0, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(QByteArray((char*)out_data, 9), 9, 0, false);
}

void UserPanel::on_ZOOM_IN_BTN_released()
{
    ui->ZOOM_IN_BTN->setText("+");
    lens_stop();
}

void UserPanel::on_ZOOM_OUT_BTN_released()
{
    ui->ZOOM_OUT_BTN->setText("-");
    lens_stop();
}

void UserPanel::on_FOCUS_NEAR_BTN_released()
{
    ui->FOCUS_NEAR_BTN->setText("+");
    lens_stop();
}

void UserPanel::on_FOCUS_FAR_BTN_released()
{
    ui->FOCUS_FAR_BTN->setText("-");
    lens_stop();
}

void UserPanel::keyPressEvent(QKeyEvent *event)
{
    static QLineEdit *edit;

    if (event->key() == Qt::Key_Escape) {
        if(!this->focusWidget()) return;
        this->focusWidget()->clearFocus();
        data_exchange(false);
        return;
    }
    switch (event->modifiers()) {
    case Qt::KeypadModifier:
    case Qt::NoModifier:
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (!(this->focusWidget())) break;
            edit = qobject_cast<QLineEdit*>(this->focusWidget());
            if (!edit) break;

            for (int i = 0; i < 5; i++) {
                if (edit == com_edit[i]) {
                    if (serial_port[i]) serial_port[i]->close();
//                    setup_serial_port(serial_port + i, i, edit->text(), 9600);
                }
            }
            if (edit == ui->FREQ_EDIT) {
                switch (hz_unit) {
                // kHz
                case 0: rep_freq = ui->FREQ_EDIT->text().toFloat(); break;
                // Hz
                case 1: rep_freq = ui->FREQ_EDIT->text().toFloat() / 1000; break;
                default: break;
                }
                ptr_tcu->communicate_display(convert_to_send_tcu(0x00, 1.25e5 / rep_freq), 7, 1, false);
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_U) {
                depth_of_view = (edit->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt()) * dist_ns;
                update_gate_width();
            }
            else if (edit == ui->LASER_WIDTH_EDIT_U) {
                laser_width = edit->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt();
                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_N) {
                if (edit->text().toInt() > 999) depth_of_view = edit->text().toInt() * dist_ns;
                else depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000) * dist_ns;
                update_gate_width();
            }
            else if (edit == ui->LASER_WIDTH_EDIT_N) {
                if (edit->text().toInt() > 999) {
                    laser_width = edit->text().toInt();
                    ui->LASER_WIDTH_EDIT_U->setText(QString::number(laser_width / 1000));
                    ui->LASER_WIDTH_EDIT_N->setText(QString::number(laser_width % 1000));
                }
                else {
                    laser_width = edit->text().toInt() + ui->LASER_WIDTH_EDIT_U->text().toInt() * 1000;
                    ui->LASER_WIDTH_EDIT_N->setText(QString::number(laser_width % 1000));
                }
                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
            }
            else if (edit == ui->DELAY_A_EDIT_U) {
                delay_dist = (edit->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt()) * dist_ns;
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_U) {
                delay_dist = (edit->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() - delay_n_n) * dist_ns;
                update_delay();
            }
            else if (edit == ui->DELAY_A_EDIT_N) {
                if (edit->text().toInt() > 999) delay_dist = edit->text().toInt() * dist_ns;
                else delay_dist = (edit->text().toInt() + ui->DELAY_A_EDIT_U->text().toInt() * 1000) * dist_ns;
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_N) {
                if (edit->text().toInt() > 999) delay_dist = edit->text().toInt() * dist_ns;
                else delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 - delay_n_n) * dist_ns;
                update_delay();
            }
            else if (edit == ui->DELAY_N_EDIT_N) {
                delay_n_n = edit->text().toInt();
                update_delay();
            }
            else if (edit == ui->MCP_EDIT) {
                ui->MCP_SLIDER->setValue(ui->MCP_EDIT->text().toInt());
            }
            else if (edit == ui->STEPPING_EDIT) {
                switch (base_unit) {
                case 0: stepping = edit->text().toFloat(); break;
                case 1: stepping = edit->text().toFloat() * 1000; break;
                case 2: stepping = edit->text().toFloat() / dist_ns; break;
                default: break;
                }
                setup_stepping(base_unit);
            }
            else if (edit == ui->ZOOM_EDIT) {
                set_zoom();
            }
            else if (edit == ui->FOCUS_EDIT) {
                set_focus();
            }
            else if (edit == ui->CCD_FREQ_EDIT || edit == ui->DUTY_EDIT || edit == ui->GAIN_EDIT) {
                on_SET_PARAMS_BUTTON_clicked();
            }
            else if (edit == ui->FOCUS_SPEED_EDIT) {
                change_focus_speed(edit->text().toInt());
            }
            else if (edit == ui->CURRENT_EDIT) {
                update_current();
            }
            else if (edit == ui->ANGLE_H_EDIT) {
                angle_h = ui->ANGLE_H_EDIT->text().toFloat();
                set_ptz_angle();
            }
            else if (edit == ui->ANGLE_V_EDIT) {
                angle_v = ui->ANGLE_V_EDIT->text().toFloat();
                set_ptz_angle();
            }
            this->focusWidget()->clearFocus();
            break;
        // 100m => 667ns, 10m => 67ns
        case Qt::Key_W:
            delay_dist += stepping * 5 * dist_ns;
            update_delay();
            break;
        case Qt::Key_S:
            delay_dist -= stepping * 5 * dist_ns;
            update_delay();
            break;
        case Qt::Key_D:
            delay_dist += stepping * dist_ns;
            update_delay();
            break;
        case Qt::Key_A:
            delay_dist -= stepping * dist_ns;
            update_delay();
            break;
        // 50m => 333ns, 5m => 33ns
        case Qt::Key_I:
            depth_of_view += stepping * 5 * dist_ns;
            update_gate_width();
            break;
        case Qt::Key_K:
            depth_of_view -= stepping * 5 * dist_ns;
            update_gate_width();
            break;
        case Qt::Key_L:
            depth_of_view += stepping * dist_ns;
            update_gate_width();
            break;
        case Qt::Key_J:
            depth_of_view -= stepping * dist_ns;
            update_gate_width();
            break;
        default: break;
        }
        break;
    case Qt::AltModifier:
        switch (event->key()) {
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
            goto_laser_preset(1 << (event->key() - Qt::Key_1));
            break;
        case Qt::Key_S:
//            ui->TITLE->prog_settings->show();
//            ui->TITLE->prog_settings->raise();
            pref->show();
            pref->raise();
            break;
        case Qt::Key_E:
            export_config();
            break;
        case Qt::Key_R:
            prompt_for_config_file();
            break;
        case Qt::Key_Q:
            std::accumulate(use_tcp, use_tcp + 5, 0) ? disconnect_from_serial_server_tcp() : connect_to_serial_server_tcp();
            break;
        case Qt::Key_L:
            laser_settings->show();
            laser_settings->raise();
       default: break;
        }
    case Qt::ControlModifier:
        switch (event->key()) {
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
            transparent_transmission_file(event->key() - Qt::Key_1);
            break;
        default:break;
        }
        break;
    case Qt::ControlModifier + Qt::ShiftModifier:
        switch (event->key()) {
        case Qt::Key_C:
            QApplication::clipboard()->setPixmap(ui->SOURCE_DISPLAY->grab());
            break;
        default: break;
        }
        break;
    default: break;
    }
}

void UserPanel::resizeEvent(QResizeEvent *event)
{
    int ww = ui->IMG_3D_CHECK->isChecked() ? w + 104 : w;
    QRect window = this->geometry();
//    qDebug() << window;
    int grp_width = ui->RIGHT->width();

    ui->RIGHT->move(window.width() - 12 - grp_width, 40);

    QRect region = ui->SOURCE_DISPLAY->geometry();
    QPoint center = ui->SOURCE_DISPLAY->center;

    int mid_width = window.width() - 12 - grp_width - 10 - ui->LEFT->width();
    ui->MID->setGeometry(10 + ui->LEFT->width(), 40, mid_width, window.height() - 50);
    int width = mid_width - 20, height = width * h / ww;
//    qDebug("width %d, height %d\n", width, height);
    int height_constraint = window.height() - 30 - 50 - 32,
        width_constraint = height_constraint * ww / h;
//    qDebug("width_constraint %d, height_constraint %d\n", width_constraint, height_constraint);
    if (width < width_constraint) width_constraint = width, height_constraint = width_constraint * h / ww;
    if (height > height_constraint) {
        center = center * width_constraint / region.width();
        region.setRect(10 + (width - width_constraint) / 2, 40, width_constraint, height_constraint);
    }
    else {
        center = center * width / region.width();
        region.setRect(10, 40, width, height);
    }
    ui->TITLE->resize(this->width(), 30);
    ui->STATUS->setGeometry(0, this->height() - 32, this->width(), 32);
    ui->ZOOM_TOOL->move(region.x(), 15);
    ui->SELECT_TOOL->move(region.x() + 25, 15);
    ui->PTZ_TOOL->move(region.x() + 50, 15);
    ui->CURR_COORD->move(region.x() + 80, 5);
    ui->START_COORD->move(ui->CURR_COORD->geometry().right() + 20, 5);
    ui->SHAPE_INFO->move(ui->START_COORD->geometry().right() + 20, 5);
    ui->INFO_CHECK->move(region.right() - 60, 0);
    ui->CENTER_CHECK->move(region.right() - 60, 20);
    ui->HIDE_BTN->move(ui->MID->geometry().left() - 8, this->geometry().height() / 2 - 10);
//    ui->HIDE_BTN->move(ui->LEFT->geometry().right() - 10 + (ui->SOURCE_DISPLAY->geometry().left() + ui->MID->geometry().left() - ui->LEFT->geometry().right() + 10) / 2 + 2, this->geometry().height() / 2 - 10);
    ui->RULER_H->setGeometry(region.left(), region.bottom() - 10, region.width(), 32);
    ui->RULER_V->setGeometry(region.right() - 10, region.top(), 32, region.height());
#ifdef LVTONG
    ui->FISHNET_RESULT->move(region.right() - 170, 5);
#endif

    image_mutex.lock();
    ui->SOURCE_DISPLAY->setGeometry(region);
    ui->SOURCE_DISPLAY->update_roi(center);
    qInfo("display region x: %d, y: %d, w: %d, h: %d", region.x(), region.y(), region.width(), region.height());
    image_mutex.unlock();

    event->accept();
}

void UserPanel::mousePressEvent(QMouseEvent *event)
{
    QMainWindow::mousePressEvent(event);

    if (event->button() != Qt::LeftButton) return;
    QPoint diff = event->globalPos() - pos();
    if (diff.y() < 5) {
        if (diff.x() < 5) resize_place = 11;
        else if (diff.x() > width() - 5) resize_place = 13;
        else resize_place = 12;
    }
    else if (diff.y() > height() - 5) {
        if (diff.x() < 5) resize_place = 31;
        else if (diff.x() > width() - 5) resize_place = 33;
        else resize_place = 32;
    }
    else {
        if (diff.x() < 5) resize_place = 21;
        else if (diff.x() > width() - 5) resize_place = 23;
    }
    mouse_pressed = true;
    prev_pos = event->globalPos();
}

void UserPanel::mouseMoveEvent(QMouseEvent *event)
{
    QMainWindow::mouseMoveEvent(event);
    if (ui->TITLE->is_maximized) return;

    if (mouse_pressed) {
        QPoint displacement;
        int ww = width(), hh = height(), xx = pos().x(), yy = pos().y();
        switch (resize_place) {
        case 11:
            displacement = event->globalPos() - pos();
            if (ww - displacement.x() >= minimumWidth()) ww -= displacement.x(), xx += displacement.x();
            if (hh - displacement.y() >= minimumHeight()) hh -= displacement.y(), yy += displacement.y();
            ui->TITLE->pressed = false;
            window()->setGeometry(xx, yy, ww, hh);
            break;
        case 21:
            displacement = event->globalPos() - pos();
            if (ww - displacement.x() >= minimumWidth()) ww -= displacement.x(), xx += displacement.x();
            window()->setGeometry(xx, yy, ww, hh);
            break;
        case 31:
            displacement = event->globalPos() - geometry().bottomLeft();
            if (ww - displacement.x() >= minimumWidth()) ww -= displacement.x(), xx += displacement.x();
            if (hh + displacement.y() >= minimumHeight()) hh += displacement.y();
            window()->setGeometry(xx, yy, ww, hh);
            break;
        case 12:
            displacement = event->globalPos() - pos();
            if (hh - displacement.y() >= minimumHeight()) hh -= displacement.y(), yy += displacement.y();
            ui->TITLE->pressed = false;
            window()->setGeometry(xx, yy, ww, hh);
            break;
        case 13:
        case 22:
            return;
        case 23:
            displacement = event->globalPos() - geometry().topRight();
            if (ww + displacement.x() >= minimumWidth()) ww += displacement.x();
            window()->resize(QSize(ww, hh));
            break;
        case 32:
            displacement = event->globalPos() - geometry().bottomLeft();
            if (hh + displacement.y() >= minimumHeight()) hh += displacement.y();
            window()->resize(QSize(ww, hh));
            break;
        case 33:
            displacement = event->globalPos() - geometry().bottomRight();
            if (ww + displacement.x() >= minimumWidth()) ww += displacement.x();
            if (hh + displacement.y() >= minimumHeight()) hh += displacement.y();
            window()->resize(QSize(ww, hh));
            break;
        default:
            return;
        }
        prev_pos = event->globalPos();
    }
    // mouse button not pressed
    else {
        QPoint diff = cursor().pos() - pos();
        // cursor in title bar
        if (diff.y() < 30 && diff.x() > width() - 5) setCursor(cursor_curr_pointer);
        // cursor in main window
        else if (diff.y() < 5) {
            // cursor @ topleft corner
            if (diff.x() < 5) setCursor(cursor_curr_resize_md);
            // cursor on min, max, exit button
            else if (diff.x() > width() - 120) setCursor(cursor_curr_pointer);
            // cursor @ top border
            else setCursor(cursor_curr_resize_v);
        }
        else if (diff.y() > height() - 5) {
            // cursor @ bottom left corner
            if (diff.x() < 5) setCursor(cursor_curr_resize_sd);
            // cursor @ bottom right corner
            else if (diff.x() > width() - 5) setCursor(cursor_curr_resize_md);
            // cursor @ bottom border
            else setCursor(cursor_curr_resize_v);
        }
        else {
            // cursor @ left border
            if (diff.x() < 5) setCursor(cursor_curr_resize_h);
            // cursor @ right border
            else if (diff.x() > width() - 5) setCursor(cursor_curr_resize_h);
            // cursor inside window
            else setCursor(cursor_curr_pointer);
        }
    }
}

void UserPanel::mouseReleaseEvent(QMouseEvent *event)
{
    QMainWindow::mouseReleaseEvent(event);

    if (event->button() != Qt::LeftButton) return;
    mouse_pressed = false;
    resize_place = 22;
}

void UserPanel::mouseDoubleClickEvent(QMouseEvent *event)
{
    QMainWindow::mouseDoubleClickEvent(event);

    mouse_pressed = false;
}

void UserPanel::dragEnterEvent(QDragEnterEvent *event)
{
    QMainWindow::dragEnterEvent(event);

    if (event->mimeData()->urls().length() < 6) event->acceptProposedAction();
}

void UserPanel::dropEvent(QDropEvent *event)
{
    QMainWindow::dropEvent(event);

    static QMimeDatabase mime_db;

    if (event->mimeData()->urls().length() == 1) {
        QString file_name = event->mimeData()->urls().first().toLocalFile();
        QFileInfo file_info = QFileInfo(file_name);
        if (file_info.exists() && file_info.size() > 2e9) {
            QMessageBox::warning(this, "PROMPT", tr("File size limit (2 Gb) exceeded"));
            return;
        }
        // TODO: check file type before reading/processing file
//        QMimeType file_type = mime_db.mimeTypeForFile(file_name);
        if (mime_db.mimeTypeForFile(file_name).name().startsWith("video")) {
            load_video_file(file_name);
            return;
        }

        if (!load_image_file(file_name, true)) load_config(file_name);

    }
    else {
        bool result = true, init = true;
        for (QUrl path : event->mimeData()->urls()) {
            bool temp = load_image_file(path.toLocalFile(), init);
            if (init) init = !temp;
            result |= temp;
        }
        if (!result) QMessageBox::warning(this, "PROMPT", tr("Some of the image files cannot be read"));
    }
}

bool UserPanel::load_image_file(QString filename, bool init)
{
    grab_image = false;
    if (!display_mutex.tryLock(1e3)) return false;

    QImage qimage_temp;
    cv::Mat mat_temp;
    bool tiff = false;

    if (filename.endsWith(".tif") || filename.endsWith(".tiff")) tiff = true;

    if (tiff) {
        if (UserPanel::load_image_tif(mat_temp, filename)) return false;
    }
    else {
        if (!qimage_temp.load(filename)) return false;
    }

    if (device_on) {
        QMessageBox::warning(this, "PROMPT", tr("Cannot read local image while cam is on"));
        return true;
    }

    if (init) {
        if (tiff) start_static_display(mat_temp.cols, mat_temp.rows, false, 8 * (mat_temp.depth() / 2 + 1));
        else      start_static_display(qimage_temp.width(), qimage_temp.height(), qimage_temp.format() != QImage::Format_Indexed8);
    }
    qimage_temp = qimage_temp.convertToFormat(is_color ? QImage::Format_RGB888 : QImage::Format_Indexed8);

    if (tiff) img_q.push(mat_temp.clone());
    else      img_q.push(cv::Mat(h, w, is_color ? CV_8UC3 : CV_8UC1, (uchar*)qimage_temp.bits(), qimage_temp.bytesPerLine()).clone());

    display_mutex.unlock();
    return true;
}

void init_filter_graph(AVFormatContext *format_context, int video_stream_idx, AVFilterGraph *filter_graph, AVCodecContext *codec_context,
                       AVFilterContext **buffersink_ctx, AVFilterContext **buffersrc_ctx) {
    char args[512];
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
//    std::shared_ptr<AVFilterInOut*> deleter_input_filter(&inputs, avfilter_inout_free);
//    std::shared_ptr<AVFilterInOut*> deleter_output_filter(&outputs, avfilter_inout_free);

    AVRational time_base = format_context->streams[video_stream_idx]->time_base;
    static enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

    if (!outputs || !inputs || !filter_graph) return;

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             codec_context->width, codec_context->height, codec_context->pix_fmt, time_base.num, time_base.den,
             codec_context->sample_aspect_ratio.num, codec_context->sample_aspect_ratio.den);

    if (avfilter_graph_create_filter(buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph) < 0) return;

    /* buffer video sink: to terminate the filter chain. */
    if (avfilter_graph_create_filter(buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph) < 0) return;

    if (av_opt_set_int_list(*buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) return;

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = *buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = *buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if (avfilter_graph_parse_ptr(filter_graph, "format=gray", &inputs, &outputs, NULL) < 0) return;

    if (avfilter_graph_config(filter_graph, NULL) < 0) return;

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
}

typedef struct {
    time_t lasttime;
} VidTimer;

static int ffmpeg_interrupt_callback(void *p) {
    time_t *t0 = (time_t *)p;
    if (*t0 > 0 && time(NULL) - *t0 > 3) return 1;
    return 0;
}

int UserPanel::load_video_file(QString filename, bool format_gray)
{
//    video_input->setMedia(QUrl(filename));
//    video_input->setMuted(true);
//    video_input->play();
//    video_surface->frame_count = 0;
    std::thread t([this, filename, format_gray](){
        grab_image = false;
        if (!display_mutex.tryLock(1e3)) return -1;

        AVFormatContext *format_context = avformat_alloc_context();

        // process timeout
        static time_t start_time = 0;

        format_context->interrupt_callback.callback = ffmpeg_interrupt_callback;
        format_context->interrupt_callback.opaque = &start_time;

        // open input video
        start_time = time(NULL);
        if (avformat_open_input(&format_context, filename.toUtf8().constData(), NULL, NULL) != 0) { display_mutex.unlock(); return -2; }

        // fetch video info
        if (avformat_find_stream_info(format_context, NULL) < 0) { display_mutex.unlock(); return -2; }
//        std::shared_ptr<AVFormatContext*> closer_format_context(&format_context, avformat_close_input);

        // find the first video stream
        int video_stream_idx = -1;
        for (int i = 0; i < format_context->nb_streams; i++) {
            if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_idx = i;
                break;
            }
        }
        if (video_stream_idx == -1) { display_mutex.unlock(); return -2; }

        // point the codec parameter to the first stream's
        static AVCodecParameters *codec_param;
        codec_param = format_context->streams[video_stream_idx]->codecpar;

        const AVCodec *codec = avcodec_find_decoder(codec_param->codec_id);
        if (codec == NULL) { display_mutex.unlock(); return -3; }

        AVCodecContext *codec_context = avcodec_alloc_context3(NULL);
        if (avcodec_parameters_to_context(codec_context, codec_param) < 0) { display_mutex.unlock(); return -3; }
        // close the decoder when the program exits
//        std::shared_ptr<AVCodecContext> closer_codec_context(codec_context, avcodec_close);

        if (avcodec_open2(codec_context, codec, NULL) < 0) { display_mutex.unlock(); return -3; }

        // create frame
        AVFrame *frame = av_frame_alloc();
//        std::shared_ptr<AVFrame*> deleter_frame(&frame, av_frame_free);
        AVFrame *frame_result = av_frame_alloc();
//        std::shared_ptr<AVFrame*> deleter_frame_result(&frame_result, av_frame_free);
        AVFrame *frame_filter = av_frame_alloc();
        cv::Mat cv_frame(codec_context->height, codec_context->width, CV_MAKETYPE(CV_8U, format_gray ? 1 : 3));

        // allocate data buffer
        AVPixelFormat pixel_fmt = format_gray ? AV_PIX_FMT_NV12 : AV_PIX_FMT_RGB24;
        int byte_num = av_image_get_buffer_size(pixel_fmt, codec_context->width, codec_context->height, 1);
        uint8_t *buffer = (uint8_t *)av_malloc(byte_num * sizeof(uint8_t));
//        std::shared_ptr<uint8_t> deleter_buffer(buffer, av_free);

        av_image_fill_arrays(frame_result->data, frame_result->linesize, buffer, pixel_fmt, codec_context->width, codec_context->height, 1);

        SwsContext *sws_context = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt,
                                     codec_context->width, codec_context->height, pixel_fmt, SWS_BILINEAR, NULL, NULL, NULL);
//        std::shared_ptr<SwsContext> deleter_sws_context(sws_context, sws_freeContext);

        start_static_display(codec_context->width, codec_context->height, !format_gray, 8, -2);

        AVPacket packet;
        static AVFilterContext *buffersink_ctx = NULL;
        static AVFilterContext *buffersrc_ctx = NULL;
        AVFilterGraph *filter_graph = avfilter_graph_alloc();
        if (format_gray) init_filter_graph(format_context, video_stream_idx, filter_graph, codec_context, &buffersink_ctx, &buffersrc_ctx);

        AVRational time_base = format_context->streams[video_stream_idx]->time_base;
        AVRational time_base_q = { 1, AV_TIME_BASE };
        long long last_pts = AV_NOPTS_VALUE;
        long long delay;
        int result;
        while (grab_image) {
            start_time = time(NULL);
            if ((result = av_read_frame(format_context, &packet)) < 0) { qDebug() << result; continue; }

            // release packet allocated by av_read_frame after iteration
            std::shared_ptr<AVPacket> deleter_packet(&packet, av_packet_unref);

            // if the packet contains data of video stream
            if (packet.stream_index == video_stream_idx) {
                // decode packet
                avcodec_send_packet(codec_context, &packet);
                // returns 0 only after decoding the entire frame
                if (avcodec_receive_frame(codec_context, frame) == 0) {
//                    qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss.zzz");
                    if (format_gray) {
                        // push the decoded frame into the filtergraph
                        if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) break;

                        // pull filtered frames from the filtergraph
                        av_frame_unref(frame_filter);
                        int ret = av_buffersink_get_frame(buffersink_ctx, frame_filter);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                    }
                    else {
                        if (frame->pts != AV_NOPTS_VALUE) {
                            if (last_pts != AV_NOPTS_VALUE) {
                                delay = av_rescale_q(frame->pts - last_pts, time_base, time_base_q);
                                if (delay > 0 && delay < 1000000) av_usleep(delay);
                            }
                            last_pts = frame->pts;
                        }
                        sws_scale(sws_context, frame->data, frame->linesize, 0, codec_context->height, frame_result->data, frame_result->linesize);
                    }

                    cv_frame.data = (format_gray ? frame_filter : frame_result)->data[0];
                    img_q.push(cv_frame.clone());
                }
            }
        }
        avfilter_graph_free(&filter_graph);
        sws_freeContext(sws_context);
        av_free(&buffer);
        av_frame_free(&frame_filter);
        av_frame_free(&frame_result);
        av_frame_free(&frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);

        display_mutex.unlock();
        return 0;
    });
    t.detach();

    return 0;
}

void UserPanel::update_pixel_depth(int depth)
{
    this->pixel_depth = depth;
    status_bar->img_pixel_depth->set_text(QString::number(depth) + "-bit");
}

void UserPanel::start_static_display(int width, int height, bool is_color, int pixel_depth, int device_type)
{
    if (grab_image || h_grab_thread) {
        grab_image = false;
        if (h_grab_thread) {
            h_grab_thread->quit();
            h_grab_thread->wait();
            delete h_grab_thread;
            h_grab_thread = NULL;
        }
    }
    while (grab_thread_state) ;
    std::queue<cv::Mat>().swap(img_q);

    image_mutex.lock();
    w = width;
    h = height;
    this->is_color = is_color;
//    pixel_depth = img.depth();
    update_pixel_depth(pixel_depth);
    this->device_type = device_type;
    image_mutex.unlock();

    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);

    grab_image = true;
    h_grab_thread = new GrabThread((void*)this);
    if (h_grab_thread == NULL) {
        grab_image = false;
        QMessageBox::warning(this, "PROMPT", tr("Cannot display image"));
        return;
    }
    h_grab_thread->start();
    start_grabbing = true;
    ui->SOURCE_DISPLAY->is_grabbing = true;
    enable_controls(true);
    ui->START_BUTTON->setEnabled(false);
}

void UserPanel::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (this->focusWidget()) this->focusWidget()->clearFocus();
    this->setFocus();
//    qDebug() << this->focusWidget();
}

void UserPanel::on_SAVE_AVI_BUTTON_clicked()
{
    static QString raw_avi;
    if (record_original) {
//        curr_cam->stop_recording(0);
        image_mutex.lock();
        vid_out[0].release();
        image_mutex.unlock();
        QString dest = save_location + "/" + raw_avi.section('/', -1, -1);
        std::thread t(UserPanel::move_to_dest, QString(raw_avi), QString(dest));
        t.detach();
//        QFile::rename(raw_avi, dest);
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        image_mutex.lock();
        raw_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_raw.avi");
        vid_out[0].open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w, h), is_color || image_3d);
        image_mutex.unlock();
    }
    record_original = !record_original;
    ui->SAVE_AVI_BUTTON->setText(record_original ? tr("Stop") : tr("ORI"));
}

void UserPanel::on_SCAN_BUTTON_clicked()
{
    bool start_scan = ui->SCAN_BUTTON->text() == tr("Scan");

//    while (updated);

    if (start_scan) {
        scan_3d = cv::Mat::zeros(h, w, CV_64F);
        scan_sum = cv::Mat::zeros(h, w, CV_64F);
        scan_idx = 0;

        rep_freq = scan_config->rep_freq;
        delay_dist = scan_config->starting_delay * dist_ns;
        scan_stopping_delay = scan_config->ending_delay;
        scan_step = scan_config->step_size_delay * dist_ns;

        if (scan_config->capture_scan_ori || scan_config->capture_scan_res) {
            scan_name = "scan_" + QDateTime::currentDateTime().toString("MMdd_hhmmss");
            if (!QDir(save_location + "/" + scan_name).exists()) {
                QDir().mkdir(save_location + "/" + scan_name);
                QDir().mkdir(save_location + "/" + scan_name + "/ori_bmp");
                QDir().mkdir(save_location + "/" + scan_name + "/res_bmp");
            }

            QFile params(save_location + "/" + scan_name + "/scan_params");
            params.open(QIODevice::WriteOnly);
            params.write(QString::asprintf("starting delay:     %06d ns\n"
                                           "ending delay:       %06d ns\n"
                                           "frames count:       %06d\n"
                                           "stepping size:      %.2f ns\n"
                                           "repeated frequency: %06d kHz\n"
                                           "laser width:        %06d ns\n"
                                           "gate width:         %06d ns\n"
                                           "MCP:                %03d",
                                           scan_config->starting_delay, scan_config->ending_delay, scan_config->frame_count, scan_config->step_size_delay,
                                           (int)rep_freq, laser_width, gate_width_a_u * 1000 + gate_width_a_n, mcp).toUtf8());
            params.close();
        }

        on_CONTINUE_SCAN_BUTTON_clicked();
    }
    else {
        emit update_scan(false);
        scan = false;

//        for (uint i = scan_q.size(); i; i--) qDebug("dist %f", scan_q.front()), scan_q.pop_front();

        scan_3d = scan_3d.mul(1 / scan_sum);
        scan_3d *= 2.25;
        cv::Mat scan_result_3d;
        ImageProc::paint_3d(scan_3d, scan_result_3d, 0, scan_config->starting_delay * dist_ns, scan_config->ending_delay * dist_ns);
        cv::imwrite((TEMP_SAVE_LOCATION + "/" + scan_name + "_result.bmp").toLatin1().constData(), scan_result_3d);
        QFile::rename(TEMP_SAVE_LOCATION + "/" + scan_name + "_result.bmp", save_location + "/" + scan_name + "/result.bmp");
    }

    ui->SCAN_BUTTON->setText(start_scan ? tr("Stop") : tr("Scan"));
}

void UserPanel::on_CONTINUE_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);

    update_delay();
    ptr_tcu->communicate_display(convert_to_send_tcu(0x00, (uint)(1.25e5 / rep_freq)), 7, 1, false);
}

void UserPanel::on_RESTART_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);
    delay_dist = 200;

    on_CONTINUE_SCAN_BUTTON_clicked();
}

void UserPanel::on_SCAN_CONFIG_BTN_clicked()
{
    if (scan_config->isVisible()) {
        scan_config->hide();
    }
    else {
        scan_config->show();
        scan_config->raise();
    }
}

void UserPanel::enable_scan_options(bool show)
{
//    if (delay_dist > scan_q.back()) scan_q.push_back(delay_dist);
    ui->CONTINUE_SCAN_BUTTON->setEnabled(!scan);
    ui->RESTART_SCAN_BUTTON->setEnabled(!scan);

    if (show) {
        ui->CONTINUE_SCAN_BUTTON->show();
        ui->RESTART_SCAN_BUTTON->show();
    }
    else {
        ui->CONTINUE_SCAN_BUTTON->hide();
        ui->RESTART_SCAN_BUTTON->hide();
    }
}

void UserPanel::on_FRAME_AVG_CHECK_stateChanged(int arg1)
{
    if (arg1) {
        seq_sum.release();
        frame_a_sum.release();
        frame_b_sum.release();
        for (cv::Mat &m: seq) m.release();
        seq_idx = 0;
    }
    calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 4 + 4;
}

void UserPanel::on_SAVE_RESULT_BUTTON_clicked()
{
    if (!QDir(save_location + "/res_bmp").exists()) QDir().mkdir(save_location + "/res_bmp");
    if (device_type == -1) save_modified = 1;
    else{
        save_modified = !save_modified;
        ui->SAVE_RESULT_BUTTON->setText(save_modified ? tr("Stop") : tr("RES"));
    }
}

void UserPanel::on_IRIS_OPEN_BTN_pressed()
{
    ui->IRIS_OPEN_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x20, 0x00, 0x00, 0x22}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x01, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04}, 7), 7, 1, false);

    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x20, 0x00, 0x00, 0x22}, 7), 7, 1, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x01, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04}, 7), 7, 1, false);

}

void UserPanel::on_IRIS_CLOSE_BTN_pressed()
{
    ui->IRIS_CLOSE_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x04, 0x00, 0x00, 0x00, 0x05}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x40, 0x00, 0x00, 0x42}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x80, 0x00, 0x00, 0x82}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x04, 0x00, 0x00, 0x00, 0x06}, 7), 7, 1, false);

    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x04, 0x00, 0x00, 0x00, 0x05}, 7), 7, 1, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x40, 0x00, 0x00, 0x42}, 7), 7, 1, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x80, 0x00, 0x00, 0x82}, 7), 7, 1, false);
    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x04, 0x00, 0x00, 0x00, 0x06}, 7), 7, 1, false);

}

void UserPanel::on_IRIS_OPEN_BTN_released()
{
    ui->IRIS_OPEN_BTN->setText("+");
    lens_stop();
}

void UserPanel::on_IRIS_CLOSE_BTN_released()
{
    ui->IRIS_CLOSE_BTN->setText("-");
    lens_stop();
}

void UserPanel::laser_preset_reached()
{
    lens_stop();
    curr_laser_idx = -curr_laser_idx;
}

void UserPanel::on_LASER_BTN_clicked()
{
#ifdef LVTONG
    if (ui->LASER_BTN->text() == "ON") {
        ui->LASER_BTN->setEnabled(false);
        ptr_tcu->communicate_display(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 7, 0, false);
        QTimer::singleShot(4000, this, SLOT(start_laser()));
    }
    else {
//        if (serial_port[3] && serial_port[3]->isOpen()) {
//            serial_port[3]->write("OFF\r", 4);
//            serial_port[3]->waitForBytesWritten(100);
//            QThread::msleep(100);
//            serial_port[3]->readAll();
//        }
        ptr_laser->communicate_display(QByteArray("OFF\r"), 4, 0, true, false);
        ptr_tcu->communicate_display(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x02, 0x99}, 7), 7, 0, false);

        ui->LASER_BTN->setText(tr("ON"));
        ui->CURRENT_EDIT->setEnabled(false);
        ui->FIRE_LASER_BTN->setEnabled(false);
    }
#else
    pref->ui->LASER_ENABLE_CHK->click();
    ui->LASER_BTN->setText(ui->LASER_BTN->text() == "ON" ? tr("OFF") : tr("ON"));

#endif
}

void UserPanel::on_GET_LENS_PARAM_BTN_clicked()
{
//    QByteArray read = (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56}, 7), 7, 7, true);
    QByteArray read = ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56}, 7), 7, 7, true);
    zoom = ((read[4] & 0xFF) << 8) + read[5] & 0xFF;

//    read = (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57}, 7), 7, 7, true);
    read = ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57}, 7), 7, 7, true);
    focus = ((read[4] & 0xFF) << 8) + read[5] & 0xFF;

    ui->ZOOM_EDIT->setText(QString::asprintf("%d", zoom));
    ui->FOCUS_EDIT->setText(QString::asprintf("%d", focus));
}

void UserPanel::on_AUTO_FOCUS_BTN_clicked()
{
    focus_direction = 1;
    focus_far();
}

void UserPanel::on_HIDE_BTN_clicked()
{
    hide_left ^= 1;
    ui->LEFT->setGeometry(10, 40, hide_left ? 0 : 210, 631);
//    ui->HIDE_BTN->setText(hide_left ? ">" : "<");
    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s/%s);", app_theme ? "light" : "dark", hide_left ? "right" : "left"));
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
}

void UserPanel::on_COM_DATA_RADIO_clicked()
{
    display_option = 0;
    ui->DATA_EXCHANGE->show();
    ui->HIST_DISPLAY->hide();
    ui->PTZ_GRP->hide();
}

void UserPanel::on_HISTOGRAM_RADIO_clicked()
{
    display_option = 1;
    ui->DATA_EXCHANGE->hide();
    ui->HIST_DISPLAY->show();
    ui->PTZ_GRP->hide();
}

void UserPanel::on_PTZ_RADIO_clicked()
{
    display_option = 2;
    ui->DATA_EXCHANGE->hide();
    ui->HIST_DISPLAY->hide();
    ui->PTZ_GRP->show();
}

void UserPanel::screenshot()
{
    image_mutex.lock();
    QPixmap screenshot = window()->grab();
    screenshot.save(save_location + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    QApplication::clipboard()->setPixmap(screenshot);
    image_mutex.unlock();
}

void UserPanel::on_ZOOM_TOOL_clicked()
{
    ui->SELECT_TOOL->setChecked(false);
    ui->PTZ_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->mode = 0;
}

void UserPanel::on_SELECT_TOOL_clicked()
{
    ui->ZOOM_TOOL->setChecked(false);
    ui->PTZ_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->mode = 1;
}

void UserPanel::on_PTZ_TOOL_clicked()
{
    ui->ZOOM_TOOL->setChecked(false);
    ui->SELECT_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->mode = 2;
}

void UserPanel::on_FILE_PATH_EDIT_editingFinished()
{
    QString temp_location = ui->FILE_PATH_EDIT->text();
    if (QDir(temp_location).exists() || QDir().mkdir(temp_location)) save_location = temp_location;
    else QMessageBox::warning(this, "PROMPT", tr("cannot create directory"));
    ui->FILE_PATH_EDIT->setText(save_location);
}

void UserPanel::on_BINNING_CHECK_stateChanged(int arg1)
{
    int binning = arg1 ? 2 : 1;
    curr_cam->binning(false, &binning);
    curr_cam->frame_size(true, &w, &h);
    qInfo("frame w: %d, h: %d", w, h);
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
}

void UserPanel::transparent_transmission_file(int id)
{
    QFile f("lens" + QString::number(id));
    f.open(QIODevice::ReadOnly);
    QByteArray line;
    while (!(line = f.readLine(128)).isEmpty()) {
        QString send_str = line.simplified();
        send_str.replace(" ", "");
//        qDebug() << "line" << line;
        bool ok;
        QByteArray cmd(send_str.length() / 2, 0x00);
        for (int i = 0; i < send_str.length() / 2; i++) cmd[i] = send_str.mid(i * 2, 2).toInt(&ok, 16);

        QString str_s("sent    "), str_r("received");
        int write_size = cmd.length();

        for (char i = 0; i < write_size; i++) str_s += QString::asprintf(" %02X", i < 0 ? 0 : (uchar)cmd[i]);
        emit append_text(str_s);

        if (!serial_port[0]->isOpen()) continue;
        serial_port[0]->clear();
        serial_port[0]->write(cmd, write_size);
        while (serial_port[0]->waitForBytesWritten(10)) ;

        while (serial_port[0]->waitForReadyRead(100)) ;

        QByteArray read = serial_port[0]->readAll();
        for (char i = 0; i < read.length(); i++) str_r += QString::asprintf(" %02X", i < 0 ? 0 : (uchar)read[i]);
        emit append_text(str_r);

        QThread().msleep(5);
    }
    f.close();
}

void UserPanel::config_gatewidth(QString filename)
{
    for (int i = 0; i < 100; i++) gw_lut[i] = -1;
    QFile config_file(filename);
    config_file.open(QIODevice::ReadOnly);
    if (!config_file.isOpen()) QMessageBox::warning(this, "PROMPT", tr("cannot open config file"));
    QByteArray line;
    while (!(line = config_file.readLine(128)).isEmpty()) {
        line = line.simplified();
        // ori: user input, res: output to tcu
        int ori = line.left(line.indexOf(',')).toInt();
        int res = line.right(line.length() - line.indexOf(',') - 1).toInt();
        gw_lut[ori] = res;
    }
}

void UserPanel::send_ctrl_cmd(uchar dir)
{
    static uchar buffer_out[7] = {0};
    buffer_out[0] = 0xFF;
    buffer_out[1] = 0x01;
    buffer_out[2] = 0x00;
    buffer_out[3] = dir;
    buffer_out[4] = dir < 5 ? ptz_speed : 0x00;
    buffer_out[5] = dir > 5 ? ptz_speed : 0x00;
    int sum = 0;
    for (int i = 1; i < 6; i++) sum += buffer_out[i];
    buffer_out[6] = sum & 0xFF;

    ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 1, false);
}

void UserPanel::ptz_button_pressed(int id) {
//    qDebug("%dp", id);
    switch(id){
    case 0: send_ctrl_cmd(0x08); send_ctrl_cmd(0x04); break;
    case 1: send_ctrl_cmd(0x08);                      break;
    case 2: send_ctrl_cmd(0x08); send_ctrl_cmd(0x02); break;
    case 3: send_ctrl_cmd(0x04);                      break;
//    case 4: send_ctrl_cmd(0x00); break;
    case 5: send_ctrl_cmd(0x02);                      break;
    case 6: send_ctrl_cmd(0x10); send_ctrl_cmd(0x04); break;
    case 7: send_ctrl_cmd(0x10);                      break;
    case 8: send_ctrl_cmd(0x10); send_ctrl_cmd(0x02); break;
    default:                                          break;
    }
}

void UserPanel::ptz_button_released(int id) {
//    qDebug("%dr", id);
    if (id == 4) return;
    ptr_ptz->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
}

void UserPanel::on_PTZ_SPEED_SLIDER_valueChanged(int value)
{
    ptz_speed = value;
    ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
}

void UserPanel::on_PTZ_SPEED_EDIT_editingFinished()
{
    ptz_speed = ui->PTZ_SPEED_EDIT->text().toInt();
    if (ptz_speed > 64) ptz_speed = 64;
    if (ptz_speed < 1)  ptz_speed = 1;
    ui->PTZ_SPEED_SLIDER->setValue(ptz_speed);
    ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
}

void UserPanel::on_GET_ANGLE_BTN_clicked()
{
    uchar buffer_out[7] = {0};
    buffer_out[0] = 0xFF;
    buffer_out[1] = 0x01;
    buffer_out[2] = 0x00;
    buffer_out[3] = 0x51;
    buffer_out[4] = 0x00;
    buffer_out[5] = 0x00;
    buffer_out[6] = 0x52;
    QByteArray read = ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 7, true);
    angle_h = (((uchar)read[4] << 8) + (uchar)read[5]) / 100.0;
    ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", angle_h));

    QThread::msleep(30);

    buffer_out[3] = 0x53;
    buffer_out[6] = 0x54;
    read = ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 7, true);
    angle_v = (((uchar)read[4] << 8) + (uchar)read[5]) / 100.0;
    ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", angle_v > 40 ? angle_v - 360 : angle_v));
}

void UserPanel::set_ptz_angle()
{
    static int angle = 0, sum = 0;
    static uchar buffer_out[7] = {0};

    angle_h += 360 * (int)((-angle_h) / 360);
    if (angle_h < 0) angle_h += 360;
    angle = angle_h * 100;
    buffer_out[0] = 0xFF;
    buffer_out[1] = 0x01;
    buffer_out[2] = 0x00;
    buffer_out[3] = 0x4B;
    buffer_out[4] = (angle >> 8) & 0xFF;
    buffer_out[5] = angle & 0xFF;
    sum = 0;
    for (int i = 1; i < 6; i++) sum += buffer_out[i];
    buffer_out[6] = sum & 0xFF;
    ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 0, false);
    ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", angle_h));

    if (angle_v >  40) angle_v =  40;
    if (angle_v < -40) angle_v = -40;
    angle = angle_v * 100;
    buffer_out[3] = 0x4D;
    buffer_out[4] = (angle >> 8) & 0xFF;
    buffer_out[5] = angle & 0xFF;
    sum = 0;
    for (int i = 1; i < 6; i++) sum += buffer_out[i];
    buffer_out[6] = sum & 0xFF;
    ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 0, false);
    ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", angle_v));
}

void UserPanel::on_STOP_BTN_clicked()
{
    ptr_ptz->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
}

void UserPanel::point_ptz_to_target(QPoint target)
{
    static int display_width, display_height;
    // TODO config params for max zoom
//    static float tot_h = 20, tot_v = 14;// large FOV
    static float tot_h = 0.82, tot_v = 0.57;// small FOV
    display_width = ui->SOURCE_DISPLAY->width();
    display_height = ui->SOURCE_DISPLAY->height();
    angle_h += target.x() * tot_h / display_width - tot_h / 2;
    angle_v += target.y() * tot_v / display_height - tot_v / 2;

    set_ptz_angle();
}

void UserPanel::on_DUAL_LIGHT_BTN_clicked()
{
    QPluginLoader pluginLoader("plugins/ir_visible_light/plugin_ir_visible_light.dll");
    QObject *plugin = pluginLoader.instance();
    qDebug() << plugin;
    if (plugin) {
        pluginInterface = qobject_cast<PluginInterface *>(plugin);
//        pluginInterface = (PluginInterface *)plugin;
        qDebug() << pluginInterface;
        if (pluginInterface) pluginInterface->start();
    }
}

void UserPanel::on_RESET_3D_BTN_clicked()
{
    frame_a_3d ^= 1;
    frame_a_sum = 0;
    frame_b_sum = 0;
}

void UserPanel::display_fishnet_result(int result)
{
    switch (result) {
    case -1: ui->FISHNET_RESULT->setText("FISHNET<br>???"), ui->FISHNET_RESULT->setStyleSheet("color: #B0C4DE;");            break;
    case  0: ui->FISHNET_RESULT->setText("FISHNET<br>DOES NOT EXIST"), ui->FISHNET_RESULT->setStyleSheet("color: #B0C4DE;"); break;
    case  1: ui->FISHNET_RESULT->setText("FISHNET<br>EXISTS"), ui->FISHNET_RESULT->setStyleSheet("color: #CD5C5C;");         break;
    default: return;
    }
}

void UserPanel::on_FIRE_LASER_BTN_clicked()
{
    if (ui->FIRE_LASER_BTN->text() == "FIRE") {
//        serial_port[3]->write("MODE:STBY 0\r", 12);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
        ptr_laser->communicate_display(QByteArray("MODE:STBY 0\r"), 12, 0, true, false);
        qDebug() << QString("MODE:STBY 0\r");

//        serial_port[3]->write("ON\r", 3);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
        ptr_laser->communicate_display(QByteArray("ON\r"), 3, 0, true, false);
        qDebug() << QString("ON\r");

        ui->FIRE_LASER_BTN->setText("STOP");
    } else {
//        serial_port[3]->write("OFF\r", 4);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
        ptr_laser->communicate_display(QByteArray("OFF\r"), 4, 0, true, false);
        qDebug() << QString("OFF\r");

//        serial_port[3]->write("MODE:STBY 1\r", 12);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
        ptr_laser->communicate_display(QByteArray("MODE:STBY 0\r"), 12, 0, true, false);
        qDebug() << QString("MODE:STBY 1\r");

        ui->FIRE_LASER_BTN->setText("FIRE");
    }
}

void UserPanel::on_IMG_REGION_BTN_clicked()
{
    int max_w = 0, max_h = 0, new_w = 0, new_h = 0, new_x = 0, new_y = 0;
    int inc_w = 1, inc_h = 1, inc_x = 1, inc_y = 1;
    curr_cam->get_max_frame_size(&max_w, &max_h);
    curr_cam->frame_size(true, &new_w, &new_h, &inc_w, &inc_h);
    curr_cam->frame_offset(true, &new_x, &new_y, &inc_x, &inc_y);
    if (ui->SHAPE_INFO->pair != QPoint(0, 0)) {
        new_w = ui->SHAPE_INFO->pair.x();
        new_h = ui->SHAPE_INFO->pair.y();
        new_x += ui->START_COORD->pair.x();
        new_y += ui->START_COORD->pair.y();
    }

    new_w = (new_w + inc_w / 2) / inc_w; new_w *= inc_w;
    new_h = (new_h + inc_h / 2) / inc_h; new_h *= inc_h;
    new_x = (new_x + inc_x / 2) / inc_x; new_x *= inc_x;
    new_y = (new_y + inc_y / 2) / inc_y; new_y *= inc_y;

    QDialog image_region_dialog(this, Qt::FramelessWindowHint);
    QFormLayout form(&image_region_dialog);
    form.setRowWrapPolicy(QFormLayout::WrapAllRows);
    form.addRow(new QLabel("Region select:", &image_region_dialog));

    QLabel *width = new QLabel("Width (max " + QString::number(max_w) + "): ", &image_region_dialog);
    QSpinBox *width_spinbox = new QSpinBox(&image_region_dialog);
    width_spinbox->resize(80, 20);
    width_spinbox->setRange(0, max_w);
    width_spinbox->setSingleStep(inc_w);
    width_spinbox->setValue(new_w);
    form.addRow(width, width_spinbox);

    QLabel *height = new QLabel("Height (max " + QString::number(max_h) + "): ", &image_region_dialog);
    QSpinBox *height_spinbox = new QSpinBox(&image_region_dialog);
    height_spinbox->resize(80, 20);
    height_spinbox->setRange(0, max_h);
    height_spinbox->setSingleStep(inc_h);
    height_spinbox->setValue(new_h);
    form.addRow(height, height_spinbox);

    QLabel *offset_x = new QLabel("Offset X (max " + QString::number(max_w - new_w)  + "): ", &image_region_dialog);
    QSpinBox *x_spinbox = new QSpinBox(&image_region_dialog);
    x_spinbox->resize(80, 20);
    x_spinbox->setRange(0, max_w - new_w);
    x_spinbox->setSingleStep(inc_x);
    x_spinbox->setValue(new_x);
    form.addRow(offset_x, x_spinbox);
    connect(width_spinbox, &QSpinBox::editingFinished, x_spinbox,
        [max_w, &offset_x, width_spinbox, x_spinbox]() {
            x_spinbox->setMaximum(max_w - width_spinbox->value());
            offset_x->setText("Offset X (max " + QString::number(max_w - width_spinbox->value())  + "): ");
        });

    QLabel *offset_y = new QLabel("Offset Y (max " + QString::number(max_h - new_h) + "): ", &image_region_dialog);
    QSpinBox *y_spinbox = new QSpinBox(&image_region_dialog);
    y_spinbox->resize(80, 20);
    y_spinbox->setRange(0, max_h - new_h);
    y_spinbox->setSingleStep(inc_y);
    y_spinbox->setValue(new_y);
    form.addRow(offset_y, y_spinbox);
    connect(height_spinbox, &QSpinBox::editingFinished, y_spinbox,
        [max_h, &offset_y, height_spinbox, y_spinbox]() {
            y_spinbox->setMaximum(max_h - height_spinbox->value());
            offset_y->setText("Offset Y (max " + QString::number(max_h - height_spinbox->value()) + "): ");
        });

    connect(width_spinbox, static_cast<void (QSpinBox::*)(int i)>(&QSpinBox::valueChanged), this,
        [&new_w, &new_h, inc_w, inc_h, width_spinbox, height_spinbox](int val){
            new_w = (width_spinbox->value() + inc_w / 2) / inc_w;  new_w *= inc_w;
            new_h = (height_spinbox->value() + inc_h / 2) / inc_h; new_h *= inc_h;
        });

    connect(height_spinbox, static_cast<void (QSpinBox::*)(int i)>(&QSpinBox::valueChanged), this,
        [&new_w, &new_h, inc_w, inc_h, width_spinbox, height_spinbox](int val){
            new_w = (width_spinbox->value() + inc_w / 2) / inc_w;  new_w *= inc_w;
            new_h = (height_spinbox->value() + inc_h / 2) / inc_h; new_h *= inc_h;
        });

    connect(x_spinbox, static_cast<void (QSpinBox::*)(int i)>(&QSpinBox::valueChanged), this,
        [this, &new_x, &new_y, inc_x, inc_y, x_spinbox, y_spinbox](int val){
            new_x = (x_spinbox->value() + inc_x / 2) / inc_x; new_x *= inc_x;
            new_y = (y_spinbox->value() + inc_y / 2) / inc_y; new_y *= inc_y;
            curr_cam->frame_offset(false, &new_x, &new_y);
        });
    connect(y_spinbox, static_cast<void (QSpinBox::*)(int i)>(&QSpinBox::valueChanged), this,
        [this, &new_x, &new_y, inc_x, inc_y, x_spinbox, y_spinbox](int val){
            new_x = (x_spinbox->value() + inc_x / 2) / inc_x; new_x *= inc_x;
            new_y = (y_spinbox->value() + inc_y / 2) / inc_y; new_y *= inc_y;
            curr_cam->frame_offset(false, &new_x, &new_y);
        });

    QDialogButtonBox button_box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &image_region_dialog);
    form.addRow(&button_box);
    QObject::connect(&button_box, SIGNAL(accepted()), &image_region_dialog, SLOT(accept()));
    QObject::connect(&button_box, SIGNAL(rejected()), &image_region_dialog, SLOT(reject()));

    image_region_dialog.setModal(true);
    image_region_dialog.move(ui->IMG_GRAB_STATIC->mapToGlobal(QPoint()));
    button_box.button(QDialogButtonBox::Ok)->setFocus();
    button_box.button(QDialogButtonBox::Ok)->setDefault(true);

    // Process when OK button is clicked
    if (image_region_dialog.exec() == QDialog::Accepted) {
        new_w = width_spinbox->value();
        new_h = height_spinbox->value();
        new_x = x_spinbox->value();
        new_y = y_spinbox->value();
        bool was_playing = start_grabbing;
        if (was_playing) ui->STOP_GRABBING_BUTTON->click();
        curr_cam->frame_offset(false, &new_x, &new_y);
        curr_cam->frame_size(false, &new_w, &new_h);
        if (device_on) curr_cam->frame_size(true, &w, &h);
        QResizeEvent e(this->size(), this->size());
        resizeEvent(&e);
        if (was_playing) ui->START_GRABBING_BUTTON->click();
    }
}
