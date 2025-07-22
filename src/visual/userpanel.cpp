#include "userpanel.h"
#include "ui_user_panel.h"
#include "ui_preferences.h"

GrabThread::GrabThread(void *info, int idx)
{
    p_info = info;
    _display_idx = idx;
}

GrabThread::~GrabThread()
{
    quit();
    wait();
}

void GrabThread::display_idx(bool read, int &idx)
{
    if (read) idx = this->_display_idx;
    else      this->_display_idx = idx;
}

void GrabThread::run()
{
    // qDebug() << "image acquisition" << QThread::currentThread();
    ((UserPanel*)p_info)->grab_thread_process(&_display_idx);
}

TimedQueue::TimedQueue(double time) : QThread(), _quit(false), expiration_time(time), fps_status(NULL) {}
TimedQueue::~TimedQueue() {
    _quit = true;
    while (!q.empty()) q.pop();
}

inline void TimedQueue::set_display_label(StatusIcon *lbl) { fps_status = lbl; }

void TimedQueue::add_to_q() {
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    mtx.lock();
    q.push(t);
    mtx.unlock();
}

void TimedQueue::empty_q()
{
    mtx.lock();
    std::queue<struct timespec>().swap(q);
    mtx.unlock();
}

inline int TimedQueue::count() { return q.size(); }

void TimedQueue::stop()
{
    _quit = true;
    quit();
    wait();
}

// return difference in milliseconds
inline double diff_timespec(struct timespec &t1, struct timespec &t2) {
    return (t2.tv_sec - t1.tv_sec) * 1e3 + (t2.tv_nsec - t1.tv_nsec) / 1e6;
}

void TimedQueue::run()
{
    while (!_quit) {
#if 1
        QThread::msleep(5);
        static struct timespec t;
        timespec_get(&t, TIME_UTC);
        if (q.empty()) { continue; }
        mtx.lock();
        if (diff_timespec(q.front(), t) > expiration_time * 1e3) q.pop();
        static QElapsedTimer et;
        if (et.isValid() && et.elapsed() < 200) { mtx.unlock(); continue; }
        fps_status->set_text(QString::number(q.size() ? ((q.size() - 1) / diff_timespec(q.front(), q.back())) * 1000 : 0.f, 'f', 2));
        mtx.unlock();
        et.restart();
#endif
#if 0
        QThread::msleep(5);
        if (q.empty()) { continue; }
        while (q.size() > 10) q.pop();
        fps_status->set_text(QString::number(q.size() ? ((q.size() - 1) / diff_timespec(q.front(), q.back())) * 1000 : 0.f, 'f', 2));
#endif
    }
}

UserPanel::UserPanel(QWidget *parent) :
    QMainWindow(parent),
    mouse_pressed(false),
    ui(new Ui::UserPanel),
    status_bar(NULL),
    pref(NULL),
    scan_config(NULL),
    laser_settings(NULL),
#ifdef DISTANCE_3D_VIEW
    view_3d(NULL),
#endif //DISTANCE_3D_VIEW
    aliasing(NULL),
    fw_display{NULL},
    secondary_display(NULL),
    simple_ui(false),
    calc_avg_option(5),
    trigger_by_software(false),
    curr_cam(NULL),
    time_exposure_edit(95000),
#if SIMPLE_UI
    gain_analog_edit(23),
#else //SIMPLE_UI
    gain_analog_edit(0),
#endif //SIMPLE_UI
    frame_rate_edit(10),
//    ptr_tcu(NULL),
//    ptr_lens(NULL),
//    ptr_laser(NULL),
//    ptr_ptz(NULL),
    serial_port{NULL},
    serial_port_connected{false},
    tcp_port{NULL},
    use_tcp{false},
    share_serial_port(false),
    stepping(10),
    hz_unit(0),
    base_unit(0),
    laser_on(0b0001),
    zoom(0),
    focus(0),
    distance(0),
    rep_freq(30),
    laser_width(40),
    delay_dist(15),
    depth_of_view(6),
    mcp_max(255),
    aliasing_mode(false),
    aliasing_level(0),
    focus_direction(0),
    curr_laser_idx(-1),
    alt_display_option(0),
    q_fps_calc(5),
    device_type(0),
    device_on(false),
    start_grabbing(false),
    record_original{false},
    record_modified{false},
    save_original{false},
    save_modified{false},
    image_3d{false},
    image_rotate{0},
    is_color{false},
    pseudocolor{false},
    w{640, 480, 480},
    h{400, 270, 270},
    pixel_format{PixelType_Gvsp_Mono8},
    pixel_depth{8, 8, 8},
    display_thread_idx{-1, -1, -1},
    grab_image{false},
    h_grab_thread{NULL},
    grab_thread_state{false},
    video_thread_state(false),
    h_joystick_thread(NULL),
    auto_scan_mode(true),
    scan(false),
    scan_distance(200),
    c(3e8),
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
    lang(0),
    tp(40),
    ptz_grp(NULL),
    ptz_speed(16),
    angle_h(0),
    angle_v(0),
    alt_ctrl_grp(NULL)
{
    ui->setupUi(this);

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
    fw_display[0] = new FloatingWindow();
    fw_display[1] = new FloatingWindow();
    displays[0] = ui->SOURCE_DISPLAY;
    displays[1] = fw_display[0]->get_display_widget();
    displays[2] = fw_display[1]->get_display_widget();
    secondary_display = new FloatingWindow();

    trans.load("zh_cn.qm");

    ui->CENTRAL->setMouseTracking(true);
    ui->LEFT->setMouseTracking(true);
    ui->MID->setMouseTracking(true);
    ui->RIGHT->setMouseTracking(true);
    ui->TITLE->setMouseTracking(true);
    ui->STATUS->setMouseTracking(true);
    setMouseTracking(true);
    setCursor(cursor_curr_pointer);

    tp.start();
    q_fps_calc.set_display_label(ui->STATUS->result_cam_fps);
    q_fps_calc.start();

    dist_ns = c * 1e-9 / 2;

    QFont temp_f(consolas);
    temp_f.setPixelSize(11);

    // connect title bar to the main window (UserPanel*)
    ui->TITLE->setup(this);
    pref = ui->TITLE->preferences;
    pref->init();
    scan_config = ui->TITLE->scan_config;

//    SerialServer *s = new SerialServer();
//    s->show();

#if ENABLE_USER_DEFAULT
    FILE *f = fopen("user_default", "rb");
    if (f) {
        uchar use_zh;
        fseek(f, 4, SEEK_SET);
        fread(&use_zh, 1, 1, f);
        fclose(f);
        if (use_zh) switch_language();
    }
#endif
    // initialization
    // - default save path
    save_location += QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
//    qDebug() << QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section('/', 0, -1);
    TEMP_SAVE_LOCATION = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s/%s);", app_theme ? "light" : "dark", hide_left ? "right" : "left"));

    connect(this, SIGNAL(set_src_pixmap(QPixmap)), ui->SOURCE_DISPLAY, SLOT(setPixmap(QPixmap)), Qt::QueuedConnection);
    connect(ui->SOURCE_DISPLAY, &Display::add_roi, this,
            [this](cv::Point p1, cv::Point p2){
                if (!grab_image[0]) return;
                cv::Rect roi = cv::Rect(p1 * w[0] / ui->SOURCE_DISPLAY->width(), p2 * w[0] / ui->SOURCE_DISPLAY->width());
                list_roi.push_back(roi);
                user_mask[0](roi).setTo(255);
            });
    connect(ui->SOURCE_DISPLAY, &Display::clear_roi, this,
            [this](){
                if (!grab_image[0]) return;
                std::vector<cv::Rect>().swap(list_roi);
                user_mask[0] = 0;
            });
    connect(this, SIGNAL(set_hist_pixmap(QPixmap)), ui->HIST_DISPLAY, SLOT(setPixmap(QPixmap)), Qt::QueuedConnection);

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
    enhance_options->addItem(tr("AINDANE"));
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
    ui->MCP_SLIDER->setMaximum(mcp_max);
    ui->MCP_SLIDER->setSingleStep(1);
    ui->MCP_SLIDER->setPageStep(10);
    ui->MCP_SLIDER->setValue(0);
    connect(ui->MCP_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_mcp(int)));
    connect(ui->MCP_SLIDER, &QSlider::sliderMoved, this, [this](int){ if (ui->AUTO_MCP_CHK->isChecked()) ui->AUTO_MCP_CHK->click(); });

    ui->DELAY_SLIDER->setMinimum(0);
    ui->DELAY_SLIDER->setMaximum(pref->max_dist);
    ui->DELAY_SLIDER->setSingleStep(10);
    ui->DELAY_SLIDER->setPageStep(100);
    ui->DELAY_SLIDER->setValue(0);
    connect(ui->DELAY_SLIDER, SIGNAL(sliderMoved(int)), SLOT(change_delay(int)));

    ui->GW_SLIDER->setMinimum(0);
    ui->GW_SLIDER->setMaximum(pref->max_dov);
    ui->GW_SLIDER->setSingleStep(5);
    ui->GW_SLIDER->setPageStep(25);
    ui->GW_SLIDER->setValue(0);
    connect(ui->GW_SLIDER, SIGNAL(sliderMoved(int)), SLOT(change_gatewidth(int)));

    ui->FOCUS_SPEED_SLIDER->setMinimum(1);
    ui->FOCUS_SPEED_SLIDER->setMaximum(63);
    ui->FOCUS_SPEED_SLIDER->setSingleStep(1);
    ui->FOCUS_SPEED_SLIDER->setPageStep(4);
    ui->FOCUS_SPEED_SLIDER->setValue(31);
    connect(ui->FOCUS_SPEED_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_focus_speed(int)));
    ui->FOCUS_SPEED_EDIT->setText("31");

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
                pref->lower_3d_thresh = arg1.toFloat() / (1 << pixel_depth[0]);
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
//    ui->CURR_COORD->setup("current");
//    connect(ui->SOURCE_DISPLAY, SIGNAL(curr_pos(QPoint)), ui->CURR_COORD, SLOT(display_pos(QPoint)));
    ui->START_COORD->setup("start");
//    connect(ui->SOURCE_DISPLAY, SIGNAL(start_pos(QPoint)), ui->START_COORD, SLOT(display_pos(QPoint)));
    ui->SHAPE_INFO->setup("size");
//    connect(ui->SOURCE_DISPLAY, SIGNAL(shape_size(QPoint)), ui->SHAPE_INFO, SLOT(display_pos(QPoint)));
    ui->SHIFT_INFO->setup("shift");
    ui->SHIFT_INFO->hide();
    connect(ui->SOURCE_DISPLAY, &Display::updated_pos, this,
        [this](int idx, QPoint pos) {
            static QPoint converted_pos(0, 0);
            static Display *disp = ui->SOURCE_DISPLAY;
            converted_pos.setX(pos.x() * img_mem[0].cols / disp->width());
            converted_pos.setY(pos.y() * img_mem[0].rows / disp->height());
//            converted_pos /= 4;
//            converted_pos *= 4;
            switch (idx) {
//            case 0: ui->CURR_COORD->display_pos(converted_pos); break;
            case 1: ui->START_COORD->display_pos(converted_pos); break;
            case 2: ui->SHAPE_INFO->display_pos(converted_pos); break;
            case 3: point_ptz_to_target(pos); break;
            case 4:
                if (ui->SHAPE_INFO->pair.isNull()) ui->SHIFT_INFO->hide();
                else ui->SHIFT_INFO->show();
                ui->SHIFT_INFO->display_pos(converted_pos);
                break;
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
    pref->ui->REFRESH_AVAILABLE_PORTS_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    ui->SWITCH_TCU_UI_BTN->setIcon(QIcon(":/tools/" + QString(app_theme ? "light" : "dark") + "/switch"));

    ui->PTZ_SPEED_SLIDER->setMinimum(1);
    ui->PTZ_SPEED_SLIDER->setMaximum(63);
    ui->PTZ_SPEED_SLIDER->setSingleStep(1);
    ui->PTZ_SPEED_SLIDER->setValue(31);
    ui->PTZ_SPEED_EDIT->setText("31");

//    q_scan.push_back(-1);
//    setup_stepping(0);

    // TODO integrate main display capture/record here
    alt_ctrl_grp = new QButtonGroup(this);
    alt_ctrl_grp->addButton(ui->STOP_ALT1_BTN, 0);
    alt_ctrl_grp->addButton(ui->STOP_ALT2_BTN, 1);
    alt_ctrl_grp->addButton(ui->CAPTURE_ALT1_BTN, 2);
    alt_ctrl_grp->addButton(ui->CAPTURE_ALT2_BTN, 3);
    alt_ctrl_grp->addButton(ui->RECORD_ALT1_BTN, 4);
    alt_ctrl_grp->addButton(ui->RECORD_ALT2_BTN, 5);
    connect(alt_ctrl_grp, SIGNAL(buttonClicked(int)), this, SLOT(alt_display_control(int)));

    // set up display info (left bottom corner)
    QStringList alt_options_str;
    alt_options_str << "DATA" << "HIST" << "PTZ" << "ALT" << "ADDON";
//    alt_options_str << "ADDON" << "ALT" << "PTZ" << "HIST" << "DATA";
    ui->MISC_OPTION_1->addItems(alt_options_str);
    ui->MISC_OPTION_2->addItems(alt_options_str);
    ui->MISC_OPTION_3->addItems(alt_options_str);
    ui->MISC_OPTION_1->setCurrentIndex(0);
    ui->MISC_OPTION_2->setCurrentIndex(2);
    ui->MISC_OPTION_3->setCurrentIndex(3);
//    for (int i = 0; i < ui->MISC_OPTION_1->count(); i++) ui->MISC_OPTION_1->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
//    for (int i = 0; i < ui->MISC_OPTION_2->count(); i++) ui->MISC_OPTION_1->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
//    for (int i = 0; i < ui->MISC_OPTION_3->count(); i++) ui->MISC_OPTION_1->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);

    connect(ui->MISC_OPTION_1, SIGNAL(selected()), ui->MISC_RADIO_1, SLOT(click()));
    connect(ui->MISC_OPTION_2, SIGNAL(selected()), ui->MISC_RADIO_2, SLOT(click()));
    connect(ui->MISC_OPTION_3, SIGNAL(selected()), ui->MISC_RADIO_3, SLOT(click()));
    ui->MISC_OPTION_1->view()->setFont(temp_f);
    ui->MISC_OPTION_2->view()->setFont(temp_f);
    ui->MISC_OPTION_3->view()->setFont(temp_f);

    ui->DATA_EXCHANGE->document()->setMaximumBlockCount(200);
//    ui->COM_DATA_RADIO->click();
    ui->DATA_EXCHANGE->setFont(temp_f);
    ui->FILE_PATH_EDIT->setFont(consolas);

    display_grp = new QButtonGroup(this);
//    display_grp->addButton(ui->COM_DATA_RADIO);
//    display_grp->addButton(ui->PLUGIN_RADIO);
//    display_grp->addButton(ui->PTZ_RADIO);
    display_grp->addButton(ui->MISC_RADIO_1);
    display_grp->addButton(ui->MISC_RADIO_2);
    display_grp->addButton(ui->MISC_RADIO_3);
    display_grp->setExclusive(true);
    ui->MISC_RADIO_1->setChecked(true);
    ui->MISC_DISPLAY->setCurrentIndex(1);
    // TODO change continuous/trigger exclusive

    // connect status bar to the main window
    status_bar = ui->STATUS;

    init_control_port();

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

    // update enumerated device-list
    connect(this, SIGNAL(update_device_list(int, QStringList)), pref, SLOT(update_device_list(int, QStringList)));
    connect(this, SIGNAL(device_connection_status_changed(int, QStringList)), pref, SLOT(update_device_list(int, QStringList)));

    // process post-video stuff
    connect(this, &UserPanel::video_stopped, this,
            [this](){
                vid_out_proc.release();
                if (!temp_output_filename.isEmpty() && !output_filename.isEmpty()) {
                    std::thread t(UserPanel::move_to_dest, temp_output_filename.trimmed(), output_filename.trimmed());
                    t.detach();
                }
                temp_output_filename = output_filename = "";
            });

    laser_settings = new LaserControl(nullptr, p_lens);
    connect(laser_settings, SIGNAL(com_write(int, QByteArray)), this, SLOT(com_write_data(int, QByteArray)));

    // setup serial port with tcp socket
//    ptr_tcu   = new TCUThread   (ui->TCU_COM,   ui->TCU_COM_EDIT,   0, ui->STATUS->tcu_status,   scan_config, NULL);
//    ptr_inc   = new InclinThread(ui->RANGE_COM, ui->RANGE_COM_EDIT, 4, nullptr,                               NULL);
//    ptr_lens  = new LensThread  (ui->LENS_COM,  ui->LENS_COM_EDIT,  1, ui->STATUS->lens_status,               NULL);
//    ptr_laser = new LaserThread (ui->LASER_COM, ui->LASER_COM_EDIT, 2, ui->STATUS->laser_status,              NULL);
//    ptr_ptz   = new PTZThread   (ui->PTZ_COM,   ui->PTZ_COM_EDIT,   3, ui->STATUS->ptz_status,                NULL);
//    connect(ptr_tcu,   &ControlPortThread::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
//    connect(ptr_inc,   &ControlPortThread::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
//    connect(ptr_lens,  &ControlPortThread::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
//    connect(ptr_laser, &ControlPortThread::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
//    connect(ptr_ptz,   &ControlPortThread::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);
//    ptr_tcu->start();
//    ptr_inc->start();
//    ptr_lens->start();
//    ptr_laser->start();
//    ptr_ptz->start();
    qDebug() << "main" << QThread::currentThread();

//    connect(pref, &Preferences::set_auto_rep_freq, ptr_tcu, [this](bool arg1){ ptr_tcu->auto_rep_freq = arg1;});
    connect(pref, &Preferences::set_auto_rep_freq, p_tcu, [this](bool arg1){ emit send_uint_tcu_msg(TCU::AUTO_PRF, arg1); });
    connect(pref, &Preferences::set_ab_lock, p_tcu, [this](bool arg1){ emit send_uint_tcu_msg(TCU::AB_LOCK, arg1); });

#ifdef DISTANCE_3D_VIEW
    view_3d = new Distance3DView(nullptr, this, &list_roi);
#endif //DISTANCE_3D_VIEW
    aliasing = new Aliasing();
    connect(aliasing, &Aliasing::delay_set_selected, this, &UserPanel::set_distance_set);

    preset = new PresetPanel();
    connect(preset, SIGNAL(preset_updated(int)), this, SLOT(generate_preset_data(int)), Qt::QueuedConnection);
    connect(this, SIGNAL(save_to_preset(int, nlohmann::json)), preset, SLOT(save_preset(int, nlohmann::json)), Qt::QueuedConnection);
    connect(preset, &PresetPanel::preset_selected, this, &UserPanel::apply_preset, Qt::QueuedConnection);

    uchar com[5] = {0};
#ifdef ENABLE_USER_DEFAULT
    f = fopen("user_default", "rb");
    if (f) {
        fseek(f, 5, SEEK_SET);
        fread(com, 1, 5, f);
        fclose(f);
    }
#endif //ENABLE_USER_DEFAULT
    for (int i = 0; i < 5; i++) com_edit[i]->setText(QString::number(com[i])), com_edit[i]->emit returnPressed();

    for (int i = 0; i < 5; i++) serial_port[i] = new QSerialPort(this)/*, setup_serial_port(serial_port + i, i, com_edit[i]->text(), 9600)*/;
    for (int i = 0; i < 5; i++) tcp_port[i] = new QTcpSocket(this);
    if (serial_port[0]->isOpen() && serial_port[3]->isOpen()) on_LASER_BTN_clicked();

    // connect ptz targeting slots (moved to signal updated_pos)
//    connect(ui->SOURCE_DISPLAY, SIGNAL(ptz_target(QPoint)), this, SLOT(point_ptz_to_target(QPoint)));

    // init ui
    ui->LASER_WIDTH_EDIT_P->hide();
    ui->DELAY_A_EDIT_P->hide();
    ui->DELAY_B_EDIT_P->hide();
    ui->GATE_WIDTH_A_EDIT_P->hide();
    ui->GATE_WIDTH_B_EDIT_P->hide();
    ui->DELAY_DIFF_GRP->hide();
    ui->GATE_WIDTH_DIFF_GRP->hide();

    // - write default params
    data_exchange(false);
    enable_controls(false);

    // TODO: move to tcu class
    // initialize gatewidth lut
//    ptr_tcu->use_gw_lut = false;
//    for (int i = 0; i < 100; i++) ptr_tcu->gw_lut[i] = i;
}

UserPanel::~UserPanel()
{
    if (q_fps_calc.isRunning()) q_fps_calc.stop();

    for (int i = 0; i < 3; i++) {
        grab_image[i] = false;
        if (h_grab_thread[i]) {
            h_grab_thread[i]->quit();
            h_grab_thread[i]->wait();
            delete h_grab_thread[i];
            h_grab_thread[i] = NULL;
        }
    }

    tp.stop();
    delete laser_settings;
#ifdef DISTANCE_3D_VIEW
    delete view_3d;
#endif //DISTANCE_3D_VIEW
    delete aliasing;
    delete preset;

    for (FloatingWindow* &w: fw_display) delete w;

//    ptr_tcu->quit_thread();
//    ptr_inc->quit_thread();
//    ptr_lens->quit_thread();
//    ptr_laser->quit_thread();
//    ptr_ptz->quit_thread();

    th_tcu->quit();
    th_lens->quit();
    th_laser->quit();
    th_ptz->quit();
    th_rf->quit();
    
    // Wait for I/O threads to finish - 100ms is sufficient for serial/TCP operations
    th_tcu->wait(100);
    th_lens->wait(100);
    th_laser->wait(100);
    th_ptz->wait(100);
    th_rf->wait(100);
    
    th_tcu->deleteLater();
    th_lens->deleteLater();
    th_laser->deleteLater();
    th_ptz->deleteLater();
    th_rf->deleteLater();
    p_tcu->deleteLater();
    p_lens->deleteLater();
    p_laser->deleteLater();
    p_ptz->deleteLater();
    p_rf->deleteLater();

    th_usbcan->quit();
    th_usbcan->wait(100);
    th_usbcan->deleteLater();
    p_usbcan->deleteLater();

    delete ui;
}

void UserPanel::init()
{
    // - set startup focus
    ui->ENUM_BUTTON->click();
    (ui->START_BUTTON->isEnabled() ? ui->START_BUTTON : ui->ENUM_BUTTON)->setFocus();

#if SIMPLE_UI
    ui->START_BUTTON->click();
    ui->HIDE_BTN->click();
    ui->START_GRABBING_BUTTON->click();
    switch_ui();
    QKeyEvent connect_to_TCP(QKeyEvent::KeyPress, Qt::Key_Q, Qt::AltModifier);
    QApplication::sendEvent(this, &connect_to_TCP);
    delay_dist = 1000;
    depth_of_view = 75;
    laser_width = 500;
    update_delay();
    update_gate_width();
    update_laser_width();
    pref->ui->AUTO_MCP_CHK->click();
    pref->ui->UNIT_LIST->setCurrentIndex(2);
    pref->ui->MAX_DIST_EDT->setText("1800.00");
    pref->ui->MAX_DOV_EDT->setText("150.00");
    pref->data_exchange(false);
    pref->ui->MAX_DIST_EDT->emit editingFinished();
    pref->ui->MAX_DOV_EDT->emit editingFinished();
    ui->TITLE->process_maximize();

    ui->FRAME_AVG_CHECK->setText("Filter");
#else //SIMPLE_UI
    ui->AUTO_MCP_CHK->hide();
    ui->SIMPLE_LASER_CHK->hide();
    ui->GW_SLIDER->hide();

    ui->FOCUS_SPEED_LABEL->hide();

    ui->RADIUS_INC_BTN->hide();
    ui->RADIUS_DEC_BTN->hide();
    ui->LASER_RADIUS_LABEL->hide();

    ui->FRAME_AVG_CHECK->setText("Avg");
#endif //SIMPLE_UI

    ui->BRIGHTNESS_LABEL->hide();
    ui->BRIGHTNESS_SLIDER->hide();
    ui->PSEUDOCOLOR_CHK->show();

#ifdef LVTONG
    pref->ui->UNDERWATER_CHK->click();

    ui->DISTANCE->hide();
    ui->DIST_BTN->hide();
    ui->CURRENT_EDIT->setText("18");
    ui->SIMPLE_LASER_CHK->setEnabled(false);

    pref->ui->SHARE_CHK->click();
    pref->ui->IMG_FORMAT_LST->setCurrentIndex(1);

    connect(this, SIGNAL(update_fishnet_result(int)), SLOT(display_fishnet_result(int)));

    ui->START_BUTTON->click();
    ui->HIDE_BTN->click();
    ui->START_GRABBING_BUTTON->click();
    switch_ui();

    delay_dist = 9;
    depth_of_view = 2.25;
    update_delay();
    update_gate_width();
    pref->ui->UNIT_LIST->setCurrentIndex(2);
    pref->ui->AUTO_MCP_CHK->click();
    pref->data_exchange(false);
#else //LVTONG
    ui->FIRE_LASER_BTN->hide();
    ui->FISHNET_RESULT->hide();
#endif //LVTONG

    ui->LOGO->hide();
//    ui->ANALYSIS_RADIO->hide();
//    ui->PLUGIN_DISPLAY_1->hide();

    pref->ui->TCU_LIST->setCurrentIndex(1);
#ifdef USING_CAMERALINK
    pref->ui->CAMERALINK_CHK->click();
#else //USING_CAMERALINK
    pref->ui->CAMERALINK_CHK->hide();
#endif //USING_CAMERALINK
    ui->SENSOR_TAPS_BTN->hide();

    switch_ui();
}

void UserPanel::data_exchange(bool read){
    if (read) {
//        device_idx = ui->DEVICE_SELECTION->currentIndex();
        calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 4 + 4;

        trigger_by_software = ui->SOFTWARE_CHECK->isChecked();
        image_3d[0] = ui->IMG_3D_CHECK->isChecked();

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
//        ptr_tcu->laser_width = ui->LASER_WIDTH_EDIT_U->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt() + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->gate_width_a = ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt() + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->gate_width_b = ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->delay_a = ui->DELAY_A_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->delay_b = ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->gate_width_n = ui->GATE_WIDTH_N_EDIT_N->text().toFloat();
//        ptr_tcu->delay_n = ui->DELAY_N_EDIT_N->text().toFloat();

        switch (base_unit) {
        // ns
        case 0: stepping = ui->STEPPING_EDIT->text().toFloat(); break;
        // μs
        case 1: stepping = ui->STEPPING_EDIT->text().toFloat() * 1000; break;
        // m
        case 2: stepping = ui->STEPPING_EDIT->text().toFloat() / dist_ns; break;
        default: break;
        }
        // use toFloat() since float representation in line edit
//        ptr_tcu->ccd_freq = ui->CCD_FREQ_EDIT->text().toFloat();
//        ptr_tcu->duty = ui->DUTY_EDIT->text().toFloat() * 1000;
//        ptr_tcu->mcp = ui->MCP_EDIT->text().toInt();

//        zoom = ui->ZOOM_EDIT->text().toInt();
//        focus = ui->FOCUS_EDIT->text().toInt();

        delay_dist = std::round(p_tcu->get(TCU::DELAY_A) * dist_ns);
        depth_of_view = std::round(p_tcu->get(TCU::GATE_WIDTH_A) * dist_ns);
    }
    else {
//        ui->DEVICE_SELECTION->setCurrentIndex(device_idx);
        ui->FRAME_AVG_OPTIONS->setCurrentIndex(calc_avg_option / 4 - 1);

        ui->SOFTWARE_CHECK->setChecked(trigger_by_software);
        ui->IMG_3D_CHECK->setChecked(image_3d[0]);

        ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
        ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
        ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));
        ui->FILE_PATH_EDIT->setText(save_location);

//        ptr_tcu->gate_width_a = depth_of_view / dist_ns;
//        ptr_tcu->gate_width_b = ptr_tcu->gate_width_a + ptr_tcu->gate_width_n;
//        ptr_tcu->delay_a = delay_dist / dist_ns;
//        ptr_tcu->delay_b = ptr_tcu->delay_a + ptr_tcu->delay_n;

        setup_hz(hz_unit);
        uint us, ns, ps;
        split_value_by_unit(p_tcu->get(TCU::LASER_WIDTH), us, ns, ps);
        ui->LASER_WIDTH_EDIT_U->setText(QString::asprintf(  "%d", us));
        ui->LASER_WIDTH_EDIT_N->setText(QString::asprintf("%03d", ns));
        ui->LASER_WIDTH_EDIT_P->setText(QString::asprintf("%03d", ps));
        split_value_by_unit(p_tcu->get(TCU::GATE_WIDTH_A), us, ns, ps, 0);
        ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf(  "%d", us));
        ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%03d", ns));
        ui->GATE_WIDTH_A_EDIT_P->setText(QString::asprintf("%03d", ps));
        split_value_by_unit(p_tcu->get(TCU::DELAY_A), us, ns, ps, 1);
        ui->DELAY_A_EDIT_U->setText(QString::asprintf(  "%d", us));
        ui->DELAY_A_EDIT_N->setText(QString::asprintf("%03d", ns));
        ui->DELAY_A_EDIT_P->setText(QString::asprintf("%03d", ps));
        split_value_by_unit(p_tcu->get(TCU::GATE_WIDTH_B), us, ns, ps, 0);
        ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf(  "%d", us));
        ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%03d", ns));
        ui->GATE_WIDTH_B_EDIT_P->setText(QString::asprintf("%03d", ps));
        split_value_by_unit(p_tcu->get(TCU::DELAY_B), us, ns, ps, 1);
        ui->DELAY_B_EDIT_U->setText(QString::asprintf(  "%d", us));
        ui->DELAY_B_EDIT_N->setText(QString::asprintf("%03d", ns));
        ui->DELAY_B_EDIT_P->setText(QString::asprintf("%03d", ps));
        ui->GATE_WIDTH_N_EDIT_N->setText(QString::asprintf("%d", int(std::round(p_tcu->get(TCU::GATE_WIDTH_N)))));
        ui->DELAY_N_EDIT_N->setText(QString::asprintf("%d", int(std::round(p_tcu->get(TCU::DELAY_N)))));
//        ui->GATE_WIDTH_N->setText(QString::asprintf("%d", int(std::round(p_tcu->get(TCU::GATE_WIDTH_N)))));
        ui->DELAY_SLIDER->setValue(delay_dist);
        ui->GW_SLIDER->setValue(depth_of_view);
        ui->MCP_SLIDER->setValue(std::round(p_tcu->get(TCU::MCP)));
        ui->MCP_EDIT->setText(QString::number((int)std::round(p_tcu->get(TCU::MCP))));

        setup_stepping(base_unit);

        ui->ZOOM_EDIT->setText(QString::asprintf("%d", p_lens->get(Lens::ZOOM_POS)));
        ui->FOCUS_EDIT->setText(QString::asprintf("%d", p_lens->get(Lens::FOCUS_POS)));

        ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", p_ptz->get(PTZ::ANGLE_H)));
        ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", p_ptz->get(PTZ::ANGLE_V)));
    }
}

int UserPanel::grab_thread_process(int *idx) {
    int thread_idx = *idx;
    grab_thread_state[thread_idx] = true;
    Display *disp;
    int _w = w[thread_idx], _h = h[thread_idx], _pixel_depth = pixel_depth[thread_idx];
    bool updated;
    cv::Mat img_display, prev_img, prev_3d;
    cv::Mat seq[8], seq_sum, frame_a_sum, frame_b_sum;
    int seq_idx = 0;
    uint hist[256];
    cv::Mat hist_mat, dist_mat;
//    ProgSettings *settings = ui->TITLE->prog_settings;
    QImage stream;
    cv::Mat sobel;
    int ww, hh, scan_img_count = -1;
    float weight = _h / 1024.0; // font scale & thickness
    prev_3d = cv::Mat(_h, _w, CV_8UC3);
    prev_img = cv::Mat(_h, _w, CV_MAKETYPE(_pixel_depth == 8 ? CV_8U : CV_16U, is_color[thread_idx] ? 3 : 1));
    user_mask[thread_idx] = cv::Mat::zeros(_h, _w, CV_8UC1);
    static int packets_lost;
    packets_lost = 0;
//    double *range = (double*)calloc(w * h, sizeof(double));
#ifdef LVTONG
    pref->ui->MODEL_LIST->setEnabled(false);
    cv::Mat fishnet_res;
    QString model_name;
    switch (pref->model_idx) {
    case 0: model_name = "models/fishnet_lvtong_legacy.onnx"; break;
    case 1: model_name = "models/fishnet_pix2pix_maxpool.onnx"; break;
    case 2: model_name = "models/fishnet_maskguided.onnx"; break;
    case 3: model_name = "models/fishnet_semantic_static_sim.onnx"; break;
    default: break;
    }

    cv::dnn::Net net = cv::dnn::readNet(model_name.toLatin1().constData());
#endif
    while (grab_image[thread_idx]) {
        disp = displays[*idx];

        {
            QMutexLocker locker(&image_mutex[thread_idx]);
            
            // Limit queue size safely
            while (q_img[thread_idx].size() > 5) {
                q_img[thread_idx].pop();
                if (device_type == 1) q_frame_info.pop();
            }

            // Safe check and access
            if (q_img[thread_idx].empty()) {
#ifdef LVTONG
                locker.unlock();
                QThread::msleep(10);
                continue;
#else
//                if (device_type == -2) continue;
                img_mem[thread_idx] = prev_img.clone();
                updated = false;
#endif
            }
            else {
                img_mem[thread_idx] = q_img[thread_idx].front();
                q_img[thread_idx].pop();
                int packets_lost_frame = 0;
                if (device_type == 1 && !q_frame_info.empty()) {
                    packets_lost_frame = q_frame_info.front();
                    q_frame_info.pop();
                }
                packets_lost += packets_lost_frame;
                if (packets_lost_frame) ui->STATUS->packet_lost->set_text("packets lost: " + QString::number(packets_lost));
//                if (packet_lost > 10) {
//                    static bool msgbox = false;
//                    if (!msgbox) {
//                        msgbox = true;
//                        msgbox = !QMessageBox::warning(nullptr, "PROMPT", "packet loss detected: " + QString::number(packet_lost));
//                    }
//                }
                updated = true;
                if (device_type == -1) q_img[thread_idx].push(img_mem[thread_idx].clone());
            }
//            cv::imwrite("../a.bmp", img_mem[thread_idx]);
            q_fps_calc.add_to_q();
        }
//        sr.upsample(img_mem[thread_idx].clone(), img_mem[thread_idx]);
//        cv::resize(img_mem[thread_idx].clone(), img_mem[thread_idx], img_mem[thread_idx].size() * 2);

        {
            QMutexLocker processing_locker(&image_mutex[thread_idx]);
//        qDebug () << QDateTime::currentDateTime().toString() << updated << img_mem.data;

        if (updated) {
            switch (image_rotate[0]) {
            case 0:
                if (_w != w[thread_idx]) {
                    prev_img.release(), prev_3d.release();
                    seq_sum.release(), frame_a_sum.release(), frame_b_sum.release();
                    for (auto& m: seq) m.release();
                    seq_idx = 0;
                }
                _w = w[thread_idx];
                _h = h[thread_idx];
                break;
            case 2:
                cv::flip(img_mem[thread_idx], img_mem[thread_idx], -1);
                if (_w != w[thread_idx]) {
                    prev_img.release(), prev_3d.release();
                    seq_sum.release(), frame_a_sum.release(), frame_b_sum.release();
                    for (auto& m: seq) m.release();
                    seq_idx = 0;
                }
                _w = w[thread_idx];
                _h = h[thread_idx];
                break;
            case 1:
                cv::flip(img_mem[thread_idx], img_mem[thread_idx], -1);
            case 3:
                cv::flip(img_mem[thread_idx], img_mem[thread_idx], 0);
                cv::transpose(img_mem[thread_idx], img_mem[thread_idx]);
                if (_w != h[thread_idx]) {
                    prev_img.release(), prev_3d.release();
                    seq_sum.release(), frame_a_sum.release(), frame_b_sum.release();
                    for (auto& m: seq) m.release();
                    seq_idx = 0;  }
                _w = h[thread_idx];
                _h = w[thread_idx];
                break;
            default:
                break;
            }

            // calc histogram (grayscale)
            memset(hist, 0, 256 * sizeof(uint));
            if (!is_color[thread_idx]) for (int i = 0; i < _h; i++) for (int j = 0; j < _w; j++) hist[(img_mem[thread_idx].data + i * img_mem[thread_idx].cols)[j]]++;

            // if the image needs flipping
            if (pref->symmetry) cv::flip(img_mem[thread_idx], img_mem[thread_idx], pref->symmetry - 2);

            // mcp self-adaptive
            if (pref->auto_mcp && !ui->MCP_SLIDER->hasFocus()) {
                int thresh_num = img_mem[thread_idx].total() / 200, thresh = (1 << pixel_depth[thread_idx]) - 1;
                while (thresh && thresh_num > 0) thresh_num -= hist[thresh--];
//                if (thresh > (1 << pixel_depth[thread_idx]) * 0.94) emit update_mcp_in_thread(ptr_tcu->mcp - sqrt(thresh - (1 << pixel_depth[thread_idx]) * 0.94));
                if (thresh > (1 << pixel_depth[thread_idx]) * 0.94) emit update_mcp_in_thread(p_tcu->get(TCU::MCP) - sqrt(thresh - (1 << pixel_depth[thread_idx]) * 0.94));
//                qDebug() << "high" << thresh;
//                thresh_num = img_mem.total() / 100, thresh = 255;
//                while (thresh && thresh_num > 0) thresh_num -= hist[thresh--];
//                if (thresh < (1 << pixel_depth[thread_idx]) * 0.39) emit update_mcp_in_thread(ptr_tcu->mcp + sqrt((1 << pixel_depth[thread_idx]) * 0.39 - thresh));
                if (thresh < (1 << pixel_depth[thread_idx]) * 0.39) emit update_mcp_in_thread(p_tcu->get(TCU::MCP) + sqrt((1 << pixel_depth[thread_idx]) * 0.39 - thresh));
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
        double is_net = 0;
        static double min, max;
        if (pref->fishnet_recog) {
            switch (pref->model_idx) {
            case 0:
            {
                cv::cvtColor(img_mem[thread_idx], fishnet_res, cv::COLOR_GRAY2RGB);
                fishnet_res.convertTo(fishnet_res, CV_32FC3, 1.0 / 255);
                cv::resize(fishnet_res, fishnet_res, cv::Size(224, 224));

                cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(224, 224));
                net.setInput(blob);
                cv::Mat prob = net.forward("195");
                //            std::cout << cv::format(prob, cv::Formatter::FMT_C) << std::endl;

                cv::minMaxLoc(prob, &min, &max);

                prob -= max;
                is_net = exp(prob.at<float>(1)) / (exp(prob.at<float>(0)) + exp(prob.at<float>(1)));
//                is_net = exp(prob.at<float>(1)) / exp(prob.at<float>(0) + prob.at<float>(1));

                emit update_fishnet_result(is_net > pref->fishnet_thresh);

                break;
            }
            case 1:
            case 2:
            {
                img_mem[thread_idx].convertTo(fishnet_res, CV_32FC1, 1.0 / 255);
                cv::resize(fishnet_res, fishnet_res, cv::Size(256, 256));

                cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(256, 256));
                net.setInput(blob);
                cv::Mat prob = net.forward();
//                std::cout << cv::format(prob, cv::Formatter::FMT_C) << std::endl;

                cv::minMaxLoc(prob, &min, &max);

//                prob -= max;
//                is_net = exp(prob.at<float>(1)) / (exp(prob.at<float>(0)) + exp(prob.at<float>(1)));

                emit update_fishnet_result(prob.at<float>(1) > prob.at<float>(0));

                break;
            }
            case 3:
            {
                img_mem[thread_idx].convertTo(fishnet_res, CV_32FC1, 1.0 / 255);
                cv::resize(fishnet_res, fishnet_res, cv::Size(224, 224));

                cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(224, 224), cv::Scalar(0.5));
                net.setInput(blob);
                cv::Mat prob = net.forward();
                //                std::cout << cv::format(prob, cv::Formatter::FMT_C) << std::endl;

                cv::minMaxLoc(prob, &min, &max);

                //                prob -= max;
                //                is_net = exp(prob.at<float>(1)) / (exp(prob.at<float>(0)) + exp(prob.at<float>(1)));

                emit update_fishnet_result(prob.at<float>(1) > prob.at<float>(0));

                break;
            }
            default: break;
            }

        }
        else emit update_fishnet_result(-1);
#endif

        // process frame average
        if (seq_sum.empty()) seq_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color[thread_idx] ? 3 : 1));
        if (frame_a_sum.empty()) frame_a_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color[thread_idx] ? 3 : 1));
        if (frame_b_sum.empty()) frame_b_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color[thread_idx] ? 3 : 1));
        if (ui->FRAME_AVG_CHECK->isChecked()) {
            if (updated) {
//                calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 4 + 4;
                if (seq[7].empty()) for (auto& m: seq) m = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color[thread_idx] ? 3 : 1));

                seq_sum -= seq[(seq_idx + 4) & 7];
//                seq_sum -= seq[seq_idx];
                static bool frame_a = true;
                if (frame_a) frame_a_sum -= seq[seq_idx];
                else         frame_b_sum -= seq[seq_idx];
                img_mem[thread_idx].convertTo(seq[seq_idx], CV_MAKETYPE(CV_16U, is_color[thread_idx] ? 3 : 1));
                seq_sum += seq[seq_idx];
                if (frame_a) frame_a_sum += seq[seq_idx];
                else         frame_b_sum += seq[seq_idx];
//                for(int i = 0; i < calc_avg_option; i++) seq_sum += seq[i];

//                seq_idx = (seq_idx + 1) % calc_avg_option;
                seq_idx = (seq_idx + 1) & 7;
                frame_a ^= 1;
            }
//            seq_sum.convertTo(modified_result, CV_8U, 1. / (calc_avg_option * (1 << (pixel_depth - 8))));
            seq_sum.convertTo(modified_result[thread_idx], CV_MAKETYPE(CV_8U, is_color[thread_idx] ? 3 : 1), 1. / (4 * (1 << (_pixel_depth - 8))));
        }
        else {
            if (!seq_sum.empty()) {
                seq_sum.release();
                for (auto& m: seq) m.release();
                frame_a_sum.release();
                frame_b_sum.release();
                seq_idx = 0;
            }
            img_mem[thread_idx].convertTo(modified_result[thread_idx], CV_MAKETYPE(CV_8U, is_color[thread_idx] ? 3 : 1), 1. / (1 << (_pixel_depth - 8)));
        }

        if (pref->split) ImageProc::split_img(modified_result[thread_idx], modified_result[thread_idx]);

        // process 3d image construction from ABN frames
        if (ui->IMG_3D_CHECK->isChecked() && thread_idx == 0 && !is_color[thread_idx]) {
            ww = _w + 104;
            hh = _h;
            double dist_min = 0, dist_max = 0;
//            modified_result = frame_a_3d ? prev_3d : ImageProc::gated3D(prev_img, img_mem, delay_dist / dist_ns, depth_of_view / dist_ns, range_threshold);
//            if (prev_3d.empty()) cv::cvtColor(img_mem, prev_3d, cv::COLOR_GRAY2RGB);
            if (updated) {
                // TODO try to reduce if statements
                if (ui->FRAME_AVG_CHECK->isChecked()) {
                    static cv::Mat frame_a_avg, frame_b_avg;
                    frame_a_sum.convertTo(frame_a_avg, _pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
                    frame_b_sum.convertTo(frame_b_avg, _pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
//                    if (is_color[thread_idx]) {
//                        cv::cvtColor(frame_a_avg.clone(), frame_a_avg, cv::COLOR_BGR2GRAY);
//                        cv::cvtColor(frame_b_avg.clone(), frame_b_avg, cv::COLOR_BGR2GRAY);
//                    }
//                    ImageProc::gated3D_v2(frame_a_3d ? frame_b_avg : frame_a_avg, frame_a_3d ? frame_a_avg : frame_b_avg, modified_result[thread_idx],
//                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_DELAY_EDT->text().toFloat() : (delay_dist - ptr_tcu->delay_offset * dist_ns) / dist_ns,
//                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_GW_EDT->text().toFloat() : (depth_of_view - ptr_tcu->gate_width_offset * dist_ns) / dist_ns,
//                                          pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d, &dist_mat, &dist_min, &dist_max);
#ifdef LVTONG
                    ImageProc::gated3D_v2(frame_a_3d ? frame_b_avg : frame_a_avg, frame_a_3d ? frame_a_avg : frame_b_avg, modified_result[thread_idx],
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_DELAY_EDT->text().toFloat() : (delay_dist - p_tcu->get(TCU::OFFSET_DELAY) * dist_ns) / dist_ns,
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_GW_EDT->text().toFloat() : (depth_of_view - p_tcu->get(TCU::OFFSET_GW) * dist_ns) / dist_ns,
                                          pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d);
#else //LVTONG
                    ImageProc::gated3D_v2(frame_a_3d ? frame_b_avg : frame_a_avg, frame_a_3d ? frame_a_avg : frame_b_avg, modified_result[thread_idx],
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_DELAY_EDT->text().toFloat() : (delay_dist - p_tcu->get(TCU::OFFSET_DELAY) * dist_ns) / dist_ns,
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_GW_EDT->text().toFloat() : (depth_of_view - p_tcu->get(TCU::OFFSET_GW) * dist_ns) / dist_ns,
                                          pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif //LVTONG
                }
                else {
//                    ImageProc::gated3D_v2(frame_a_3d ? prev_img : img_mem[thread_idx], frame_a_3d ? img_mem[thread_idx] : prev_img, modified_result[thread_idx],
//                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_DELAY_EDT->text().toFloat() : (delay_dist - ptr_tcu->delay_offset * dist_ns) / dist_ns,
//                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_GW_EDT->text().toFloat() : (depth_of_view - ptr_tcu->gate_width_offset * dist_ns) / dist_ns,
//                                          pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d, &dist_mat, &dist_min, &dist_max);
#ifdef LVTONG
                    ImageProc::gated3D_v2(frame_a_3d ? prev_img : img_mem[thread_idx], frame_a_3d ? img_mem[thread_idx] : prev_img, modified_result[thread_idx],
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_DELAY_EDT->text().toFloat() : (delay_dist - p_tcu->get(TCU::OFFSET_DELAY) * dist_ns) / dist_ns,
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_GW_EDT->text().toFloat() : (depth_of_view - p_tcu->get(TCU::OFFSET_GW) * dist_ns) / dist_ns,
                                          pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d);
#else //LVTONG
                    ImageProc::gated3D_v2(frame_a_3d ? prev_img : img_mem[thread_idx], frame_a_3d ? img_mem[thread_idx] : prev_img, modified_result[thread_idx],
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_DELAY_EDT->text().toFloat() : (delay_dist - p_tcu->get(TCU::OFFSET_DELAY) * dist_ns) / dist_ns,
                                          pref->custom_3d_param ? pref->ui->CUSTOM_3D_GW_EDT->text().toFloat() : (depth_of_view - p_tcu->get(TCU::OFFSET_GW) * dist_ns) / dist_ns,
                                          pref->colormap, pref->lower_3d_thresh, pref->upper_3d_thresh, pref->truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif //LVTONG
                    frame_a_3d ^= 1;
                }
#ifdef LVTONG
#else //LVTONG
                prev_3d = modified_result[thread_idx].clone();
#endif //LVTONG
            }
            else modified_result[thread_idx] = prev_3d;
#ifdef LVTONG
#else
            cv::Mat masked_dist;
            if (list_roi.size()) dist_mat.copyTo(masked_dist, user_mask[thread_idx]), emit update_dist_mat(masked_dist, dist_min, dist_max);
            else emit update_dist_mat(dist_mat, dist_min, dist_max);
#endif
//            cv::resize(modified_result[thread_idx], img_display, cv::Size(disp->width(), disp->height()), 0, 0, cv::INTER_AREA);
        }
        // process ordinary image enhance
        else {
            ww = _w;
            hh = _h;
            if (ui->IMG_ENHANCE_CHECK->isChecked()) {
                switch (ui->ENHANCE_OPTIONS->currentIndex()) {
                // histogram
                case 1: {
//                    cv::equalizeHist(modified_result, modified_result);
                    ImageProc::hist_equalization(modified_result[thread_idx], modified_result[thread_idx]);
                    break;
                }
                // laplace
                case 2: {
                    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, 0, 5, 0, 0, -1, 0);
                    cv::filter2D(modified_result[thread_idx], modified_result[thread_idx], CV_8U, kernel);
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
                    ImageProc::plateau_equl_hist(modified_result[thread_idx], modified_result[thread_idx], 4);
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
                    ImageProc::accumulative_enhance(modified_result[thread_idx], modified_result[thread_idx], pref->accu_base);
                    break;
                }
                // TODO rewrite sigmoid enchance
                // sigmoid (nonlinear) (mergw log w/ 1/(1+exp))
                case 5: {
                    ImageProc::guided_image_filter(modified_result[thread_idx], modified_result[thread_idx], 60, 0.01, 1);
//                    uchar *img = modified_result.data;
//                    cv::Mat img_log, img_nonLT = cv::Mat(h, w, CV_8U);
//                    modified_result.convertTo(img_log, CV_32F);
//                    modified_result += 1.0;
//                    cv::log(img_log, img_log);
////                    img_log *= settings->log;
//                    img_log *= 1.2;
//                    double m = 0, kv = 0, mean = cv::mean(modified_result)[0];
//                    uchar p;
//                    for (int i = 0; i < h; i++) {
//                        for (int j = 0; j < w; j++) {
//                            p = img[i * modified_result.step + j];
//                            if (!p) {
//                                img_nonLT.data[i * img_nonLT.step + j] = 0;
//                                continue;
//                            }
//                            if      (p <=  60) kv = 7;
//                            else if (p <= 200) kv = 7 + (p - 60) / 70;
//                            else if (p <= 255) kv = 9 + (p - 200) / 55;
//                            m = kv * (p / (p + mean));
//                            img_nonLT.data[i * img_nonLT.step + j] = (int)(2 / (1 + exp(-m)) - 1) * 255 - p;
//                        }
//                    }
//                    cv::normalize(img_log, img_log, 0, 255, cv::NORM_MINMAX);
//                    cv::convertScaleAbs(img_log, img_log);
//                    modified_result = 0.05 * img_log + 0.05 * img_nonLT + 0.8 * modified_result;
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
                    ImageProc::adaptive_enhance(modified_result[thread_idx], modified_result[thread_idx], pref->low_in, pref->high_in, pref->low_out, pref->high_out, pref->gamma);
                    break;
                }
                // enhance_dehaze
                case 7: {
                    modified_result[thread_idx] = ~modified_result[thread_idx];
                    ImageProc::haze_removal(modified_result[thread_idx], modified_result[thread_idx], 7, pref->dehaze_pct, 0.1, 60, 0.01);
                    modified_result[thread_idx] = ~modified_result[thread_idx];
                    break;
                }
                // dcp
                case 8: {
                    ImageProc::haze_removal(modified_result[thread_idx], modified_result[thread_idx], 7, pref->dehaze_pct, 0.1, 60, 0.01);
                    break;
                }
                // aindane
                case 9: {
                    ImageProc::aindane(modified_result[thread_idx], modified_result[thread_idx]);
                    break;
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
            ImageProc::brightness_and_contrast(modified_result[thread_idx], modified_result[thread_idx], std::exp(ui->CONTRAST_SLIDER->value() / 10.), ui->BRIGHTNESS_SLIDER->value() * 12.8);
            // map [0, 20] to [0, +inf) using tan
            ImageProc::brightness_and_contrast(modified_result[thread_idx], modified_result[thread_idx], tan((20 - ui->GAMMA_SLIDER->value()) / 40. * M_PI));
        }

        if (ui->PSEUDOCOLOR_CHK->isChecked() && thread_idx == 0 && modified_result[thread_idx].channels() == 1) {
            cv::Mat mask;
//            if (true) {
//                cv::Mat mean, stddev;
//                cv::meanStdDev(modified_result[thread_idx], mean, stddev);
//                double m = mean.at<double>(0, 0), s = stddev.at<double>(0, 0);
//                cv::threshold(modified_result[thread_idx], modified_result[thread_idx], m - 2 * s, 0, cv::THRESH_TOZERO);
//                cv::threshold(modified_result[thread_idx], modified_result[thread_idx], m + 2 * s, 0, cv::THRESH_TOZERO_INV);
//                modified_result[thread_idx].convertTo(mask, CV_8UC1);
//            }
            cv::normalize(modified_result[thread_idx], modified_result[thread_idx], 0, 255, cv::NORM_MINMAX, CV_8UC1, mask);
            cv::Mat temp_mask = modified_result[thread_idx], result_3d;
            cv::applyColorMap(modified_result[thread_idx], result_3d, pref->colormap);
            result_3d.copyTo(modified_result[thread_idx], temp_mask);
        }

        // display the gray-value histogram of the current grayscale image, or the distance histogram of the current 3D image
        if (alt_display_option == 2) {
            cv::Mat mat_for_hist;
            if (!is_color[thread_idx] && !pseudocolor[thread_idx]) mat_for_hist = modified_result[thread_idx].clone();
            else                       cv::cvtColor(modified_result[thread_idx], mat_for_hist, cv::COLOR_RGB2GRAY);
            for (int i = 0; i < _h; i++) for (int j = 0; j < _w; j++) hist[(mat_for_hist.data + i * mat_for_hist.cols)[j]]++;

            uchar *img = mat_for_hist.data;
            int step = mat_for_hist.step;
            memset(hist, 0, 256 * sizeof(uint));
            for (int i = 0; i < hh; i++) for (int j = 0; j < ww; j++) hist[(img + i * step)[j]]++;
            uint max = 0;
            for (int i = 1; i < 256; i++) {
                // discard abnormal value
                //                    if (hist[i] > 50000) hist[i] = 0;
                if (hist[i] > max) max = hist[i];
            }
            hist_mat = cv::Mat(225, 256, CV_8UC3, cv::Scalar(56, 64, 72));
            for (int i = 1; i < 256; i++) {
                cv::rectangle(hist_mat, cv::Point(i, 225), cv::Point(i + 1, 225 - hist[i] * 225.0 / max), cv::Scalar(202, 225, 255));
            }
            // TODO change to signal/slots
//            ui->HIST_DISPLAY->setPixmap(QPixmap::fromImage(QImage(hist_mat.data, hist_mat.cols, hist_mat.rows, hist_mat.step, QImage::Format_RGB888).scaled(ui->HIST_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            emit set_hist_pixmap(QPixmap::fromImage(QImage(hist_mat.data, hist_mat.cols, hist_mat.rows, hist_mat.step, QImage::Format_RGB888).scaled(ui->HIST_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }

        // FIXME possible crash when moving scaled image to the corners
        // crop the region to display
//        cv::Rect region = cv::Rect(disp->display_region.tl() * (image_3d ? ww + 104 : ww) / disp->width(), disp->display_region.br() * (image_3d ? ww + 104 : ww) / disp->width());
        cv::Rect region = cv::Rect(disp->display_region.tl() * ww / disp->width(), disp->display_region.br() * ww / disp->width());
        // qDebug() << disp->display_region.tl().x << disp->display_region.tl().y << disp->display_region.width << disp->display_region.height;
        // qDebug() << disp->size();
        if (region.height + region.y > modified_result[thread_idx].rows) region.height = modified_result[thread_idx].rows - region.y;
        if (region.width + region.x > modified_result[thread_idx].cols) region.width = modified_result[thread_idx].cols - region.x;
        // qDebug("region x: %d, y: %d, w: %d, h: %d", region.x, region.y, region.width, region.height);
        // qDebug("image  w: %d, h: %d", modified_result[thread_idx].cols, modified_result[thread_idx].rows);
        modified_result[thread_idx] = modified_result[thread_idx](region);
        cv::resize(modified_result[thread_idx], modified_result[thread_idx], cv::Size(ww, hh));

        // put info (dist, dov, time) as text on image
        QString info_tcu = pref->custom_topleft_info ?
                    pref->ui->CUSTOM_INFO_EDT->text() :
                    base_unit == 2 ? QString::asprintf("DIST %05d m  DOV %04d m", (int)delay_dist, (int)depth_of_view) :
                                     QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(delay_dist / dist_ns), (int)std::round(depth_of_view / dist_ns));
        QString info_time = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");
        if (ui->INFO_CHECK->isChecked()) {
            cv::putText(modified_result[thread_idx], info_tcu.toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            cv::putText(modified_result[thread_idx], info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
            static int baseline = 0;
            if (pref->fishnet_recog) {
//                cv::putText(modified_result[display_idx], "FISHNET", cv::Point(ww - 40 - cv::getTextSize("FISHNET", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                cv::putText(modified_result[thread_idx], is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            }
#endif
        }

        // resize to display size
        cv::resize(modified_result[thread_idx], img_display, cv::Size(disp->width(), disp->height()), 0, 0, cv::INTER_AREA);
        bool use_mask = modified_result[thread_idx].channels() == 1;
        cv::Mat info_mask(cv::Size(disp->width(), disp->height()), CV_8U, cv::Scalar(0)), thresh_result;
        if (use_mask) cv::threshold(img_display, thresh_result, 128, 255, cv::THRESH_BINARY);
        // draw the center cross
        if (!image_3d[thread_idx] && ui->CENTER_CHECK->isChecked()) {
//            for (int i = img_display.cols / 2 - 9; i < img_display.cols / 2 + 10; i++) img_display.at<uchar>(img_display.rows / 2, i) = img_display.at<uchar>(img_display.rows / 2, i) > 127 ? 0 : 255;
//            for (int i = img_display.rows / 2 - 9; i < img_display.rows / 2 + 10; i++) img_display.at<uchar>(i, img_display.cols / 2) = img_display.at<uchar>(i, img_display.cols / 2) > 127 ? 0 : 255;
            cv::line(use_mask ? info_mask : img_display, cv::Point(info_mask.cols / 2 - 9, info_mask.rows / 2), cv::Point(info_mask.cols / 2 + 10, info_mask.rows / 2), cv::Scalar(255), 1);
            cv::line(use_mask ? info_mask : img_display, cv::Point(info_mask.cols / 2, info_mask.rows / 2 - 9), cv::Point(info_mask.cols / 2, info_mask.rows / 2 + 10), cv::Scalar(255), 1);
        }
        if (ui->SELECT_TOOL->isChecked() && disp->selection_v1 != disp->selection_v2) {
            cv::rectangle(use_mask ? info_mask : img_display, disp->selection_v1, disp->selection_v2, cv::Scalar(255));
            cv::circle(use_mask ? info_mask : img_display, (disp->selection_v1 + disp->selection_v2) / 2, 1, cv::Scalar(255), -1);
        }
        if (use_mask) img_display = (img_display & (~info_mask)) + ((~thresh_result) & info_mask);
//        img_display &= ~info_mask;
//        img_display += ~thresh_result & info_mask;

        // image display
//        stream = QImage(cropped_img.data, cropped_img.cols, cropped_img.rows, cropped_img.step, QImage::Format_RGB888);
        stream = QImage(img_display.data, img_display.cols, img_display.rows, img_display.step, is_color[thread_idx] || pseudocolor[thread_idx] ? QImage::Format_RGB888 : QImage::Format_Indexed8);
//        qDebug() << "img display" << img_display.cols << img_display.rows << disp->size() << is_color[thread_idx] << pseudocolor[thread_idx] << stream.isNull();
//        ui->SOURCE_DISPLAY->setPixmap(QPixmap::fromImage(stream.scaled(ui->SOURCE_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        // use signal->slot instead of directly call
        disp->emit set_pixmap(QPixmap::fromImage(stream.scaled(disp->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        if (*idx == 0 && ui->DUAL_DISPLAY_CHK->isChecked()) {
            cv::Mat ori_img_display;
            Display *ori_display = secondary_display->get_display_widget();
            cv::resize(img_mem[thread_idx], ori_img_display, cv::Size(ori_display->width(), ori_display->height()), 0, 0, cv::INTER_AREA);
            stream = QImage(ori_img_display.data, ori_img_display.cols, ori_img_display.rows, ori_img_display.step, is_color[thread_idx] ? QImage::Format_RGB888 : QImage::Format_Indexed8);
            ori_display->emit set_pixmap(QPixmap::fromImage(stream).scaled(ori_display->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
//        qDebug() << QDateTime::currentDateTime().toString("ss.zzz");

//        qDebug() << "--------------------" << thread_idx;
        // process scan
        if (thread_idx ==0 && scan && updated) {
            static cv::Mat temp;
            scan_img_count -= 1;
            if (scan_img_count < 0) scan_img_count = 0;

            static QString scan_save_path_a, scan_save_path;
            int save_img_num = scan_config->num_single_pos;
//            if (thread_idx == 1) qDebug() << "##--##" << scan_img_count << save_img_num;
            if (scan_img_count > 0 && scan_img_count <= save_img_num) {
//                qDebug() << "====================" << thread_idx;
                save_scan_img(scan_save_path, QString::number(save_img_num - scan_img_count + 1) + ".bmp");
            }
            if (thread_idx == 0 && scan_img_count == save_img_num) {
                window()->grab().save(scan_save_path + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
            }

            if (scan_img_count == 0) ;

            if (!scan_img_count) {
                scan_img_count = frame_rate_edit * (scan_config->ptz_wait_time + save_img_num / frame_rate_edit);
//                qDebug() << "scan idx" << scan_tcu_idx << scan_ptz_idx;

//                qDebug() << "delay" << scan_tcu_idx;
                if (scan_tcu_idx == scan_tcu_route.size() - 1 || scan_tcu_idx == -1) {
                    scan_tcu_idx = -1;
                    scan_ptz_idx++;

                    if (scan_ptz_idx < scan_ptz_route.size()) {
//                        qDebug() << scan_ptz_route[scan_ptz_idx];
                        p_usbcan->emit control(USBCAN::POSITION, scan_ptz_route[scan_ptz_idx].first);
                        p_usbcan->emit control(USBCAN::PITCH, scan_ptz_route[scan_ptz_idx].second);

                        scan_save_path_a = save_location + "/" + scan_name + "/" + QString::number(scan_ptz_idx);
                        QDir().mkdir(scan_save_path_a);
                        QFile params(scan_save_path_a + "/params");
                        params.open(QIODevice::WriteOnly);
                        params.write(QString::asprintf("angle: \n"
                                                       "    H: %06.2f\n"
                                                       "    V:  %05.2f\n",
                                                       scan_ptz_route[scan_ptz_idx].first,
                                                       scan_ptz_route[scan_ptz_idx].second).toUtf8());
                    }
                    else {
                        scan_img_count = -1;
                        on_SCAN_BUTTON_clicked();
                    }
                }

                scan_tcu_idx++;
                delay_dist = scan_tcu_route[scan_tcu_idx] * dist_ns;
                scan_save_path = scan_save_path_a + "/" + QString::number(scan_tcu_route[scan_tcu_idx], 'f', 2) + " ns";
//                qDebug() << scan_save_path;
                QDir().mkdir(scan_save_path);
                QDir().mkdir(scan_save_path + "/ori_bmp");
                QDir().mkdir(scan_save_path + "/res_bmp");
                if (grab_image[1]) QDir().mkdir(scan_save_path + "/alt_bmp");

                emit update_delay_in_thread();
            }
        }
//        if (thread_idx == 0 && scan && scan_tcu_idx == scan_tcu_route.size() && scan_ptz_idx == scan_ptz_route.size() - 1) on_SCAN_BUTTON_clicked();
//        if (scan && std::round(delay_dist / dist_ns) > scan_stopping_delay) {on_SCAN_BUTTON_clicked();}

//        cv::imwrite("../a.bmp", img_mem[thread_idx]);
//        cv::imwrite("../b.bmp", modified_result[thread_idx]);

        // image write / video record
        if (updated && save_original[thread_idx]) {
            save_to_file(false, thread_idx);
            if (device_type == -1 || !pref->consecutive_capture || *idx) save_original[thread_idx] = 0;
        }
        if (updated && save_modified[thread_idx]) {
            save_to_file(true, thread_idx);
            if (device_type == -1 || !pref->consecutive_capture || *idx) save_modified[thread_idx] = 0;
        }
        if (updated && record_original[thread_idx]) {
            cv::Mat temp = img_mem[thread_idx].clone();
            if (pref->save_info) {
                cv::putText(temp, info_tcu.toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                cv::putText(temp, info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            }
            if (is_color[thread_idx]) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
            // inc by 1 because main display uses two videowriter
            if (*idx) vid_out[thread_idx + 1].write(temp);
            else      vid_out[0].write(temp);
        }
        if (updated && record_modified[thread_idx]) {
            cv::Mat temp = modified_result[thread_idx].clone();
            if (!image_3d[thread_idx] && !ui->INFO_CHECK->isChecked()) {
                if (pref->save_info) {
                    cv::putText(modified_result[thread_idx], info_tcu.toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                    cv::putText(modified_result[thread_idx], info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
                    static int baseline = 0;
                    if (pref->fishnet_recog) {
//                        cv::putText(modified_result[display_idx], "FISHNET", cv::Point(ww - 40 - cv::getTextSize("FISHNET", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                        cv::putText(modified_result[thread_idx], is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pref->fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                    }
#endif
                }
            }
            if (is_color[thread_idx] || pseudocolor[thread_idx]) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
            // inc by 1 because main display uses two videowriter
            if (*idx) vid_out[thread_idx + 1].write(temp);
            else      vid_out[1].write(temp);
        }

        if (updated) prev_img = img_mem[thread_idx].clone();
        } // QMutexLocker automatically unlocks here
    }
//    free(range);
#ifdef LVTONG
    pref->ui->MODEL_LIST->setEnabled(true);
#endif
    grab_thread_state[thread_idx] = false;
    return 0;
}

void UserPanel::swap_grab_thread_display(int display_idx1, int display_idx2)
{
    int thread_idx1 = display_thread_idx[display_idx1],
        thread_idx2 = display_thread_idx[display_idx2];
    qDebug() << "swap thread" << thread_idx1 << thread_idx2;

    if (thread_idx1 != -1 && thread_idx1 != -1) {
        image_mutex[thread_idx1].lock();
        image_mutex[thread_idx2].lock();

        h_grab_thread[thread_idx1]->display_idx(false, display_idx2);
        h_grab_thread[thread_idx2]->display_idx(false, display_idx1);
        display_thread_idx[display_idx1] = thread_idx2;
        display_thread_idx[display_idx2] = thread_idx1;

        image_mutex[thread_idx1].unlock();
        image_mutex[thread_idx2].unlock();
    }
    else if (thread_idx1 != -1) {
        image_mutex[thread_idx1].lock();

        h_grab_thread[thread_idx1]->display_idx(false, display_idx2);
        display_thread_idx[display_idx1] = thread_idx2;
        display_thread_idx[display_idx2] = thread_idx1;

        image_mutex[thread_idx1].unlock();
        displays[display_idx1]->clear();
    }
    else if (thread_idx2 != -1) {
        image_mutex[thread_idx2].lock();

        h_grab_thread[thread_idx2]->display_idx(false, display_idx1);
        display_thread_idx[display_idx1] = thread_idx2;
        display_thread_idx[display_idx2] = thread_idx1;

        image_mutex[thread_idx2].unlock();
        displays[display_idx2]->clear();
    }
}

inline bool UserPanel::is_maximized()
{
    return ui->TITLE->is_maximized;
}

// used when moving temp recorded vid to destination
void UserPanel::move_to_dest(QString src, QString dst)
{
    qDebug() << src << "->" << dst;
    QFile::rename(src, dst);
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
    pref->ui->REFRESH_AVAILABLE_PORTS_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    ui->SWITCH_TCU_UI_BTN->setIcon(QIcon(":/tools/" + QString(app_theme ? "light" : "dark") + "/switch"));

//    for (int i = 0 ; i < 5; i++) if (serial_port_connected[i]) com_label[i]->setStyleSheet(app_theme ? "color: #180D1C;" : "color: #B0C4DE;");
//    ptr_tcu->set_theme();
//    ptr_lens->set_theme();
//    ptr_laser->set_theme();
//    ptr_ptz->set_theme();
    ui->TCU_COM->setStyleSheet(p_tcu->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");
    ui->LENS_COM->setStyleSheet(p_lens->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");
    ui->LASER_COM->setStyleSheet(p_laser->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");
    ui->PTZ_COM->setStyleSheet(p_ptz->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");

    QFont temp_f(consolas);
    temp_f.setPixelSize(11);
    ui->DATA_EXCHANGE->setFont(temp_f);

    setCursor(cursor_curr_pointer);

    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s/%s);", app_theme ? "light" : "dark", hide_left ? "right" : "left"));

    laser_settings->set_theme();

#if ENABLE_USER_DEFAULT
    FILE *f = fopen("user_default", "rb+");
    if (!f) return;
    fseek(f, 3, SEEK_SET);
    fwrite(&app_theme, 1, 1, f);
    fclose(f);
#endif
}

void UserPanel::stop_image_writing()
{
    if (save_original[0]) on_SAVE_BMP_BUTTON_clicked();
    if (save_modified[0]) on_SAVE_RESULT_BUTTON_clicked();
    QMessageBox::warning(this, "PROMPT", "too much memory used, stopping writing images");
//    QElapsedTimer t;
//    t.start();
//    while (t.elapsed() < 1000) ((QApplication*)QApplication::instance())->processEvents();
//    QThread::msleep(1000);
}

// FIXME font change when switching language
void UserPanel::switch_language()
{
    lang ^= 1;
    lang ? QApplication::installTranslator(&trans) : QApplication::removeTranslator(&trans);
//    lang ? QApplication::setFont(consolas) : QApplication::setFont(monaco);
    ui->retranslateUi(this);
//    ui->TITLE->prog_settings->switch_language(en, &trans);
    pref->ui->retranslateUi(pref);
//    pref->switch_language(en, &trans);
//    int idx = ui->ENHANCE_OPTIONS->currentIndex();
//    ui->ENHANCE_OPTIONS->clear();
//    ui->ENHANCE_OPTIONS->addItem(tr("None"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Histogram"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Laplace"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Log-based"));
//    ui->ENHANCE_OPTIONS->addItem(tr("Gamma-based"));
//    ui->ENHANCE_OPTIONS->setCurrentIndex(idx);

    if (simple_ui & lang) {
        ui->ZOOM_IN_BTN->setText(tr("IN"));
        ui->ZOOM_OUT_BTN->setText(tr("OUT"));
        ui->FOCUS_NEAR_BTN->setText(tr("NEAR"));
        ui->FOCUS_FAR_BTN->setText(tr("FAR"));
        ui->RADIUS_INC_BTN->setText(tr("INC"));
        ui->RADIUS_DEC_BTN->setText(tr("DEC"));
    }
    else {
        ui->ZOOM_IN_BTN->setText("+");
        ui->ZOOM_OUT_BTN->setText("-");
        ui->FOCUS_NEAR_BTN->setText("+");
        ui->FOCUS_FAR_BTN->setText("-");
        ui->RADIUS_INC_BTN->setText("+");
        ui->RADIUS_DEC_BTN->setText("-");
    }

#if ENABLE_USER_DEFAULT
    FILE *f = fopen("user_default", "rb+");
    if (!f) return;
    fseek(f, 4, SEEK_SET);
    fwrite(&lang, 1, 1, f);
    fclose(f);
#endif
}

void UserPanel::closeEvent(QCloseEvent *event)
{
    shut_down();
    event->accept();
//    ui->TITLE->prog_settings->reject();
    pref->reject();
    laser_settings->reject();
    scan_config->reject();
    for (FloatingWindow* &w: fw_display) w->close();
    secondary_display->close();
    if (QFile::exists("HQVSDK.xml")) QFile::remove("HQVSDK.xml");
    aliasing->close();
    preset->close();
}

int UserPanel::shut_down() {
    if (record_original[0]) on_SAVE_AVI_BUTTON_clicked();
    if (record_modified[0]) on_SAVE_FINAL_BUTTON_clicked();

    grab_image[0] = false;
    if (h_grab_thread[0]) {
        h_grab_thread[0]->quit();
        h_grab_thread[0]->wait();
        delete h_grab_thread[0];
        h_grab_thread[0] = NULL;
    }

    if (!q_img[0].empty()) std::queue<cv::Mat>().swap(q_img[0]);

    if (curr_cam) {
        int shutdown_result = curr_cam->shut_down();
        if (shutdown_result != 0) {
            qWarning("Camera shutdown failed with error code: %d", shutdown_result);
            // Try to force cleanup even if shutdown failed
        }
        delete curr_cam;
        curr_cam = NULL;
    }

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
    ui->SENSOR_TAPS_BTN->setEnabled(device_on && !start_grabbing);
    ui->IMG_3D_CHECK->setEnabled(start_grabbing);
    ui->PSEUDOCOLOR_CHK->setEnabled(start_grabbing);
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

void UserPanel::init_control_port()
{
    p_tcu   = new TCU(0);
    p_lens  = new Lens(1);
    p_laser = new Laser(2);
    p_ptz   = new PTZ(3);
    p_rf    = new RangeFinder(4);
    th_tcu   = new QThread(this);
    th_lens  = new QThread(this);
    th_laser = new QThread(this);
    th_ptz   = new QThread(this);
    th_rf    = new QThread(this);
    p_tcu  ->moveToThread(th_tcu);
    p_lens ->moveToThread(th_lens);
    p_laser->moveToThread(th_laser);
    p_ptz  ->moveToThread(th_ptz);
    p_rf   ->moveToThread(th_rf);
    th_tcu  ->start();
    th_lens ->start();
    th_laser->start();
    th_ptz  ->start();
    th_rf   ->start();

    connect(ui->TCU_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { p_tcu->emit connect_to_serial(ui->TCU_COM_EDIT->text()); });
    connect(p_tcu, &ControlPort::port_status_updated, this, [this]() { update_port_status(p_tcu, ui->TCU_COM); });
    connect(p_tcu, &ControlPort::port_io_log, this, &UserPanel::append_data, Qt::QueuedConnection);
    connect(p_tcu, &TCU::tcu_param_updated, this, &UserPanel::update_tcu_params);
    connect(this, SIGNAL(send_double_tcu_msg(qint32, double)), p_tcu, SLOT(set_user_param(qint32, double)), Qt::QueuedConnection);
    connect(this, SIGNAL(send_uint_tcu_msg(qint32, uint)), p_tcu, SLOT(set_user_param(qint32, uint)), Qt::QueuedConnection);

    connect(ui->LENS_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { p_lens->emit connect_to_serial(ui->LENS_COM_EDIT->text()); });
    connect(p_lens, &ControlPort::port_status_updated, this, [this]() { update_port_status(p_lens, ui->LENS_COM); });
    connect(p_lens, &ControlPort::port_io_log, this, &UserPanel::append_data, Qt::QueuedConnection);
    connect(p_lens, &Lens::lens_param_updated, this, &UserPanel::update_lens_params);
    connect(this, SIGNAL(send_lens_msg(qint32, uint)), p_lens, SLOT(lens_control(qint32, uint)), Qt::QueuedConnection);
    connect(this, SIGNAL(set_lens_pos(qint32, uint)), p_lens, SLOT(set_pos_temp(qint32, uint)), Qt::QueuedConnection);

    connect(ui->LASER_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { p_laser->emit connect_to_serial(ui->LASER_COM_EDIT->text()); });
    connect(p_laser, &ControlPort::port_status_updated, this, [this]() { update_port_status(p_laser, ui->LASER_COM); });
    connect(this, SIGNAL(send_laser_msg(QString)), p_laser, SLOT(laser_control(QString)), Qt::QueuedConnection);
//    connect(p_laser, &ControlPort::port_io, this, &UserPanel::append_data, Qt::QueuedConnection);

    connect(ui->PTZ_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { p_ptz->emit connect_to_serial(ui->PTZ_COM_EDIT->text()); });
    connect(p_ptz, &ControlPort::port_status_updated, this, [this]() { update_port_status(p_ptz, ui->PTZ_COM); });
    connect(p_ptz, &ControlPort::port_io_log, this, &UserPanel::append_data, Qt::QueuedConnection);
    connect(p_ptz, &PTZ::ptz_param_updated, this, &UserPanel::update_ptz_params);
    connect(this, SIGNAL(send_ptz_msg(qint32, double)), p_ptz, SLOT(ptz_control(qint32, double)), Qt::QueuedConnection);

    connect(p_rf, &RangeFinder::distance_updated, this, &UserPanel::update_distance);

    p_usbcan = new USBCAN();
    th_usbcan = new QThread(this);
    p_usbcan->moveToThread(th_usbcan);
    th_usbcan->start();
    connect(p_usbcan, &USBCAN::angle_updated, this, &UserPanel::update_usbcan_angle);
}

void UserPanel::save_to_file(bool save_result, int idx) {
//    QString temp = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"),
//            dest = QString(save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//    cv::imwrite(temp.toLatin1().data(), save_result ? modified_result : img_mem);
//    QFile::rename(temp, dest);

    cv::Mat *temp = save_result ? &modified_result[idx] : &img_mem[idx];
//    QPixmap::fromImage(QImage(temp->data, temp->cols, temp->rows, temp->step, temp->channels() == 3 ? QImage::Format_RGB888 : QImage::Format_Indexed8)).save(save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp", "BMP", 100);

//    std::thread t_save(UserPanel::save_image_bmp, *temp, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//    t_save.detach();

//    if (!tp.append_task(std::bind(UserPanel::save_image_tif, tif_16, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full();
//    if (device_type < 0) {
//        UserPanel::save_image_bmp(temp->clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//        return;
//    }
    // TODO implement 16bit result image processing/writing
    cv::Mat result_image;
    if (pref->save_in_grayscale) cv::cvtColor(*temp, result_image, cv::COLOR_RGB2GRAY);
    else                         result_image = temp->clone();
    if (save_result) {
        QString dt = QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz");
        if (pref->split) {
            if (!tp.append_task(std::bind(ImageIO::save_image_bmp, result_image(cv::Rect(             0,              0, temp->cols / 2, temp->rows / 2)).clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_0" + ".bmp"))) emit task_queue_full();
            if (!tp.append_task(std::bind(ImageIO::save_image_bmp, result_image(cv::Rect(temp->cols / 2,              0, temp->cols / 2, temp->rows / 2)).clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_1" + ".bmp"))) emit task_queue_full();
            if (!tp.append_task(std::bind(ImageIO::save_image_bmp, result_image(cv::Rect(             0, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_2" + ".bmp"))) emit task_queue_full();
            if (!tp.append_task(std::bind(ImageIO::save_image_bmp, result_image(cv::Rect(temp->cols / 2, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_3" + ".bmp"))) emit task_queue_full();
        }
        switch (pref->img_format){
        case 0: if (!tp.append_task(std::bind(ImageIO::save_image_bmp, result_image, save_location + "/res_bmp/" + dt + ".bmp"))) emit task_queue_full(); break;
        case 1: if (!tp.append_task(std::bind(ImageIO::save_image_jpg, result_image, save_location + "/res_bmp/" + dt + ".jpg"))) emit task_queue_full(); break;
        default: break;
        }
    } else {
        switch (pixel_depth[0]) {
        case  8:
            switch (pref->img_format){
            case 0: if (!tp.append_task(std::bind(ImageIO::save_image_bmp, result_image, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"))) emit task_queue_full(); break;
            case 1: if (!tp.append_task(std::bind(ImageIO::save_image_jpg, result_image, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        case 10:
        case 12:
        case 16:
            switch (pref->img_format){
            case 0: if (!tp.append_task(std::bind(ImageIO::save_image_tif, result_image * (1 << (16 - pixel_depth[0])), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full(); break;
            case 1: if (!tp.append_task(std::bind(ImageIO::save_image_jpg, result_image * (1 << (16 - pixel_depth[0])), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        default: break;
        }
    }
}

void UserPanel::save_scan_img(QString path, QString name) {
//        QString temp = QString(TEMP_SAVE_LOCATION + "/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp"),
//                dest = QString(save_location + "/" + scan_name + "/ori_bmp/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp");
//        cv::imwrite(temp.toLatin1().data(), img_mem);
//        QFile::rename(temp, dest);
//        if (ui->TITLE->prog_settings->save_scan_ori) QPixmap::fromImage(QImage(img_mem.data, img_mem.cols, img_mem.rows, img_mem.step, QImage::Format_Indexed8)).save(save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp", "BMP");
    if (scan_config->capture_scan_ori) {
//            std::thread t_ori(save_image_bmp, img_mem, save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp");
//            t_ori.detach();
//            tp.append_task(std::bind(ImageIO::save_image_bmp, img_mem[0].clone(), save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%.2fm", delay_dist) : QString::asprintf("%.2fns", ptr_tcu->delay_a)) + ".bmp"));
//            tp.append_task(std::bind(ImageIO::save_image_bmp, img_mem[0].clone(), save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%.2fm", delay_dist) : QString::asprintf("%.2fns", p_tcu->get(TCU::DELAY_A))) + ".bmp"));
        tp.append_task(std::bind(ImageIO::save_image_bmp, img_mem[0].clone(), path + "/ori_bmp/" + name));

    }
//        dest = QString(save_location + "/" + scan_name + "/res_bmp/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp");
//        cv::imwrite(temp.toLatin1().data(), modified_result);
//        QFile::rename(temp, dest);
//        if (ui->TITLE->prog_settings->save_scan_res) QPixmap::fromImage(QImage(modified_result.data, modified_result.cols, modified_result.rows, modified_result.step, QImage::Format_Indexed8)).save(save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp", "BMP");
    if (scan_config->capture_scan_res) {
//            std::thread t_res(save_image_bmp, modified_result, save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp");
//            t_res.detach();
//            tp.append_task(std::bind(ImageIO::save_image_bmp, modified_result[0].clone(), save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%.2fns", ptr_tcu->delay_a)) + ".bmp"));
//            tp.append_task(std::bind(ImageIO::save_image_bmp, modified_result[0].clone(), save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%.2fns", p_tcu->get(TCU::DELAY_A))) + ".bmp"));
        tp.append_task(std::bind(ImageIO::save_image_bmp, modified_result[0].clone(), path + "/res_bmp/" + name));
    }

//    qDebug() << "####################" << thread_idx;
    if (grab_image[1]) tp.append_task(std::bind(ImageIO::save_image_bmp, img_mem[1].clone(), path + "/alt_bmp/" + name));
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
//            ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//            ptr_tcu->set_user_param(TCU::LASER_WIDTH, ptr_tcu->laser_width);
//            ptr_tcu->set_user_param(TCUThread::LASER_USR, ptr_tcu->laser_width);
            emit send_double_tcu_msg(TCU::LASER_USR, p_tcu->get(TCU::LASER_WIDTH));
            update_delay();
            update_gate_width();
            change_mcp(5);
            break;
        case 2:
            ui->GET_LENS_PARAM_BTN->click();
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
#if 0
    FILE *f = fopen("user_default", "rb+");
    if (!f) return;
    uchar port_num_uchar = port_num.toUInt() & 0xFF;
    fseek(f, id, SEEK_SET);
    fwrite(&port_num_uchar, 1, 1, f);
    fclose(f);
#endif
}

void UserPanel::on_ENUM_BUTTON_clicked()
{
    if (pref->ebus_cam) ui->SENSOR_TAPS_BTN->show();
    else                ui->SENSOR_TAPS_BTN->hide();

    if (curr_cam) delete curr_cam;
#ifdef WIN32
#ifdef USING_CAMERALINK
    if (pref->cameralink)    curr_cam = new EuresysCam;
    else if (pref->ebus_cam) curr_cam = new EbusCam;
    else                     curr_cam = new MvCam;
#else
    if (pref->ebus_cam) curr_cam = new EbusCam;
    else                curr_cam = new MvCam;
#endif
#else
    curr_cam = new MvCam;
#endif
//    if (ui->TITLE->prog_settings->cameralink) curr_cam->cameralink = true;
    enable_controls(device_type = (curr_cam->search_for_devices() ?  (pref->cameralink ? 3 : (pref->ebus_cam ? 2 : 1)) : 0));
    update_dev_ip();
    emit update_device_list(1, curr_cam->get_device_list());
}

void UserPanel::on_START_BUTTON_clicked()
{
    if (device_on) return;

    if (!curr_cam) {
        QMessageBox::warning(this, "PROMPT", tr("No camera available"));
        return;
    }

    if (curr_cam->start(pref->device_idx)) {
        QMessageBox::warning(this, "PROMPT", tr("start failed"));
        return;
    }
    device_on = true;
    enable_controls(true);

    on_SET_PARAMS_BUTTON_clicked();
    ui->GAIN_SLIDER->setMinimum(0);
    ui->GAIN_SLIDER->setMaximum(curr_cam->device_type == 1 ? 23 : 480);
    ui->GAIN_SLIDER->setSingleStep(1);
    ui->GAIN_SLIDER->setPageStep(4);
    ui->GAIN_SLIDER->setValue(gain_analog_edit);
    connect(ui->GAIN_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_gain(int)));

//    on_CONTINUOUS_RADIO_clicked();
    on_TRIGGER_RADIO_clicked();

    curr_cam->time_exposure(true, &time_exposure_edit);
    curr_cam->gain_analog(true, &gain_analog_edit);
    curr_cam->frame_rate(true, &frame_rate_edit);
    ui->GAIN_SLIDER->setValue((int)gain_analog_edit);
    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));

#ifdef LVTONG
    int new_w = 1024;
    int new_h = 1024;
    int new_x = 296;
    int new_y = 32;
    curr_cam->frame_size(false, &new_w, &new_h);
    curr_cam->frame_offset(false, &new_x, &new_y);
#endif

//    ui->TITLE->prog_settings->set_pixel_format(0);
    pref->set_pixel_format(0);
    emit device_connection_status_changed(0, QStringList());
}

void UserPanel::on_SHUTDOWN_BUTTON_clicked()
{
    ui->STOP_GRABBING_BUTTON->click();
    shut_down();
    ui->IMG_ENHANCE_CHECK->setChecked(false);
    ui->FRAME_AVG_CHECK->setChecked(false);
    ui->IMG_3D_CHECK->setChecked(false);
    on_ENUM_BUTTON_clicked();
    QTimer::singleShot(100, this, SLOT(clean()));
    emit device_connection_status_changed(0, QStringList());
}

void UserPanel::on_CONTINUOUS_RADIO_clicked()
{
    ui->CONTINUOUS_RADIO->setChecked(true);
    ui->TRIGGER_RADIO->setChecked(false);
    ui->SOFTWARE_CHECK->setEnabled(false);
    trigger_mode_on = false;
    curr_cam->trigger_mode(false, &trigger_mode_on);

    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(false);
}

void UserPanel::on_TRIGGER_RADIO_clicked()
{
    ui->CONTINUOUS_RADIO->setChecked(false);
    ui->TRIGGER_RADIO->setChecked(true);
    ui->SOFTWARE_CHECK->setEnabled(true);
    trigger_mode_on = true;
    curr_cam->trigger_mode(false, &trigger_mode_on);

    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(start_grabbing && trigger_by_software);
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
    if (!curr_cam || !device_on) {
        qWarning("Cannot trigger: camera not initialized or device not on");
        return;
    }
    curr_cam->trigger_once();
}

void UserPanel::on_START_GRABBING_BUTTON_clicked()
{
//    if (!device_on || start_grabbing || curr_cam == NULL) return;
    if (!device_on || curr_cam == NULL) return;

    static struct main_ui_info cam_union = {&q_frame_info, q_img + 0};
    curr_cam->set_user_pointer(&cam_union);

    // TODO complete BayerGB process & display
    curr_cam->pixel_type(true, &pixel_format[0]);
    switch (pixel_format[0]) {
    case PixelType_Gvsp_Mono8:       is_color[0] = false; update_pixel_depth( 8); break;
    case PixelType_Gvsp_Mono10:      is_color[0] = false; update_pixel_depth(10); break;
    case PixelType_Gvsp_Mono12:      is_color[0] = false; update_pixel_depth(12); break;
    case PixelType_Gvsp_BayerGB8:    is_color[0] =  true; update_pixel_depth( 8); break;
    case PixelType_Gvsp_RGB8_Packed: is_color[0] =  true; update_pixel_depth( 8); break;
    default:                         is_color[0] = false; update_pixel_depth( 8); break;
    }

    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
//    ptr_tcu->duty = time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
//    ptr_tcu->ccd_freq = frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();
    emit send_double_tcu_msg(TCU::DUTY, time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000);
    emit send_double_tcu_msg(TCU::CCD_FREQ, frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat());

    int main_thread_idx = display_thread_idx[0];
    if (display_thread_idx[0] == -1) main_thread_idx = 0;

    if (grab_image[main_thread_idx] || h_grab_thread[main_thread_idx]) {
        grab_image[main_thread_idx] = false;
        h_grab_thread[main_thread_idx]->quit();
        h_grab_thread[main_thread_idx]->wait();
        delete h_grab_thread[main_thread_idx];
        h_grab_thread[main_thread_idx] = NULL;
        display_thread_idx[0] = -1;
    }

    // adjust display size according to frame size
    curr_cam->frame_size(true, &w[0], &h[0]);
    status_bar->img_resolution->set_text(QString::asprintf("%d x %d", w[0], h[0]));
    qInfo("frame w: %d, h: %d", w[0], h[0]);
    //    QRect region = ui->SOURCE_DISPLAY->geometry();
    //    region.setHeight(ui->SOURCE_DISPLAY->width() * h / w);
    //    ui->SOURCE_DISPLAY->setGeometry(region);
    //    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
    //    ui->SOURCE_DISPLAY->update_roi(QPoint());
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
    secondary_display->resize_display(w[0], h[0]);

    grab_image[0] = true;

    h_grab_thread[0] = new GrabThread((void*)this, 0);
    if (h_grab_thread[0] == NULL) {
        grab_image[0] = false;
        QMessageBox::warning(this, "PROMPT", tr("Create thread fail"));
        return;
    }
    h_grab_thread[0]->start();

    if (curr_cam->start_grabbing()) {
        grab_image[0] = false;
        if (h_grab_thread[0]) {
            h_grab_thread[0]->quit();
            h_grab_thread[0]->wait();
            delete h_grab_thread[0];
            h_grab_thread[0] = NULL;
        }
        return;
    }
    start_grabbing = true;
    ui->SOURCE_DISPLAY->is_grabbing = true;
    enable_controls(true);
    display_thread_idx[0] = 0;

#ifdef ICMOS
    ui->HIDE_BTN->click();
#endif
}

void UserPanel::on_STOP_GRABBING_BUTTON_clicked()
{
//    if (!device_on || !start_grabbing || curr_cam == NULL) return;

    if (record_original[0]) on_SAVE_AVI_BUTTON_clicked();
    if (record_modified[0]) on_SAVE_FINAL_BUTTON_clicked();

    int main_thread_idx = display_thread_idx[0];
//    if (display_thread_idx[0] != -1) main_thread_idx = 0;

    if (!q_img[main_thread_idx].empty()) std::queue<cv::Mat>().swap(q_img[main_thread_idx]);

    grab_image[main_thread_idx] = false;
    if (h_grab_thread[main_thread_idx]) {
        h_grab_thread[main_thread_idx]->quit();
        h_grab_thread[main_thread_idx]->wait();
        delete h_grab_thread[main_thread_idx];
        h_grab_thread[main_thread_idx] = NULL;
    }

    start_grabbing = false;
    ui->SOURCE_DISPLAY->is_grabbing = false;
    QTimer::singleShot(100, this, SLOT(clean()));
    enable_controls(device_type);
    if (device_type == -1) ui->ENUM_BUTTON->click();
//    if (device_type == -2) ui->ENUM_BUTTON->click(), video_input->stop(), video_surface->stop();
    if (device_type == -2) ui->ENUM_BUTTON->click();

    curr_cam->stop_grabbing();
    display_thread_idx[0] = -1;
}

void UserPanel::on_SAVE_BMP_BUTTON_clicked()
{
    if (!QDir(save_location + "/ori_bmp").exists()) QDir().mkdir(save_location + "/ori_bmp");
    if (device_type == -1 || !pref->consecutive_capture) save_original[0] = 1;
    else{
        save_original[0] ^= 1;
        ui->SAVE_BMP_BUTTON->setText(save_original[0] ? tr("STOP") : tr("ORI"));
        pref->ui->CONSECUTIVE_CAPTURE_CHK->setEnabled(!save_original[0]);
    }
}

void UserPanel::on_SAVE_FINAL_BUTTON_clicked()
{
    static QString res_avi;
//    cv::imwrite(QString(save_location + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".jpg").toLatin1().data(), modified_result);
    if (record_modified[0]) {
//        curr_cam->stop_recording(0);
        {
            QMutexLocker locker(&image_mutex[0]);
            vid_out[1].release();
        }
        QString dest = save_location + "/" + res_avi.section('/', -1, -1);
        std::thread t(UserPanel::move_to_dest, QString(res_avi), QString(dest));
        t.detach();
//        QFile::rename(res_avi, dest);
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        {
            QMutexLocker locker(&image_mutex[0]);
            res_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_res.avi");
//            vid_out[1].open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(image_3d[0] ? w[0] + 104 : w[0], h[0]), is_color[0] || image_3d[0]);
            vid_out[1].open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('X', '2', '6', '4'), frame_rate_edit, cv::Size(image_3d[0] ? w[0] + 104 : w[0], h[0]), is_color[0] || pseudocolor[0]);
        }
    }
    record_modified[0] ^= 1;
    ui->SAVE_FINAL_BUTTON->setText(record_modified[0] ? tr("STOP") : tr("RES"));
}

void UserPanel::on_SET_PARAMS_BUTTON_clicked()
{
    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
    time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
    frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();

    // CCD FREQUENCY
//    ptr_tcu->communicate_display(convert_to_send_tcu(0x06, 1.25e8 / fps), 7, 1, false);
//    ptr_tcu->set_user_param(TCUThread::CCD_FREQ, frame_rate_edit);
    emit send_double_tcu_msg(TCU::CCD_FREQ, frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat());

    // DUTY RATIO -> EXPO. TIME
//    ptr_tcu->communicate_display(convert_to_send_tcu(0x07, duty * 1.25e2), 7, 1, false);
//    ptr_tcu->set_user_param(TCUThread::DUTY, time_exposure_edit);
    emit send_double_tcu_msg(TCU::DUTY, time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000);

//    time_exposure_edit = ptr_tcu->duty;
//    frame_rate_edit = ptr_tcu->fps;
    if (device_on) {
        curr_cam->time_exposure(false, &time_exposure_edit);
        curr_cam->frame_rate(false, &frame_rate_edit);
//        curr_cam->gain_analog(false, &gain_analog_edit);
        change_gain(gain_analog_edit);
    }
    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));

    q_fps_calc.empty_q();
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

    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));
}

void UserPanel::on_FILE_PATH_BROWSE_clicked()
{
    QString temp = QFileDialog::getExistingDirectory(this, tr("Select folder"), save_location);
    if (!temp.isEmpty()) save_location = temp;
    if (save_original[0] && !QDir(temp + "/ori_bmp").exists()) QDir().mkdir(temp + "/ori_bmp");
    if (save_modified[0] && !QDir(temp + "/res_bmp").exists()) QDir().mkdir(temp + "/res_bmp");

    ui->FILE_PATH_EDIT->setText(save_location);
}

void UserPanel::clean()
{
    ui->SOURCE_DISPLAY->clear();
    ui->DATA_EXCHANGE->clear();
    if (grab_image[0]) return;
    std::queue<cv::Mat>().swap(q_img[0]);
    img_mem[0].release();
    modified_result[0].release();
//    img_display[0].release();
//    prev_img[0].release();
//    prev_3d[0].release();
//    for (cv::Mat &m: seq[0]) m.release();
//    seq_sum[0].release();
//    frame_a_sum[0].release();
//    frame_b_sum[0].release();
}

void UserPanel::setup_hz(int hz_unit)
{
    if (aliasing_mode) return;
    this->hz_unit = hz_unit;
    switch (hz_unit) {
    // kHz
    case 0: ui->FREQ_UNIT->setText("kHz"); ui->FREQ_EDIT->setText(QString::number(p_tcu->get(TCU::REPEATED_FREQ), 'f', 2)); break;
    // Hz
    case 1: ui->FREQ_UNIT->setText("Hz"); ui->FREQ_EDIT->setText(QString::number((int)(p_tcu->get(TCU::REPEATED_FREQ) * 1000))); break;
    default: break;
    }
}

void UserPanel::setup_stepping(int base_unit)
{
    this->base_unit = base_unit;
    switch (base_unit) {
    // ns
    case 0: ui->STEPPING_UNIT->setText("ns"); ui->STEPPING_EDIT->setText(QString::number(stepping, 'f', 2)); break;
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
    ui->DELAY_SLIDER->setMaximum(max_dist);
}

void UserPanel::setup_max_dov(float max_dov)
{
    ui->GW_SLIDER->setMaximum(max_dov);
}

void UserPanel::update_delay_offset(float dist_offset)
{
//    ptr_tcu->delay_offset = dist_offset / dist_ns;
    emit send_double_tcu_msg(TCU::OFFSET_DELAY, dist_offset / dist_ns);
//    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist - dist_offset));
}

void UserPanel::update_gate_width_offset(float dov_offset)
{
//    ptr_tcu->gate_width_offset = dov_offset / dist_ns;
    emit send_double_tcu_msg(TCU::OFFSET_GW, dov_offset / dist_ns);
//    ui->GATE_WIDTH->setText(QString::asprintf("%.2f m", depth_of_view - dov_offset));
}

void UserPanel::update_laser_offset(float laser_offset)
{
//    ptr_tcu->laser_offset = laser_offset;
    emit send_double_tcu_msg(TCU::OFFSET_LASER, laser_offset);
}

// e.g. laser_on: 0b1110 -> laser[1], laser[2] and laser[3] are on, laser[0] is off
void UserPanel::setup_laser(int laser_on)
{
//    ptr_tcu->set_user_param(TCUThread::LASER_ON, laser_on);
    emit send_uint_tcu_msg(TCU::LASER_ON, laser_on);
}

void UserPanel::set_baudrate(int idx, int baudrate)
{
//    if (serial_port[idx]->isOpen()) serial_port[idx]->setBaudRate(baudrate);
//    switch (idx) {
//    case 0: ptr_tcu->set_baudrate(baudrate); break;
//    case 1: break;
//    case 2: ptr_lens->set_baudrate(baudrate); break;
//    case 3: ptr_laser->set_baudrate(baudrate); break;
//    case 4: ptr_ptz->set_baudrate(baudrate); break;
//    default: break;
//    }
    switch (idx) {
        case 0: p_tcu->set_baudrate(baudrate); break;
        case 1: break;
        case 2: p_lens->set_baudrate(baudrate); break;
        case 3: p_laser->set_baudrate(baudrate); break;
        case 4: p_ptz->set_baudrate(baudrate); break;
        default: break;
    }
}

void UserPanel::set_tcp_status_on_port(int idx, bool use_tcp)
{
//    switch (idx) {
//    case 0: ptr_tcu->set_tcp_status(use_tcp); break;
//    case 1: break;
//    case 2: ptr_lens->set_tcp_status(use_tcp); break;
//    case 3: ptr_laser->set_tcp_status(use_tcp); break;
//    case 4: ptr_ptz->set_tcp_status(use_tcp); break;
//    default: break;
//    }
    switch (idx) {
        case 0: p_tcu->set_use_tcp(use_tcp); break;
        case 1: break;
        case 2: p_lens->set_use_tcp(use_tcp); break;
        case 3: p_laser->set_use_tcp(use_tcp); break;
        case 4: p_ptz->set_use_tcp(use_tcp); break;
        default: break;
    }
}

void UserPanel::set_tcu_as_shared_port(bool share)
{
//    ptr_lens->share_port_from(share ? ptr_tcu : NULL);
    p_lens->share_port_from(share ? p_tcu : nullptr);
    p_ptz->share_port_from(share ? p_tcu : nullptr);
}

void UserPanel::com_write_data(int com_idx, QByteArray data)
{
#if 0
//    communicate_display(com_idx, data, data.length(), 0, true);
    ControlPortThread *temp_port = NULL;
    switch (com_idx) {
    case 0: temp_port = ptr_tcu; break;
    case 1: temp_port = ptr_lens; break;
//    case 2: temp_port = ptr_laser; break;
    case 3: temp_port = ptr_ptz; break;
    default: break;
    }
//    if (temp_port) temp_port->communicate_display(data, data.length(), 0, true);
    if (temp_port) temp_port->send_data(PortData{data, data.length(), 0, true, false});
#endif
    ControlPort *temp = nullptr;
    switch (com_idx)
    {
        case 0: temp = p_tcu; break;
        case 1: break;
        case 2: temp = p_lens; break;
        case 3: temp = p_laser; break;
        case 4: temp = p_ptz; break;
        default: break;
    }
    if (temp) temp->emit send_data(data, 0, 100);
}

void UserPanel::display_port_info(int idx)
{
//    if (serial_port[idx]->isOpen()) ui->TITLE->prog_settings->display_baudrate(idx, serial_port[idx]->baudRate());
//    if (serial_port[idx]->isOpen()) pref->display_baudrate(idx, serial_port[idx]->baudRate());
//    switch (idx) {
//    case 0:
//        pref->display_baudrate(idx, ptr_tcu->get_baudrate());
//        pref->ui->TCP_SERVER_CHK->setChecked(ptr_tcu->get_tcp_status());
//        break;
//    case 1: pref->display_baudrate(idx, 0); break;
//    case 2:
//        pref->display_baudrate(idx, ptr_lens->get_baudrate());
//        pref->ui->TCP_SERVER_CHK->setChecked(ptr_lens->get_tcp_status());
//        break;
//    case 3:
//        pref->display_baudrate(idx, ptr_laser->get_baudrate());
//        pref->ui->TCP_SERVER_CHK->setChecked(ptr_laser->get_tcp_status());
//        break;
//    case 4:
//        pref->display_baudrate(idx, ptr_ptz->get_baudrate());
//        pref->ui->TCP_SERVER_CHK->setChecked(ptr_ptz->get_tcp_status());
//        break;
//    default: break;
//    }

    ControlPort *cp = nullptr;
    switch (idx) {
    case 0: cp = p_tcu; break;
    case 1: pref->display_baudrate(idx, 0); return;
    case 2: cp = p_lens; break;
    case 3: cp = p_laser; break;
    case 4: cp = p_ptz; break;
    default: break;
    }
    if (cp) {
        pref->display_baudrate(idx, cp->get_baudrate());
        pref->ui->TCP_SERVER_CHK->setChecked(cp->get_use_tcp());
    }
}

void UserPanel::update_dev_ip()
{
//    ui->TITLE->prog_settings->enable_ip_editing(device_type == 1);
    pref->enable_ip_editing(device_type > 0 && pref->device_idx < curr_cam->gige_device_num());
    if (device_type > 0) {
        int ip, gateway, nic_address;
        curr_cam->ip_address(true, &ip, &gateway, &nic_address);
//        ui->TITLE->prog_settings->config_ip(true, ip, gateway);
        pref->config_ip(true, ip, gateway, nic_address);
    }
}

void UserPanel::set_dev_ip(int ip, int gateway)
{
    int static_ip = MV_IP_CFG_STATIC;
    curr_cam->ip_config(false, &static_ip);
    curr_cam->ip_address(false, &ip, &gateway);
    ui->ENUM_BUTTON->click();
}

void UserPanel::change_pixel_format(int pixel_format, int display_idx)
{
    bool was_playing = start_grabbing;
    if (was_playing) ui->STOP_GRABBING_BUTTON->click();

    this->pixel_format[0] = pixel_format;
    int ret = curr_cam->pixel_type(false, &pixel_format);
    switch (pixel_format) {
    case PixelType_Gvsp_Mono8:       is_color[display_idx] = false; update_pixel_depth( 8); break;
    case PixelType_Gvsp_Mono10:      is_color[display_idx] = false; update_pixel_depth(10); break;
    case PixelType_Gvsp_Mono12:      is_color[display_idx] = false; update_pixel_depth(12); break;
    case PixelType_Gvsp_BayerGB8:    is_color[display_idx] =  true; update_pixel_depth( 8); break;
    case PixelType_Gvsp_RGB8_Packed: is_color[display_idx] =  true; update_pixel_depth( 8); break;
    default:                         is_color[display_idx] = false; update_pixel_depth( 8); break;
    }

    if (was_playing) ui->START_GRABBING_BUTTON->click();
}

void UserPanel::update_lower_3d_thresh()
{
    ui->RANGE_THRESH_EDIT->setText(QString::number((int)(pref->lower_3d_thresh * (1 << pixel_depth[0]))));
}

void UserPanel::reset_custom_3d_params()
{
//    pref->ui->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(ptr_tcu->delay_a)));
//    pref->ui->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(ptr_tcu->gate_width_a)));
    pref->ui->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(p_tcu->get(TCU::DELAY_A))));
    pref->ui->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(p_tcu->get(TCU::GATE_WIDTH_A))));
}

struct frame_process_parameters {
    cv::VideoWriter *out;
    int enhance_option;
};

// TODO implement function to prompt for output filename and video conversion
void UserPanel::export_current_video()
{
    if (device_type != -2 || current_video_filename.isEmpty()) return;
    output_filename = QFileDialog::getSaveFileName(this, tr("Name the Output Video"), save_location,
                                                tr("MPEG-4 Video           (*.mp4);;\
                                                    Audio Video Interleaved(*.avi);;\
                                                    All Files              (*.*)"));
    if (output_filename.isEmpty()) { temp_output_filename.clear(); return; }

    srand(time(NULL));
    temp_output_filename = TEMP_SAVE_LOCATION + "/" + QString::number(rand(), 16) + ".mp4";
    vid_out_proc.open(temp_output_filename.toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), frame_rate_edit, cv::Size(w[0], h[0]), true);
    // TODO update enhance function parameters; add support for frame average
    int enhance_option = 0;
    if (ui->IMG_ENHANCE_CHECK->isChecked()) enhance_option = ui->ENHANCE_OPTIONS->currentIndex();
    struct frame_process_parameters *params = new struct frame_process_parameters;
    params->out = &vid_out_proc;
    params->enhance_option = enhance_option;
    load_video_file(current_video_filename, false,
        [](cv::Mat &frame, void *ptr){
            cv::Mat temp = frame.clone();
            switch (((struct frame_process_parameters*)ptr)->enhance_option) {
            case 1: ImageProc::hist_equalization(temp, temp); break;
            case 2: {
                cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, 0, 5, 0, 0, -1, 0);
                cv::filter2D(temp, temp, CV_8U, kernel);
                break;
            }
            case 3: ImageProc::plateau_equl_hist(temp, temp, 4); break;
            case 4: ImageProc::accumulative_enhance(temp, temp, 1); break;
            case 5: break;
            case 6:
                ImageProc::adaptive_enhance(temp, temp, 0, 0.05, 0, 1, 1.2); break;
            case 7:
                temp = ~temp;
                ImageProc::haze_removal(temp, temp, 7, 0.95, 0.1, 60, 0.01);
                temp = ~temp;
                break;
            case 8: ImageProc::haze_removal(temp, temp, 7, 0.95, 0.1, 60, 0.01); break;
            default: break;
            }
            cv::Mat frame_swapped;
            cv::cvtColor(temp, frame_swapped, cv::COLOR_RGB2BGR);
            ((struct frame_process_parameters*)ptr)->out->write(frame_swapped);
        }, params, 0, false);
}

void UserPanel::set_tcu_type(int idx)
{
    int diff = 0;
//    if (ptr_tcu->tcu_type == 0 && idx) diff = 1;
//    if (ptr_tcu->tcu_type && idx == 0) diff = -1;
//    ptr_tcu->tcu_type = idx;
    if (idx == 2) update_tcu_param_pos(1, ui->LASER_WIDTH_UNIT_U, ui->LASER_WIDTH_EDIT_N, ui->LASER_WIDTH_UNIT_N, ui->LASER_WIDTH_EDIT_P);
    if (p_tcu->type() == 2) update_tcu_param_pos(-1, ui->LASER_WIDTH_UNIT_U, ui->LASER_WIDTH_EDIT_N, ui->LASER_WIDTH_UNIT_N, ui->LASER_WIDTH_EDIT_P);

    if (p_tcu->type() == 0 && idx) diff = 1;
    if (p_tcu->type() && idx == 0) diff = -1;
    p_tcu->set_type(idx);
    ui->DELAY_A->resize(QSize(ui->DELAY_A->width() + 12 * diff, ui->DELAY_A->height()));
    ui->DELAY_B->resize(QSize(ui->DELAY_B->width() + 12 * diff, ui->DELAY_B->height()));
    ui->DELAY_GRP->move(idx ? -6 : 0, 100);
    ui->GATE_WIDTH_A->resize(QSize(ui->GATE_WIDTH_A->width() + 12 * diff, ui->GATE_WIDTH_A->height()));
    ui->GATE_WIDTH_B->resize(QSize(ui->GATE_WIDTH_B->width() + 12 * diff, ui->GATE_WIDTH_B->height()));
    ui->GATE_WIDTH_GRP->move(idx ? -6 : 0, 180);
    if (diff) update_tcu_param_pos(diff, ui->DELAY_A_UNIT_U, ui->DELAY_A_EDIT_N, ui->DELAY_A_UNIT_N, ui->DELAY_A_EDIT_P);
    if (diff) update_tcu_param_pos(diff, ui->DELAY_B_UNIT_U, ui->DELAY_B_EDIT_N, ui->DELAY_B_UNIT_N, ui->DELAY_B_EDIT_P);
    if (diff) update_tcu_param_pos(diff, ui->GATE_WIDTH_A_UNIT_U, ui->GATE_WIDTH_A_EDIT_N, ui->GATE_WIDTH_A_UNIT_N, ui->GATE_WIDTH_A_EDIT_P);
    if (diff) update_tcu_param_pos(diff, ui->GATE_WIDTH_B_UNIT_U, ui->GATE_WIDTH_B_EDIT_N, ui->GATE_WIDTH_B_UNIT_N, ui->GATE_WIDTH_B_EDIT_P);

    switch (idx) {
    case 0: mcp_max = 255; break;
    case 1: mcp_max = 4095; break;
    case 2: mcp_max = 4095; break;
    default: break;
    }
    ui->MCP_SLIDER->setMaximum(mcp_max);
}

void UserPanel::update_ps_config(bool read, int idx, uint val)
{
//    if (read) {
//        pref->ui->PS_STEPPING_EDT->setText(QString::number(ptr_tcu->ps_step[idx]));
//        pref->ui->MAX_PS_STEP_EDT->setText(QString::number(int(std::round(4000. / ptr_tcu->ps_step[idx]))));
//    }
//    else ptr_tcu->ps_step[idx] = val;

    if (read) {
        pref->ui->PS_STEPPING_EDT->setText(QString::number(p_tcu->get(TCU::PS_STEP_1 + idx)));
        pref->ui->MAX_PS_STEP_EDT->setText(QString::number(int(std::round(4000. / p_tcu->get(TCU::PS_STEP_1 + idx)))));
    }
    else emit send_uint_tcu_msg(TCU::PS_STEP_1 + idx, val);
}

void UserPanel::set_auto_mcp(bool auto_mcp)
{
    this->auto_mcp = auto_mcp;
    ui->AUTO_MCP_CHK->setChecked(auto_mcp);
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
        else if (joybtn_A) on_IRIS_OPEN_BTN_pressed(),     lens_adjust_ongoing = 0b100;
        break;
    case JoystickThread::BUTTON::R1: joybtn_R1 = true; break;
    case JoystickThread::BUTTON::R2:
        joybtn_R2 = true;
        if      (joybtn_X) on_ZOOM_OUT_BTN_pressed(),       lens_adjust_ongoing = 0b001;
        else if (joybtn_Y) on_FOCUS_FAR_BTN_pressed(),      lens_adjust_ongoing = 0b010;
        else if (joybtn_A) on_IRIS_CLOSE_BTN_pressed(),     lens_adjust_ongoing = 0b100;
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

void UserPanel::update_port_status(ControlPort *port, QLabel *lbl)
{
    qint32 port_status = port->get_port_status();
    QString style = QString(port_status & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;") +
                    QString(port_status & ControlPort::TCP_CONNECTED ? "text-decoration:underline;" : "");
    lbl->setStyleSheet(style);
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

void UserPanel::update_lens_params(qint32 lens_param, uint val)
{
    switch (lens_param)
    {
        case Lens::ZOOM_POS:  if (!ui->ZOOM_EDIT->hasFocus())  ui->ZOOM_EDIT->setText(QString::number(val)); break;
        case Lens::FOCUS_POS: if (!ui->FOCUS_EDIT->hasFocus()) ui->FOCUS_EDIT->setText(QString::number(val)); break;
        case Lens::LASER_RADIUS: qDebug() << "laser radius" << val; break;
        case Lens::NO_PARAM:
            update_lens_params(Lens::ZOOM_POS, p_lens->get(Lens::ZOOM_POS));
            update_lens_params(Lens::FOCUS_POS, p_lens->get(Lens::FOCUS_POS));
            ui->FOCUS_SPEED_SLIDER->setValue(std::round(p_lens->get(Lens::STEPPING)));
        default:break;
    }
}

void UserPanel::update_ptz_params(qint32 ptz_param, double val)
{
    switch (ptz_param)
    {
        case PTZ::ANGLE_H: if (!ui->ANGLE_H_EDIT->hasFocus()) ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", val)); break;
        case PTZ::ANGLE_V: if (!ui->ANGLE_V_EDIT->hasFocus()) ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", val)); break;
        case PTZ::NO_PARAM:
            update_ptz_params(PTZ::ANGLE_H, p_ptz->get(PTZ::ANGLE_H));
            update_ptz_params(PTZ::ANGLE_V, p_ptz->get(PTZ::ANGLE_V));
            ui->PTZ_SPEED_SLIDER->setValue(std::round(p_ptz->get(PTZ::SPEED)));
            break;
        default:break;
    }
}

void UserPanel::update_distance(double distance)
{
    ui->DISTANCE->setText(QString::asprintf("%.2f m", distance));
}

void UserPanel::update_usbcan_angle(float _h, float _v)
{
    // Ensure horizontal angle is always positive (0 to 360)
    float display_h = _h < 0 ? _h + 360.0f : _h;
    
    if (!ui->ANGLE_H_EDIT->hasFocus()) ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", display_h));
    if (!ui->ANGLE_V_EDIT->hasFocus()) ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", _v));
}

void UserPanel::set_distance_set(int id)
{
    aliasing_mode = true;
    struct AliasingData temp = aliasing->retrieve_data(id);
    aliasing_level = temp.num_period;
//    ptr_tcu->set_user_param(TCUThread::REPEATED_FREQ, temp.rep_freq);
    emit send_double_tcu_msg(TCU::REPEATED_FREQ, temp.rep_freq);
//    setup_hz(hz_unit);
    if (pref->ui->AUTO_REP_FREQ_CHK->isChecked()) pref->ui->AUTO_REP_FREQ_CHK->click();
//    ptr_tcu->delay_dist = temp.distance;
    emit send_double_tcu_msg(TCU::EST_DIST, temp.rep_freq);
    update_delay();
}

void UserPanel::switch_ui()
{
    simple_ui ^= 1;
    if (simple_ui) {
        // TCU group
        ui->TCU_STATIC->resize(ui->TCU_STATIC->size() - QSize(0, 40));
        ui->SWITCH_TCU_UI_BTN->hide();
        ui->PRF_GRP->move(ui->PRF_GRP->pos() + QPoint(10, 0));
        ui->LASER_WIDTH_GRP->hide();
        ui->AUTO_MCP_CHK->show();
        ui->MCP_GRP->move(ui->MCP_GRP->pos() + QPoint(10, 0));
        ui->MCP_SLIDER->resize(ui->MCP_SLIDER->size() - QSize(10, 0));
        ui->MCP_EDIT->move(ui->MCP_EDIT->pos() - QPoint(10, 0));
        ui->DELAY_GRP->hide();
        ui->GATE_WIDTH_GRP->hide();
        ui->DELAY_SLIDER->move(ui->DELAY_SLIDER->pos() - QPoint(0, 37));
        ui->ESTIMATED->move(ui->ESTIMATED->pos() - QPoint(0, 40));
        ui->EST_DIST->move(ui->EST_DIST->pos() - QPoint(0, 40));
        ui->GW_SLIDER->show();
        ui->ESTIMATED_2->move(ui->ESTIMATED_2->pos() - QPoint(0, 74));
        ui->GATE_WIDTH->move(ui->GATE_WIDTH->pos() - QPoint(0, 74));
        ui->STEPPING->hide();
        ui->STEPPING_EDIT->hide();
        ui->STEPPING_UNIT->hide();
        ui->SIMPLE_LASER_CHK->show();

        // lens group
        ui->LENS_STATIC->setGeometry(QRect(ui->LENS_STATIC->geometry()).adjusted(0, -40, 0, 0));
        ui->ZOOM_EDIT->hide();
        ui->FOCUS_EDIT->hide();
        ui->GET_LENS_PARAM_BTN->hide();
        ui->AUTO_FOCUS_BTN->hide();
        ui->FOCUS_SPEED_LABEL->show();
        ui->FOCUS_SPEED_SLIDER->move(ui->FOCUS_SPEED_SLIDER->pos() + QPoint(20, 10));
        ui->FOCUS_SPEED_EDIT->move(ui->FOCUS_SPEED_EDIT->pos() + QPoint(-26, 35));
        ui->ZOOM_IN_BTN->setGeometry(QRect(ui->ZOOM_IN_BTN->geometry()).adjusted(10, -17, 14, -13));
        ui->ZOOM_OUT_BTN->setGeometry(QRect(ui->ZOOM_OUT_BTN->geometry()).adjusted(75, -17, 79, -13));
        ui->ZOOM_LABEL->move(ui->ZOOM_LABEL->pos() + QPoint(44, 5));
        ui->FOCUS_NEAR_BTN->setGeometry(QRect(ui->FOCUS_NEAR_BTN->geometry()).adjusted(-50, 23, -46, 27));
        ui->FOCUS_FAR_BTN->setGeometry(QRect(ui->FOCUS_FAR_BTN->geometry()).adjusted(15, 23, 19, 27));
        ui->FOCUS_LABEL->move(ui->FOCUS_LABEL->pos() + QPoint(-16, 45));
        ui->RADIUS_INC_BTN->show();
        ui->RADIUS_DEC_BTN->show();
        ui->LASER_RADIUS_LABEL->show();
        if (lang) {
            ui->ZOOM_IN_BTN->setText(tr("IN"));
            ui->ZOOM_OUT_BTN->setText(tr("OUT"));
            ui->FOCUS_NEAR_BTN->setText(tr("NEAR"));
            ui->FOCUS_FAR_BTN->setText(tr("FAR"));
            ui->RADIUS_INC_BTN->setText(tr("INC"));
            ui->RADIUS_DEC_BTN->setText(tr("DEC"));
        }

        // img-proc group
        ui->IMG_PROC_STATIC->setGeometry(QRect(ui->IMG_PROC_STATIC->geometry()).adjusted(0, 0, 0, 10));
        ui->IMG_ENHANCE_CHECK->move(ui->IMG_ENHANCE_CHECK->pos() + QPoint(10, 5));
        ui->ENHANCE_OPTIONS->setCurrentIndex(7);
        ui->ENHANCE_OPTIONS->hide();
        ui->FRAME_AVG_CHECK->move(ui->FRAME_AVG_CHECK->pos() + QPoint(10, -10));
        ui->FRAME_AVG_OPTIONS->hide();
        ui->IMG_3D_CHECK->move(ui->IMG_3D_CHECK->pos() + QPoint(10, 0));
        ui->RANGE_THRESH_EDIT->move(ui->RANGE_THRESH_EDIT->pos() + QPoint(10, 0));
        ui->RESET_3D_BTN->hide();
//        ui->BRIGHTNESS_LABEL->hide();
//        ui->BRIGHTNESS_SLIDER->hide();
//        ui->PSEUDOCOLOR_CHK->show();
        ui->PSEUDOCOLOR_CHK->move(ui->PSEUDOCOLOR_CHK->pos() + QPoint(0, 5));
        ui->FRAME_AVG_CHECK->setText("Filter");

        ui->SCAN_GRP->hide();
//        ui->LOGO->show();
    }
    else {
        // TCU group
        ui->TCU_STATIC->resize(ui->TCU_STATIC->size() + QSize(0, 40));
        ui->SWITCH_TCU_UI_BTN->show();
        ui->PRF_GRP->move(ui->PRF_GRP->pos() - QPoint(10, 0));
        ui->LASER_WIDTH_GRP->show();
        ui->AUTO_MCP_CHK->hide();
        ui->MCP_GRP->move(ui->MCP_GRP->pos() - QPoint(10, 0));
        ui->MCP_SLIDER->resize(ui->MCP_SLIDER->size() + QSize(10, 0));
        ui->MCP_EDIT->move(ui->MCP_EDIT->pos() + QPoint(10, 0));
        ui->DELAY_GRP->show();
        ui->GATE_WIDTH_GRP->show();
        ui->DELAY_SLIDER->move(ui->DELAY_SLIDER->pos() + QPoint(0, 37));
        ui->ESTIMATED->move(ui->ESTIMATED->pos() + QPoint(0, 40));
        ui->EST_DIST->move(ui->EST_DIST->pos() + QPoint(0, 40));
        ui->GW_SLIDER->hide();
        ui->ESTIMATED_2->move(ui->ESTIMATED_2->pos() + QPoint(0, 74));
        ui->GATE_WIDTH->move(ui->GATE_WIDTH->pos() + QPoint(0, 74));
        ui->STEPPING->show();
        ui->STEPPING_EDIT->show();
        ui->STEPPING_UNIT->show();
        ui->SIMPLE_LASER_CHK->hide();

        // lens group
        ui->LENS_STATIC->setGeometry(QRect(ui->LENS_STATIC->geometry()).adjusted(0, 40, 0, 0));
        ui->ZOOM_EDIT->show();
        ui->FOCUS_EDIT->show();
        ui->GET_LENS_PARAM_BTN->show();
        ui->AUTO_FOCUS_BTN->show();
        ui->FOCUS_SPEED_LABEL->hide();
        ui->FOCUS_SPEED_SLIDER->move(ui->FOCUS_SPEED_SLIDER->pos() - QPoint(20, 10));
        ui->FOCUS_SPEED_EDIT->move(ui->FOCUS_SPEED_EDIT->pos() - QPoint(-26, 35));
        ui->ZOOM_IN_BTN->setGeometry(QRect(ui->ZOOM_IN_BTN->geometry()).adjusted(-10, 17, -14, 13));
        ui->ZOOM_OUT_BTN->setGeometry(QRect(ui->ZOOM_OUT_BTN->geometry()).adjusted(-75, 17, -79, 13));
        ui->ZOOM_LABEL->move(ui->ZOOM_LABEL->pos() - QPoint(44, 5));
        ui->FOCUS_NEAR_BTN->setGeometry(QRect(ui->FOCUS_NEAR_BTN->geometry()).adjusted(50, -23, 46, -27));
        ui->FOCUS_FAR_BTN->setGeometry(QRect(ui->FOCUS_FAR_BTN->geometry()).adjusted(-15, -23, -19, -27));
        ui->FOCUS_LABEL->move(ui->FOCUS_LABEL->pos() - QPoint(-16, 45));
        ui->RADIUS_INC_BTN->hide();
        ui->RADIUS_DEC_BTN->hide();
        ui->LASER_RADIUS_LABEL->hide();
        ui->ZOOM_IN_BTN->setText("+");
        ui->ZOOM_OUT_BTN->setText("-");
        ui->FOCUS_NEAR_BTN->setText("+");
        ui->FOCUS_FAR_BTN->setText("-");
        ui->RADIUS_INC_BTN->setText("+");
        ui->RADIUS_DEC_BTN->setText("-");

        // img-proc group
        ui->IMG_PROC_STATIC->setGeometry(QRect(ui->IMG_PROC_STATIC->geometry()).adjusted(0, 0, 0, -10));
        ui->IMG_ENHANCE_CHECK->move(ui->IMG_ENHANCE_CHECK->pos() - QPoint(10, 5));
        ui->ENHANCE_OPTIONS->setCurrentIndex(0);
        ui->ENHANCE_OPTIONS->show();
        ui->FRAME_AVG_CHECK->move(ui->FRAME_AVG_CHECK->pos() - QPoint(10, -10));
        ui->FRAME_AVG_OPTIONS->show();
        ui->IMG_3D_CHECK->move(ui->IMG_3D_CHECK->pos() - QPoint(10, 0));
        ui->RANGE_THRESH_EDIT->move(ui->RANGE_THRESH_EDIT->pos() - QPoint(10, 0));
        ui->RESET_3D_BTN->show();
//        ui->BRIGHTNESS_LABEL->show();
//        ui->BRIGHTNESS_SLIDER->show();
//        ui->PSEUDOCOLOR_CHK->hide();
        ui->PSEUDOCOLOR_CHK->move(ui->PSEUDOCOLOR_CHK->pos() - QPoint(0, 5));
        ui->FRAME_AVG_CHECK->setText("Avg");

        ui->SCAN_GRP->show();
//        ui->LOGO->hide();
    }
}

// FIXME update config i/o
void UserPanel::save_config_to_file()
{
//    QString config_name = QFileDialog::getSaveFileName(this, tr("Save Configuration"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/config.ssy", tr("*.ssy"));
    QString config_name = QFileDialog::getSaveFileName(this, tr("Save Configuration"), save_location + "/config.ssy", tr("YJS config(*.ssy);;All Files()"));
    if (config_name.isEmpty()) return;
    if (!config_name.endsWith(".ssy")) config_name.append(".ssy");
    QFile config(config_name);
    if (!config.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "PROMPT", tr("cannot create config file"));
        return;
    }
    
    QDataStream out(&config);
    convert_write(out, WIN_PREF);
    convert_write(out, TCU_PARAMS);
    convert_write(out, SCAN);
    convert_write(out, IMG);
    convert_write(out, TCU_PREF);
    config.close();
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
    if (!config.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "PROMPT", tr("cannot open config file"));
        return;
    }
    
    bool read_success = true;
    QDataStream out(&config);
    read_success &= convert_read(out, WIN_PREF);
    read_success &= convert_read(out, TCU_PARAMS);
    read_success &= convert_read(out, SCAN);
    read_success &= convert_read(out, IMG);
    read_success &= convert_read(out, TCU_PREF);
    config.close();
    if (read_success) {
//        data_exchange(false);
        update_delay();
        update_gate_width();
//        ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//        ptr_tcu->set_user_param(TCU::LASER_WIDTH, ptr_tcu->laser_width);
//        ptr_tcu->set_user_param(TCUThread::LASER_USR, ptr_tcu->laser_width);
        emit send_double_tcu_msg(TCU::LASER_USR, p_tcu->get(TCU::LASER_WIDTH));
//        ui->MCP_SLIDER->setValue(ptr_tcu->mcp);
        ui->MCP_SLIDER->setValue(std::round(p_tcu->get(TCU::MCP)));
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

void UserPanel::generate_preset_data(int idx)
{
#ifdef DEPRECATED_PRESET_STRUCT
    // move attribute_get to function
    PresetData preset_data{0};
    memset(&preset_data, 0, sizeof(PresetData));
    preset_data.rep_freq     = p_tcu->get(TCU::REPEATED_FREQ);
    preset_data.laser_width  = p_tcu->get(TCU::LASER_WIDTH);
    preset_data.delay_a      = p_tcu->get(TCU::DELAY_A);
    preset_data.gw_a         = p_tcu->get(TCU::GATE_WIDTH_A);
    preset_data.delay_b      = p_tcu->get(TCU::DELAY_B);
    preset_data.gw_b         = p_tcu->get(TCU::GATE_WIDTH_B);
    preset_data.ccd_freq     = p_tcu->get(TCU::CCD_FREQ);
    preset_data.duty         = p_tcu->get(TCU::DUTY);
    preset_data.mcp          = p_tcu->get(TCU::MCP);
    preset_data.delay_n      = p_tcu->get(TCU::DELAY_N);
    preset_data.gatewidth_n  = p_tcu->get(TCU::GATE_WIDTH_N);
    preset_data.zoom         = p_lens->get(Lens::ZOOM_POS);
    preset_data.focus        = p_lens->get(Lens::FOCUS_POS);
    preset_data.laser_radius = p_lens->get(Lens::LASER_RADIUS);
    preset_data.lens_speed   = p_lens->get(Lens::STEPPING);
    preset_data.angle_h      = p_ptz->get(PTZ::ANGLE_H);
    preset_data.angle_v      = p_ptz->get(PTZ::ANGLE_V);
    preset_data.ptz_speed    = p_ptz->get(PTZ::SPEED);
    emit save_to_preset(idx, preset_data);
#endif

//    nlohmann::json json_tcu;
//    json_tcu["TCU"] = p_tcu->to_json();
//    nlohmann::json json_lens;
//    json_lens["Lens"] = p_lens->to_json();
//    nlohmann::json json_ptz;
//    json_ptz["PTZ"] = p_ptz->to_json();
//    emit save_to_preset(idx, nlohmann::json::array({json_tcu, json_lens, json_ptz}));
    emit save_to_preset(idx, nlohmann::json{{"TCU", p_tcu->to_json()}, {"Lens", p_lens->to_json()}, {"PTZ", p_ptz->to_json()}});
}

#if DEPRECATED_PRESET_STRUCT
void UserPanel::apply_preset(PresetData preset)
{
    delay_dist = preset.delay_a * dist_ns;
//    ptr_tcu->delay_n = preset.delay_n;
    emit send_double_tcu_msg(TCU::DELAY_N, preset.delay_n);
    update_delay();
    depth_of_view = preset.gw_a * dist_ns;
//    ptr_tcu->gate_width_n = preset.gatewidth_n;
    emit send_double_tcu_msg(TCU::GATE_WIDTH_N, preset.gatewidth_n);
    update_gate_width();
    change_mcp(preset.mcp);
    ui->ZOOM_EDIT->setText(QString::number(zoom = preset.zoom));
    ui->FOCUS_EDIT->setText(QString::number(focus = preset.focus));
    set_zoom();
    set_focus();
    change_focus_speed(preset.lens_speed);
    angle_h = preset.angle_h;
    angle_v = preset.angle_v;
    set_ptz_angle();
    ui->PTZ_SPEED_SLIDER->setValue(preset.ptz_speed);
}
#endif

void UserPanel::apply_preset(nlohmann::json preset_data)
{
    if (preset_data.contains("TCU") && preset_data.contains("Lens") && preset_data.contains("PTZ")) {
        p_tcu->emit load_json(preset_data["TCU"]);
        p_lens->emit load_json(preset_data["Lens"]);
        p_ptz->emit load_json(preset_data["PTZ"]);
    }
}

void UserPanel::prompt_for_serial_file()
{
    QString serial_filename = QFileDialog::getOpenFileName(this, tr("Load SN Config"), save_location, tr("(*.csv);;All Files()"));
    config_gatewidth(serial_filename);
}

void UserPanel::prompt_for_input_file()
{
    bool sp_mode = QApplication::keyboardModifiers() & Qt::ShiftModifier;

    QDialog input_file_dialog(this, Qt::FramelessWindowHint);
    input_file_dialog.resize(400, 80);
    QGridLayout grid(&input_file_dialog);
    grid.setColumnStretch(0, 1);
    grid.setColumnStretch(1, 2);
    grid.setColumnStretch(2, 1);
//    grid.setRowWrapPolicy(QFormLayout::WrapLongRows);

//    use_gray_format->setChecked(true);
//    QLineEdit *file_path = new QLineEdit("rtsp://192.168.2.100/mainstream", &input_file_dialog);
    grid.addWidget(new QLabel("video URL:", &input_file_dialog), 0, 0);

    QComboBox *file_path_x = new QComboBox(&input_file_dialog);
    static QStringList file_paths = (QStringList() << "rtsp://192.168.2.xxx/mainstream"
                                     << "rtsp://admin:abcd1234@192.168.1.xxx:554/h264/ch1/main/av_stream"
                                     << "rtsp://admin:abcd1234@192.168.1.xxx:554");
    static QStringList sp_paths = (QStringList() << "https://mst3k-localnow.amagi.tv/playlist.m3u8"
                                   << "http://109.233.89.170/Disney_Channel/mono.m3u8"
                                   << "https://travelxp-travelxp-1-nz.samsung.wurl.tv/playlist.m3u8"
                                   << "https://aegis-cloudfront-1.tubi.video/dc8bda97-ce9e-4091-b4e8-11254dea4da6/playlist.m3u8");
    file_path_x->addItems(file_paths);
    if (sp_mode) file_path_x->addItems(sp_paths);
    file_path_x->setEditable(true);
    grid.addWidget(file_path_x, 1, 0, 1, 3);

    QCheckBox *use_gray_format = new QCheckBox("gray", &input_file_dialog);
    use_gray_format->setEnabled(sp_mode);
    grid.addWidget(use_gray_format, 2, 0);

    QLabel *spacer = new QLabel("", &input_file_dialog);
    grid.addWidget(spacer, 2, 1);

    QComboBox *display_list = new QComboBox(&input_file_dialog);
    static QStringList display_widgets = (QStringList() << "main display" << "alt 1" << "alt 2");
    display_list->addItems(display_widgets);
    display_list->setCurrentIndex(1);
    grid.addWidget(display_list, 2, 2);

    QDialogButtonBox button_box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &input_file_dialog);
    grid.addWidget(&button_box, 3, 2);
    QObject::connect(&button_box, SIGNAL(accepted()), &input_file_dialog, SLOT(accept()));
    QObject::connect(&button_box, SIGNAL(rejected()), &input_file_dialog, SLOT(reject()));

    input_file_dialog.setModal(true);
    button_box.button(QDialogButtonBox::Ok)->setFocus();
    button_box.button(QDialogButtonBox::Ok)->setDefault(true);

    // Process when OK button is clicked
    if (input_file_dialog.exec() == QDialog::Accepted)
        load_video_file(file_path_x->currentText(), use_gray_format->isChecked(),
                        [](cv::Mat &frame, void* ptr){ (*(std::queue<cv::Mat>*)ptr).push(frame.clone()); },
            &(this->q_img[display_list->currentIndex()]), display_list->currentIndex());
}

void UserPanel::search_for_devices()
{
    ui->ENUM_BUTTON->click();
}

void UserPanel::update_light_speed(bool uw)
{
    c = uw ? 3e8 * 0.75 : 3e8;
    scan_config->dist_ns = pref->dist_ns = dist_ns = c * 1e-9 / 2;
    pref->update_distance_display();
    emit send_double_tcu_msg(TCU::LIGHT_SPEED, dist_ns);
    update_delay();
    update_gate_width();
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
        qint64 bytes_written = serial_port[id]->write(write, write_size);
        if (bytes_written == -1) {
            qWarning("Serial port write error on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }
        
        int write_retries = 5; // 5 * 10ms = 50ms max
        while (serial_port[id]->waitForBytesWritten(10) && --write_retries > 0 && serial_port[id]->isOpen()) ;

        if (fb && serial_port[id]->isOpen()) {
            int read_retries = 2; // 2 * 100ms = 200ms max  
            while (serial_port[id]->waitForReadyRead(100) && --read_retries > 0 && serial_port[id]->isOpen()) ;
        }

        if (!serial_port[id]->isOpen()) {
            qWarning("Serial port disconnected during communication on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }

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
        qint64 bytes_written = tcp_port[id]->write(write, write_size);
        if (bytes_written == -1) {
            qWarning("TCP port write error on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }
        
        int write_retries = 5; // 5 * 10ms = 50ms max
        while (tcp_port[id]->waitForBytesWritten(10) && --write_retries > 0 && tcp_port[id]->isOpen()) ;

        if (fb && tcp_port[id]->isOpen()) {
            int read_retries = 2; // 2 * 100ms = 200ms max
            while (tcp_port[id]->waitForReadyRead(100) && --read_retries > 0 && tcp_port[id]->isOpen()) ;
        }

        if (!tcp_port[id]->isOpen()) {
            qWarning("TCP port disconnected during communication on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }

        read = read_size ? tcp_port[id]->read(read_size) : tcp_port[id]->readAll();
        for (char i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
        emit append_text(str_r);
    }

    port_mutex.unlock();
//    image_mutex.unlock();
    QThread().msleep(10);
    return read;
}

void UserPanel::update_laser_width()
{
#ifndef LVTONG
    static QElapsedTimer t;
    if (t.elapsed() < 40) return;
    t.start();
#endif
    if (laser_width < 0) laser_width = 0;
    if (laser_width > pref->max_laser_width) laser_width = pref->max_laser_width;

//    ptr_tcu->set_user_param(TCU::LASER_WIDTH, laser_width);
//    ptr_tcu->set_user_param(TCUThread::LASER_USR, laser_width);
    emit send_double_tcu_msg(TCU::LASER_USR, laser_width);
}

void UserPanel::update_delay()
{
//    qDebug() << sender();
    static QElapsedTimer t;
    if (t.isValid() && t.elapsed() < 40) return;
    t.restart();

    // REPEATED FREQUENCY
    // FIXME check if delay_dist + offset is valid
    if (delay_dist < 0) delay_dist = 0;
    if (delay_dist > pref->max_dist) delay_dist = pref->max_dist;
//    qDebug("estimated distance: %f\n", delay_dist);

    if (!aliasing_mode) {
//        ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist - ptr_tcu->delay_offset * dist_ns));
//        ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist));
        if (!qobject_cast<QSlider*>(sender())) ui->DELAY_SLIDER->setValue(delay_dist);

//        ptr_tcu->set_user_param(TCUThread::EST_DIST, delay_dist);
        emit send_double_tcu_msg(TCU::EST_DIST, delay_dist);
    }
    else {
        aliasing_mode = false;
//        ptr_tcu->set_user_param(TCUThread::EST_DIST, ptr_tcu->delay_dist);
        emit send_double_tcu_msg(TCU::EST_DIST, p_tcu->get(TCU::EST_DIST));
    }
}

void UserPanel::update_gate_width() {
#ifndef LVTONG
    static QElapsedTimer t;
    if (t.isValid() && t.elapsed() < 40) return;
    t.restart();
#endif
    if (depth_of_view < 0) depth_of_view = 0;
    if (depth_of_view > pref->max_dov) depth_of_view = pref->max_dov;

//    if (ptr_tcu->set_user_param(TCUThread::EST_DOV, depth_of_view) == -1) {
//        depth_of_view = ptr_tcu->gate_width_a * dist_ns;
//        ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf("%d", get_width_in_us(ptr_tcu->gate_width_a)));
//        ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%03d", get_width_in_ns(ptr_tcu->gate_width_a)));
//        ui->GATE_WIDTH_A_EDIT_P->setText(QString::asprintf("%03d", get_width_in_ps(ptr_tcu->gate_width_a, 0)));
//        ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf("%d", get_width_in_us(ptr_tcu->gate_width_b)));
//        ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%03d", get_width_in_ns(ptr_tcu->gate_width_b)));
//        ui->GATE_WIDTH_B_EDIT_P->setText(QString::asprintf("%03d", get_width_in_ps(ptr_tcu->gate_width_b, 2)));
//        QMessageBox::warning(this, "PROMPT", tr("gatewidth not supported"));
//    }

    if (!qobject_cast<QSlider*>(sender())) ui->GW_SLIDER->setValue(depth_of_view);

    emit send_double_tcu_msg(TCU::EST_DOV, depth_of_view);
}

void UserPanel::update_tcu_params(qint32 tcu_param)
{
    uint us, ns, ps;
    switch (tcu_param)
    {
        case TCU::LASER_USR:
            split_value_by_unit(p_tcu->get(TCU::LASER_WIDTH), us, ns, ps);
            ui->LASER_WIDTH_EDIT_U->setText(QString::asprintf(  "%d", us));
            ui->LASER_WIDTH_EDIT_N->setText(QString::asprintf("%03d", ns));
            ui->LASER_WIDTH_EDIT_P->setText(QString::asprintf("%03d", ps));
            break;
        case TCU::EST_DIST:
            ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist = p_tcu->get(TCU::EST_DIST)));
            rep_freq = p_tcu->get(TCU::REPEATED_FREQ);
            setup_hz(hz_unit);
            split_value_by_unit(p_tcu->get(TCU::DELAY_A), us, ns, ps, 1);
            ui->DELAY_A_EDIT_U->setText(QString::asprintf(  "%d", us));
            ui->DELAY_A_EDIT_N->setText(QString::asprintf("%03d", ns));
            ui->DELAY_A_EDIT_P->setText(QString::asprintf("%03d", ps));
            split_value_by_unit(p_tcu->get(TCU::DELAY_B), us, ns, ps, 1);
            ui->DELAY_B_EDIT_U->setText(QString::asprintf(  "%d", us));
            ui->DELAY_B_EDIT_N->setText(QString::asprintf("%03d", ns));
            ui->DELAY_B_EDIT_P->setText(QString::asprintf("%03d", ps));
            ui->DELAY_SLIDER->setValue(delay_dist);
            break;
        case TCU::EST_DOV:
            ui->GATE_WIDTH->setText(QString::asprintf("%.2f m", depth_of_view = p_tcu->get(TCU::EST_DOV)));
            split_value_by_unit(p_tcu->get(TCU::GATE_WIDTH_A), us, ns, ps, 0);
            ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf(  "%d", us));
            ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%03d", ns));
            ui->GATE_WIDTH_A_EDIT_P->setText(QString::asprintf("%03d", ps));
            split_value_by_unit(p_tcu->get(TCU::GATE_WIDTH_B), us, ns, ps, 0);
            ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf(  "%d", us));
            ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%03d", ns));
            ui->GATE_WIDTH_B_EDIT_P->setText(QString::asprintf("%03d", ps));
            ui->GW_SLIDER->setValue(depth_of_view);
            break;
        case TCU::NO_PARAM:
            update_tcu_params(TCU::LASER_USR);
            update_tcu_params(TCU::EST_DIST);
            update_tcu_params(TCU::EST_DOV);
            ui->MCP_SLIDER->setValue(p_tcu->get(TCU::MCP));
        default: break;
    }
}

void UserPanel::filter_scan()
{
    static cv::Mat filter = img_mem[0].clone();
    filter = img_mem[0] / 64;
//    qDebug("ratio %f\n", cv::countNonZero(filter) / (float)filter.total());
//    if (cv::countNonZero(filter) / (float)filter.total() > 0.3) /*scan = !scan, *//*emit update_scan(true);*/q_scan.push_back(ptr_tcu->get_tcu_params());
}

void UserPanel::auto_scan_for_target()
{
    rep_freq = 30;
    delay_dist = 1000;
    scan_stopping_delay = 6400;
    scan_step = 100;
    depth_of_view = 150;
}

void UserPanel::update_current()
{
//    if (!serial_port[3]) return;
//    QString send = "DIOD1:CURR " + ui->CURRENT_EDIT->text() + "\r";
    QString send = QString::asprintf("PCUR %.2f\r", ui->CURRENT_EDIT->text().toFloat());
//    serial_port[3]->write(send.toLatin1().data(), send.length());
//    serial_port[3]->waitForBytesWritten(100);
//    serial_port[3]->readAll();
//    ptr_laser->communicate_display(send.toLatin1(), send.length(), 0, true, false);
//    ptr_laser->send_data(PortData{send.toLatin1(), send.length(), 0, true, false});
//    p_laser->emit send_data(send.toLatin1(), 0, 100);
    emit send_laser_msg(send);
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
//        out << ptr_tcu->ccd_freq << ptr_tcu->duty << ptr_tcu->rep_freq << ptr_tcu->laser_width << ptr_tcu->mcp << delay_dist << ptr_tcu->delay_n << depth_of_view << stepping;
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
//        out >> ptr_tcu->ccd_freq >> ptr_tcu->duty >> ptr_tcu->rep_freq >> ptr_tcu->laser_width >> ptr_tcu->mcp >> delay_dist >> ptr_tcu->delay_n >> depth_of_view >> stepping;
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
//        ptr_tcu->gw_lut[line_sep[0].toInt()] = line_sep[1].toInt();
    }
}

void UserPanel::connect_to_serial_server_tcp()
{
//    QString ip = "192.168.1.233";
//    bool connected = false;
//    tcp_port[0]->connectToHost(ip, 10001);
//    tcp_port[2]->connectToHost(ip, 10002);
//    connected |= tcp_port[0]->waitForConnected(3000);
//    connected |= tcp_port[2]->waitForConnected(3000);
//    tcp_port[4] = tcp_port[2];
//    for (int i = 0; i < 5; i++) use_tcp[i] = connected;
//    connected |= ptr_tcu->  setup_tcp_port(pref->ui->TCP_SERVER_IP_EDIT->text(), 10001, true);
//    connected |= ptr_lens-> setup_tcp_port(pref->ui->TCP_SERVER_IP_EDIT->text(), 10002, true);
//    connected |= ptr_laser->setup_tcp_port(ip, 10000, true);
//    connected |= ptr_ptz->  setup_tcp_port(ip, 10000, true);
//    ptr_ptz->share_port_from(ptr_lens);
    p_tcu->emit connect_to_tcp(pref->ui->TCP_SERVER_IP_EDIT->text(), 10001);
    QThread::msleep(200);
    p_lens->emit connect_to_tcp(pref->ui->TCP_SERVER_IP_EDIT->text(), 10002);
//    pref->ui->TCP_SERVER_CHK->setEnabled(connected);
//    pref->ui->TCP_SERVER_CHK->setChecked(connected);
//    qDebug() << ptr_tcu->get_port_status() << ptr_lens->get_port_status() << ptr_laser->get_port_status() << ptr_ptz->get_port_status();
//    if (connected) QMessageBox::information(this, "serial port server", "connected to server");
//    else           QMessageBox::information(this, "serial port server", "cannot connect to server");
    QThread::msleep(200);
    p_rf->emit connect_to_tcp(pref->ui->TCP_SERVER_IP_EDIT->text(), 10003);

    QTimer::singleShot(1000,
    [this]()
    {
        if (ControlPort::TCP_CONNECTED & p_tcu->get_port_status())
        {
            pref->ui->TCP_SERVER_CHK->setEnabled(true);
            pref->ui->TCP_SERVER_CHK->setChecked(true);
        }
        else QMessageBox::information(this, "serial port server", "cannot connect to server");
    });
}

void UserPanel::disconnect_from_serial_server_tcp()
{
//    for (int i = 0; i < 4; i++) tcp_port[i]->disconnectFromHost();
//    ptr_tcu->  setup_tcp_port("", 0, false);
//    ptr_lens-> setup_tcp_port("", 0, false);
//    ptr_laser->setup_tcp_port("", 0, false);
//    ptr_ptz->  setup_tcp_port("", 0, false);
//    ptr_ptz->share_port_from(nullptr);
    p_tcu-> emit disconnect_from(true);
    p_lens->emit disconnect_from(true);
    pref->ui->TCP_SERVER_CHK->setEnabled(false);
    pref->ui->TCP_SERVER_CHK->setChecked(false);
//    QMessageBox::information(this, "serial port server", "disconnected from server");
//    qDebug() << ptr_tcu->get_port_status() << ptr_lens->get_port_status() << ptr_laser->get_port_status() << ptr_ptz->get_port_status();
//    for (int i = 0; i < 5; i++) use_tcp[i] = false;
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
        distance = QInputDialog::getInt(this, "DISTANCE INPUT", "DETECTED DISTANCE: ", 100, 100, pref->max_dist, 100, &ok, Qt::FramelessWindowHint);
        if (!ok) return;
    }

    if (distance < 100) distance = 100;
    ui->DISTANCE->setText(QString::asprintf("%d m", distance));
//    data_exchange(true);

    // change delay and gate width according to distance
//    delay_dist = distance + ptr_tcu->delay_offset * dist_ns;
    delay_dist = distance/* + p_tcu->get(TCU::OFFSET_DELAY) * dist_ns*/;
    update_delay();
//    update_gate_width();
//    change_mcp(150);

    rep_freq = 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000);
    if (rep_freq > 30) rep_freq = 30;
//    if      (distance < 1000) depth_of_view =  500 * dist_ns, ptr_tcu->laser_width = std::round(depth_of_view / dist_ns);
//    else if (distance < 3000) depth_of_view = 1000 * dist_ns, ptr_tcu->laser_width = std::round(depth_of_view / dist_ns);
//    else if (distance < 6000) depth_of_view = 2000 * dist_ns, ptr_tcu->laser_width = std::round(depth_of_view / dist_ns);
//    else                      depth_of_view = 3500 * dist_ns, ptr_tcu->laser_width = std::round(depth_of_view / dist_ns);
    if      (distance < 1000) depth_of_view =  500 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else if (distance < 3000) depth_of_view = 1000 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else if (distance < 6000) depth_of_view = 2000 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else                      depth_of_view = 3500 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));

//    ptr_tcu->communicate_display(convert_to_send_tcu(0x1E, distance), 7, 1, true);
//    ptr_tcu->set_user_param(TCUThread::DIST, distance);
//    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist - ptr_tcu->delay_offset * dist_ns));
    emit send_uint_tcu_msg(TCU::DIST, distance);
    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist));

    setup_hz(hz_unit);
    uint us, ns, ps;
//    split_value_by_unit(ptr_tcu->laser_width, us, ns, ps);
    split_value_by_unit(p_tcu->get(TCU::LASER_WIDTH), us, ns, ps);
    ui->LASER_WIDTH_EDIT_U->setText(QString::asprintf(  "%d", us));
    ui->LASER_WIDTH_EDIT_N->setText(QString::asprintf("%03d", ns));
    ui->LASER_WIDTH_EDIT_P->setText(QString::asprintf("%03d", ps));
//    split_value_by_unit(ptr_tcu->gate_width_a, us, ns, ps, 0);
    split_value_by_unit(p_tcu->get(TCU::GATE_WIDTH_A), us, ns, ps, 0);
    ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf(  "%d", us));
    ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%03d", ns));
    ui->GATE_WIDTH_A_EDIT_P->setText(QString::asprintf("%03d", ps));
//    split_value_by_unit(ptr_tcu->delay_a, us, ns, ps, 1);
    split_value_by_unit(p_tcu->get(TCU::DELAY_A), us, ns, ps, 1);
    ui->DELAY_A_EDIT_U->setText(QString::asprintf(  "%d", us));
    ui->DELAY_A_EDIT_N->setText(QString::asprintf("%03d", ns));
    ui->DELAY_A_EDIT_P->setText(QString::asprintf("%03d", ps));
//    split_value_by_unit(ptr_tcu->gate_width_b, us, ns, ps, 2);
    split_value_by_unit(p_tcu->get(TCU::GATE_WIDTH_B), us, ns, ps, 2);
    ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf(  "%d", us));
    ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%03d", ns));
    ui->GATE_WIDTH_B_EDIT_P->setText(QString::asprintf("%03d", ps));
//    split_value_by_unit(ptr_tcu->delay_b, us, ns, ps, 3);
    split_value_by_unit(p_tcu->get(TCU::DELAY_B), us, ns, ps, 3);
    ui->DELAY_B_EDIT_U->setText(QString::asprintf(  "%d", us));
    ui->DELAY_B_EDIT_N->setText(QString::asprintf("%03d", ns));
    ui->DELAY_B_EDIT_P->setText(QString::asprintf("%03d", ps));
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
    // image_3d[0] = ui->IMG_3D_CHECK->isChecked();

    image_mutex[0].lock();
    image_3d[0] = arg1;
    if (!arg1) frame_a_3d = 0/*, prev_3d[0] = cv::Mat(w[0], h[0], CV_8UC3)*/;
    image_mutex[0].unlock();

    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);

    if (simple_ui) {
        emit send_double_tcu_msg(TCU::DELAY_N, arg1 ? depth_of_view / dist_ns : 0);
        if (pref->ui->AUTO_MCP_CHK->isChecked()) pref->ui->AUTO_MCP_CHK->click();
    }
}

void UserPanel::on_ZOOM_IN_BTN_pressed()
{
    ui->ZOOM_IN_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x40, 0x00, 0x00, 0x41}, 7), 7, 1, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x40, 0x00, 0x00, 0x41}, 7), 7, 0, false);
//    ptr_lens->lens_control(LensThread::ZOOM_IN);
    emit send_lens_msg(Lens::ZOOM_IN);
}

void UserPanel::on_ZOOM_OUT_BTN_pressed()
{
    ui->ZOOM_OUT_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x20, 0x00, 0x00, 0x21}, 7), 7, 1, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x20, 0x00, 0x00, 0x21}, 7), 7, 0, false);
//    ptr_lens->lens_control(LensThread::ZOOM_OUT);
    emit send_lens_msg(Lens::ZOOM_OUT);
}

void UserPanel::on_FOCUS_FAR_BTN_pressed()
{
    ui->FOCUS_FAR_BTN->setText("x");
    focus_far();
}

void UserPanel::on_FOCUS_NEAR_BTN_pressed()
{
    ui->FOCUS_NEAR_BTN->setText("x");
    focus_near();
}

inline void UserPanel::focus_far()
{
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x80, 0x00, 0x00, 0x81}, 7), 7, 1, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x80, 0x00, 0x00, 0x81}, 7), 7, 0, false);
//    ptr_lens->lens_control(LensThread::FOCUS_FAR);
    emit send_lens_msg(Lens::FOCUS_FAR);
}

inline void UserPanel::focus_near()
{
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02}, 7), 7, 0, false);
//    ptr_lens->lens_control(LensThread::FOCUS_NEAR);
    emit send_lens_msg(Lens::FOCUS_NEAR);
}

void UserPanel::on_RADIUS_INC_BTN_pressed()
{
    ui->RADIUS_INC_BTN->setText("x");
    emit send_lens_msg(Lens::RADIUS_UP);
}

void UserPanel::on_RADIUS_DEC_BTN_pressed()
{
    ui->RADIUS_DEC_BTN->setText("x");
    emit send_lens_msg(Lens::RADIUS_DOWN);
}

inline void UserPanel::set_laser_preset_target(int *pos)
{
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x81, uchar((pos[0] >> 8) & 0xFF), uchar(pos[0] & 0xFF), uchar((((pos[0] >> 8) & 0xFF) + (pos[0] & 0xFF) + 0x82) & 0xFF)}, 7), 7, 1, false);
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4F, uchar((pos[1] >> 8) & 0xFF), uchar(pos[1] & 0xFF), uchar((((pos[1] >> 8) & 0xFF) + (pos[1] & 0xFF) + 0x51) & 0xFF)}, 7), 7, 1, false);
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4E, uchar((pos[2] >> 8) & 0xFF), uchar(pos[2] & 0xFF), uchar((((pos[2] >> 8) & 0xFF) + (pos[2] & 0xFF) + 0x50) & 0xFF)}, 7), 7, 1, false);
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x81, uchar((pos[3] >> 8) & 0xFF), uchar(pos[3] & 0xFF), uchar((((pos[3] >> 8) & 0xFF) + (pos[3] & 0xFF) + 0x83) & 0xFF)}, 7), 7, 1, false);

//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x81, uchar((pos[0] >> 8) & 0xFF), uchar(pos[0] & 0xFF), uchar((((pos[0] >> 8) & 0xFF) + (pos[0] & 0xFF) + 0x82) & 0xFF)}, 7), 7, 0, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4F, uchar((pos[1] >> 8) & 0xFF), uchar(pos[1] & 0xFF), uchar((((pos[1] >> 8) & 0xFF) + (pos[1] & 0xFF) + 0x51) & 0xFF)}, 7), 7, 0, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4E, uchar((pos[2] >> 8) & 0xFF), uchar(pos[2] & 0xFF), uchar((((pos[2] >> 8) & 0xFF) + (pos[2] & 0xFF) + 0x50) & 0xFF)}, 7), 7, 0, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x81, uchar((pos[3] >> 8) & 0xFF), uchar(pos[3] & 0xFF), uchar((((pos[3] >> 8) & 0xFF) + (pos[3] & 0xFF) + 0x83) & 0xFF)}, 7), 7, 0, false);

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

inline void UserPanel::lens_stop() {
//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 0, false);
//    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02}, 7), 7, 0, false);
//    ptr_lens->lens_control(LensThread::STOP);
    emit send_lens_msg(Lens::STOP);
}

void UserPanel::set_zoom()
{
    zoom = ui->ZOOM_EDIT->text().toUInt();

//    unsigned sum = 0;
//    uchar out[7];
//    out[0] = 0xFF;
//    out[1] = 0x01;
//    out[2] = 0x00;
//    out[3] = 0x4F;
//    out[4] = (zoom >> 8) & 0xFF;
//    out[5] = zoom & 0xFF;
//    for (int i = 1; i < 6; i++)
//        sum += out[i];
//    out[6] = sum & 0xFF;

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(QByteArray((char*)out, 7), 7, 1, false);
//    ptr_lens->communicate_display(QByteArray((char*)out, 7), 7, 0, false);
//    ptr_lens->lens_control(LensThread::SET_ZOOM, &zoom);
    emit send_lens_msg(Lens::SET_ZOOM, zoom);
}

void UserPanel::set_focus()
{
    focus = ui->FOCUS_EDIT->text().toUInt();

//    unsigned sum = 0;
//    uchar out[7];
//    out[0] = 0xFF;
//    out[1] = 0x01;
//    out[2] = 0x00;
//    out[3] = 0x4E;
//    out[4] = (focus >> 8) & 0xFF;
//    out[5] = focus & 0xFF;
//    for (int i = 1; i < 6; i++)
//        sum += out[i];
//    out[6] = sum & 0xFF;

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(QByteArray((char*)out, 7), 7, 1, false);
//    ptr_lens->communicate_display(QByteArray((char*)out, 7), 7, 0, false);
//    ptr_lens->lens_control(LensThread::SET_FOCUS, &focus);
    emit send_lens_msg(Lens::SET_FOCUS, focus);
}


void UserPanel::start_laser()
{
//    ptr_tcu->communicate_display(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 7, 0, false);
//    ptr_tcu->send_data(PortData{generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 7, 0, false, false});
    p_tcu->emit send_data(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 0, 0);

    QTimer::singleShot(1000, this, SLOT(init_laser()));
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
//    ptr_laser->communicate_display(QByteArray("MODE:RMT 1\r"), 11, 0, true, false);
//    ptr_laser->send_data(PortData{QByteArray("MODE:RMT 1\r"), 11, 0, true, false});
//    p_laser->emit send_data(QByteArray("MODE:RMT 1\r"), 0, 100);
    emit send_laser_msg("MODE:RMT 1\r");
    qDebug() << QString("MODE:RMT 1\r");

    // start
//    serial_port[3]->write("ON\r", 3);
//    serial_port[3]->waitForBytesWritten(100);
//    QThread::msleep(100);
//    serial_port[3]->readAll();
//    ptr_laser->communicate_display(QByteArray("ON\r"), 3, 0, true, false);
//    ptr_laser->send_data(PortData{QByteArray("ON\r"), 3, 0, true, false});
//    p_laser->emit send_data(QByteArray("ON\r"), 0, 100);
    emit send_laser_msg("ON\r");
    qDebug() << QString("ON\r");

    // enable external trigger
//    serial_port[3]->write("QSW:PRF 0\r", 10);
//    serial_port[3]->waitForBytesWritten(100);
//    QThread::msleep(100);
//    serial_port[3]->readAll();
//    ptr_laser->communicate_display(QByteArray("QSW:PRF 0\r"), 10, 0, true, false);
//    ptr_laser->send_data(PortData{QByteArray("QSW:PRF 0\r"), 10, 0, true, false});
//    p_laser->emit send_data(QByteArray("QSW:PRF 0\r"), 0);
    emit send_laser_msg("QSW:PRF 0\r");
    qDebug() << QString("QSW:PRF 0\r");

    update_current();

    ui->SIMPLE_LASER_CHK->setEnabled(true);
    ui->FIRE_LASER_BTN->setEnabled(true);
    ui->SIMPLE_LASER_CHK->click();
}

void UserPanel::change_mcp(int val)
{
    if (val < 0) val = 0;
    if (val > mcp_max) val = mcp_max;
    ui->MCP_EDIT->setText(QString::number(val));
    ui->MCP_SLIDER->setValue(val);

//    if (val == ptr_tcu->mcp) return;
    if (val == (int)std::round(p_tcu->get(TCU::MCP))) return;

//    convert_to_send_tcu(0x0A, mcp);
//    static QElapsedTimer t;
//    if (!h_grab_thread || t.elapsed() > (fps > 9 ? 900 / fps : 100)) communicate_display(0, convert_to_send_tcu(0x0A, mcp), 7, 1, false), t.start();
//    ptr_tcu->communicate_display(convert_to_send_tcu(0x0A, mcp), 7, 1, false);
//    ptr_tcu->set_user_param(TCUThread::MCP, val);
    emit send_uint_tcu_msg(TCU::MCP, val);
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
    curr_cam->gain_analog(true, &gain_analog_edit);
    // qDebug() << "gain set" << gain_analog_edit << (int)std::round(gain_analog_edit);
    ui->GAIN_EDIT->setText(QString::number((int)std::round(gain_analog_edit)));
    ui->GAIN_SLIDER->setValue((int)std::round(gain_analog_edit));
}

void UserPanel::change_delay(int val)
{
    if (abs(delay_dist - val) < 1) return;
    delay_dist = val;
    update_delay();
}

void UserPanel::change_gatewidth(int val)
{
    if (abs(depth_of_view - val) < 1) return;
    depth_of_view = val;
    update_gate_width();
}

void UserPanel::change_focus_speed(int val)
{
    if (val < 1)  val = 1;
    if (val > 63) val = 63;
    ui->FOCUS_SPEED_EDIT->setText(QString::asprintf("%d", val));
    ui->FOCUS_SPEED_SLIDER->setValue(val);
//    if (val > 1) val -= 1;

//#ifndef LVTONG
//    static QElapsedTimer t;
//    if (t.isValid() && t.elapsed() < 100) return;
//    t.restart();
//#endif

//    int temp_serial_port_id = pref->share_port && serial_port[0]->isOpen() ? 0 : 2;

//    uchar out_data[9] = {0};
//    out_data[0] = 0xB0;
//    out_data[1] = 0x01;
//    out_data[2] = 0xA0;
//    out_data[3] = 0x01;
//    out_data[4] = val;
//    out_data[5] = val;
//    out_data[6] = val;
//    out_data[7] = val;

//    out_data[8] = (4 * (uint)val + 0xA2) & 0xFF;
////    communicate_display(temp_serial_port_id, QByteArray((char*)out_data, 9), 9, 0, false);
//    ptr_lens->communicate_display(QByteArray((char*)out_data, 9), 9, 0, false);

//    uint u_val = val;
//    ptr_lens->lens_control(LensThread::STEPPING, &u_val);
    emit send_lens_msg(Lens::STEPPING, val);

//     out_data[0] = 0xB0;
//     out_data[1] = 0x02;
//     out_data[2] = 0xA0;
//     out_data[3] = 0x01;
//     out_data[4] = val;
//     out_data[5] = val;
//     out_data[6] = val;
//     out_data[7] = val;

//     out_data[8] = (4 * (uint)val + 0xA3) & 0xFF;
// //    if (multi_laser_lenses) communicate_display(temp_serial_port_id, QByteArray((char*)out_data, 9), 9, 0, false);
//     if (multi_laser_lenses) ptr_lens->communicate_display(QByteArray((char*)out_data, 9), 9, 0, false);
}

void UserPanel::on_ZOOM_IN_BTN_released()
{
    ui->ZOOM_IN_BTN->setText((simple_ui & lang) ? tr("IN") : "+");
    lens_stop();
}

void UserPanel::on_ZOOM_OUT_BTN_released()
{
    ui->ZOOM_OUT_BTN->setText((simple_ui & lang) ? tr("OUT") : "-");
    lens_stop();
}

void UserPanel::on_FOCUS_NEAR_BTN_released()
{
    ui->FOCUS_NEAR_BTN->setText((simple_ui & lang) ? tr("NEAR") : "+");
    lens_stop();
}

void UserPanel::on_FOCUS_FAR_BTN_released()
{
    ui->FOCUS_FAR_BTN->setText((simple_ui & lang) ? tr("FAR") : "-");
    lens_stop();
}

void UserPanel::on_RADIUS_INC_BTN_released()
{
    ui->RADIUS_INC_BTN->setText((simple_ui & lang) ? tr("INC") : "+");
    lens_stop();
}

void UserPanel::on_RADIUS_DEC_BTN_released()
{
    ui->RADIUS_DEC_BTN->setText((simple_ui & lang) ? tr("DEC") : "-");
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

//            for (int i = 0; i < 5; i++) {
//                if (edit == com_edit[i]) {
////                    if (serial_port[i]) serial_port[i]->close();
////                    setup_serial_port(serial_port + i, i, edit->text(), 9600);
//                }
//            }
            if (edit == ui->FREQ_EDIT) {
                switch (hz_unit) {
                // kHz
                case 0: rep_freq = ui->FREQ_EDIT->text().toFloat(); break;
                // Hz
                case 1: rep_freq = ui->FREQ_EDIT->text().toFloat() / 1000; break;
                default: break;
                }
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x00, 1.25e5 / ptr_tcu->rep_freq), 7, 1, false);
//                ptr_tcu->set_user_param(TCUThread::REPEATED_FREQ, rep_freq);
                emit send_double_tcu_msg(TCU::REPEATED_FREQ, rep_freq);
            }
            else if (edit == ui->LASER_WIDTH_EDIT_U) {
                laser_width = edit->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt() + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.;
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//                ptr_tcu->set_user_param(TCU::LASER_WIDTH, laser_width);
                update_laser_width();
                if (simple_ui) ;
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_U) {
                depth_of_view = (edit->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt() + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.) * dist_ns;
                update_gate_width();
            }
            else if (edit == ui->GATE_WIDTH_B_EDIT_U) {
//                depth_of_view = (edit->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->gate_width_n) * dist_ns;
                if (p_tcu->get(TCU::AB_LOCK)) {
                    depth_of_view = (edit->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::GATE_WIDTH_N)) * dist_ns;
                    update_gate_width();
                }
                else {
                    double temp_gw_b;
                    temp_gw_b = edit->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::GATE_WIDTH_N);
                    emit send_double_tcu_msg(TCU::GATE_WIDTH_B, temp_gw_b);
                }
            }
            else if (edit == ui->LASER_WIDTH_EDIT_N) {
                if (edit->text().toInt() > 999) laser_width = edit->text().toInt() + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.;
                else laser_width = edit->text().toInt() + ui->LASER_WIDTH_EDIT_U->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.;
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//                ptr_tcu->set_user_param(TCU::LASER_WIDTH, ptr_tcu->laser_width);
                update_laser_width();
                if (simple_ui) ;
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_N) {
                if (edit->text().toInt() > 999) depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.) * dist_ns;
                else depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.) * dist_ns;
                update_gate_width();
            }
            else if (edit == ui->GATE_WIDTH_B_EDIT_N) {
//                if (edit->text().toInt() > 999) depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->gate_width_n) * dist_ns;
//                else depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->gate_width_n) * dist_ns;
                if (p_tcu->get(TCU::AB_LOCK)) {
                    if (edit->text().toInt() > 999) depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::GATE_WIDTH_N)) * dist_ns;
                    else depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::GATE_WIDTH_N)) * dist_ns;
                    update_gate_width();
                }
                else {
                    double temp_gw_b;
                    if (edit->text().toInt() > 999) temp_gw_b = edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::GATE_WIDTH_N);
                    else temp_gw_b = edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::GATE_WIDTH_N);
                    emit send_double_tcu_msg(TCU::GATE_WIDTH_B, temp_gw_b);
                }
            }
            else if (edit == ui->LASER_WIDTH_EDIT_P) {
                laser_width = edit->text().toInt() / 1000. + ui->LASER_WIDTH_EDIT_U->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt();
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//                ptr_tcu->set_user_param(TCU::LASER_WIDTH, ptr_tcu->laser_width);
                update_laser_width();
                if (simple_ui) ;
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_P) {
                depth_of_view = (edit->text().toInt() / 1000. + ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt()) * dist_ns;
                update_gate_width();
            }
            else if (edit == ui->GATE_WIDTH_B_EDIT_P) {
//                depth_of_view = (edit->text().toInt() / 1000. + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() - ptr_tcu->gate_width_n) * dist_ns;
                if (p_tcu->get(TCU::AB_LOCK)) {
                    depth_of_view = (edit->text().toInt() / 1000. + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() - p_tcu->get(TCU::GATE_WIDTH_N)) * dist_ns;
                    update_gate_width();
                }
                else {
                    double temp_gw_b;
                    temp_gw_b = edit->text().toInt() / 1000. + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() - p_tcu->get(TCU::GATE_WIDTH_N);
                    emit send_double_tcu_msg(TCU::GATE_WIDTH_B, temp_gw_b);
                }
            }
            else if (edit == ui->DELAY_A_EDIT_U) {
                delay_dist = (edit->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.) * dist_ns;
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_U) {
//                delay_dist = (edit->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000. - ptr_tcu->delay_n) * dist_ns;
                if (p_tcu->get(TCU::AB_LOCK)) {
                    delay_dist = (edit->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::DELAY_N)) * dist_ns;
                    update_delay();
                }
                else {
                    double temp_delay_b;
                    temp_delay_b = edit->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::DELAY_N);
                    emit send_double_tcu_msg(TCU::DELAY_B, temp_delay_b);
                }
            }
            else if (edit == ui->DELAY_A_EDIT_N) {
                if (edit->text().toInt() > 999) delay_dist = (edit->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.) * dist_ns;
                else delay_dist = (edit->text().toInt() + ui->DELAY_A_EDIT_U->text().toInt() * 1000+ ui->DELAY_A_EDIT_P->text().toInt() / 1000.) * dist_ns;
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_N) {
//                if (edit->text().toInt() > 999) delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_P->text().toInt() / 1000.) * dist_ns - ptr_tcu->delay_n;
//                else delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->delay_n) * dist_ns;
                if (p_tcu->get(TCU::AB_LOCK)) {
                    if (edit->text().toInt() > 999) delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::DELAY_N)) * dist_ns;
                    else delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::DELAY_N)) * dist_ns;
                    update_delay();
                }
                else {
                    double temp_delay_b;
                    if (edit->text().toInt() > 999) temp_delay_b = edit->text().toInt() + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::DELAY_N);
                    else temp_delay_b = edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - p_tcu->get(TCU::DELAY_N);
                    emit send_double_tcu_msg(TCU::DELAY_B, temp_delay_b);
                }
            }
            else if (edit == ui->DELAY_A_EDIT_P) {
                delay_dist = (edit->text().toInt() / 1000. + ui->DELAY_A_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt()) * dist_ns;
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_P) {
//                delay_dist = (edit->text().toInt() / 1000. + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() - ptr_tcu->delay_n) * dist_ns;
                if (p_tcu->get(TCU::AB_LOCK)) {
                    delay_dist = (edit->text().toInt() / 1000. + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() - p_tcu->get(TCU::DELAY_N)) * dist_ns;
                    update_delay();
                }
                else {
                    double temp_delay_b;
                    temp_delay_b = edit->text().toInt() / 1000. + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() - p_tcu->get(TCU::DELAY_N);
                    emit send_double_tcu_msg(TCU::DELAY_B, temp_delay_b);
                }
            }
            else if (edit == ui->GATE_WIDTH_N_EDIT_N) {
//                ptr_tcu->gate_width_n = edit->text().toFloat();
                emit send_double_tcu_msg(TCU::GATE_WIDTH_N, edit->text().toFloat());
                update_gate_width();
            }
            else if (edit == ui->DELAY_N_EDIT_N) {
//                ptr_tcu->delay_n = edit->text().toFloat();
                emit send_double_tcu_msg(TCU::DELAY_N, edit->text().toFloat());
                update_delay();
            }
            else if (edit == ui->MCP_EDIT) {
                ui->MCP_SLIDER->setValue(ui->MCP_EDIT->text().toInt());
                if (ui->AUTO_MCP_CHK->isChecked()) ui->AUTO_MCP_CHK->click();
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
                angle_h = ui->ANGLE_H_EDIT->text().toDouble();
                // Ensure horizontal angle is always positive (0 to 360)
                angle_h = fmod(angle_h + 360.0, 360.0);
                if (!pref->gcan) emit send_ptz_msg(PTZ::SET_H, angle_h);
                else            p_usbcan->emit control(USBCAN::POSITION, angle_h);
            }
            else if (edit == ui->ANGLE_V_EDIT) {
                angle_v = ui->ANGLE_V_EDIT->text().toDouble();
                if (!pref->gcan) emit send_ptz_msg(PTZ::SET_V, angle_v);
                else             p_usbcan->emit control(USBCAN::PITCH, angle_v);
            }
            else if (edit == ui->PTZ_COM_EDIT) {
                if (ui->PTZ_COM_EDIT->text().isEmpty())  p_usbcan->emit disconnect_from();
                else p_usbcan->emit connect_to();
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
        case Qt::Key_0:
            secondary_display->show(); break;
        case Qt::Key_1:
            fw_display[0]->show(); break;
        case Qt::Key_2:
            fw_display[1]->show(); break;
        case Qt::Key_3:
        case Qt::Key_4:
//            goto_laser_preset(1 << (event->key() - Qt::Key_1));
            break;
        case Qt::Key_S:
//            ui->TITLE->prog_settings->show();
//            ui->TITLE->prog_settings->raise();
            pref->show();
            pref->raise();
            break;
        case Qt::Key_Q:
//            std::accumulate(use_tcp, use_tcp + 5, 0) ? disconnect_from_serial_server_tcp() : connect_to_serial_server_tcp();
//            (ptr_tcu->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
//            (ptr_lens->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
//            (ptr_laser->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
//            (ptr_ptz->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ?
//                disconnect_from_serial_server_tcp() : connect_to_serial_server_tcp();
            (p_tcu->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
            (p_lens->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
            (p_laser->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
            (p_ptz->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ?
                disconnect_from_serial_server_tcp() : connect_to_serial_server_tcp();
            break;
        case Qt::Key_L:
            laser_settings->show();
            laser_settings->raise();
            break;
        case Qt::Key_V:
#ifdef DISTANCE_3D_VIEW
            view_3d->show();
            view_3d->raise();
#endif //DISTANCE_3D_VIEW
            break;
        case Qt::Key_A:
            // FIXME: aliasing function temporary banned
            // aliasing->show_ui();
            break;
        case Qt::Key_D:
            // aliasing->update_distance(delay_dist);
            break;
        case Qt::Key_P:
            preset->show_ui();
            break;
        default: break;
        }
        break;
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
        case Qt::Key_D:
            switch_ui();
            break;
        default: break;
        }
        break;
    default: break;
    }
}

void UserPanel::resizeEvent(QResizeEvent *event)
{
    int ww, hh;
    switch (image_rotate[0]) {
    case 1:
    case 3:
        ww = h[0]; hh = w[0]; break;
    case 0:
    case 2:
    default:
        ww = w[0]; hh = h[0]; break;
    }
//    ww = w[0]; hh = h[0];
    ww = ui->IMG_3D_CHECK->isChecked() ? ww + 104 : ww;
    QRect window = this->geometry();
//    qDebug() << window;
    int grp_width = ui->RIGHT->width();

    ui->RIGHT->move(window.width() - 12 - grp_width, 40);

    QRect region = ui->SOURCE_DISPLAY->geometry();
    QPoint center = ui->SOURCE_DISPLAY->center;

    int mid_width = window.width() - 12 - grp_width - 10 - ui->LEFT->width();
    ui->MID->setGeometry(10 + ui->LEFT->width(), 40, mid_width, window.height() - 40 - ui->STATUS->height());
    int width = mid_width - 20, height = width * hh / ww;
//    qDebug("width %d, height %d\n", width, height);
    int height_constraint = ui->MID->height() - 30 - 24, // bottom padding 24 pixels
        width_constraint = height_constraint * ww / hh;
//    qDebug("width_constraint %d, height_constraint %d\n", width_constraint, height_constraint);
    if (width < width_constraint) width_constraint = width, height_constraint = width_constraint * hh / ww;
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
//    ui->CURR_COORD->move(region.x() + 80, 5);
    ui->START_COORD->move(region.x() + 80, 5);
    ui->SHAPE_INFO->move(ui->START_COORD->geometry().right() + 20, 5);
    ui->SHIFT_INFO->move(ui->SHAPE_INFO->geometry().right() + 20, 5);

    ui->INFO_CHECK->move(region.right() - 60, 0);
    ui->CENTER_CHECK->move(region.right() - 60, 20);
    ui->HIDE_BTN->move(ui->MID->geometry().left() - 8, this->geometry().height() / 2 - 10);
//    ui->HIDE_BTN->move(ui->LEFT->geometry().right() - 10 + (ui->SOURCE_DISPLAY->geometry().left() + ui->MID->geometry().left() - ui->LEFT->geometry().right() + 10) / 2 + 2, this->geometry().height() / 2 - 10);
    ui->RULER_H->setGeometry(region.left(), region.bottom() - 10, region.width(), 32);
    ui->RULER_V->setGeometry(region.right() - 10, region.top(), 32, region.height());
#ifdef LVTONG
    ui->FISHNET_RESULT->move(region.right() - 170, 5);
#endif

    image_mutex[0].lock();
    ui->SOURCE_DISPLAY->setGeometry(region);
    ui->SOURCE_DISPLAY->update_roi(center);
    qInfo("display region x: %d, y: %d, w: %d, h: %d", region.x(), region.y(), region.width(), region.height());
    image_mutex[0].unlock();

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

    if (event->mimeData()->urls().length() < 99) event->acceptProposedAction();
}

void UserPanel::dropEvent(QDropEvent *event)
{
    QMainWindow::dropEvent(event);

    static QMimeDatabase mime_db;

    if (event->mimeData()->urls().length() == 1) {
        QString file_name = event->mimeData()->urls().first().toLocalFile();
        QFileInfo file_info = QFileInfo(file_name);
        if (file_info.exists() && file_info.size() > 2e9) {
            QMessageBox::warning(this, "PROMPT", tr("File size limit (2 GB) exceeded"));
            return;
        }
        // TODO: check file type before reading/processing file
//        QMimeType file_type = mime_db.mimeTypeForFile(file_name);
        if (mime_db.mimeTypeForFile(file_name).name().startsWith("video")) {
            load_video_file(current_video_filename = file_name, false, [](cv::Mat &frame, void* ptr){ (*(std::queue<cv::Mat>*)ptr).push(frame.clone()); }, &(this->q_img[0]), 0);
            return;
        }

        if (!load_image_file(file_name, true)) load_config(file_name);

    }
    else {
        bool result = true, init = true;
        uint count = 0;
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
    if (init) grab_image[0] = false;
    if (!display_mutex[0].tryLock(1e3)) return false;

    QImage qimage_temp;
    cv::Mat mat_temp;
    bool tiff = false;

    if (filename.endsWith(".tif") || filename.endsWith(".tiff")) tiff = true;

    if (tiff) {
        if (ImageIO::load_image_tif(mat_temp, filename)) { display_mutex[0].unlock(); return false; }
    }
    else {
        if (!qimage_temp.load(filename)) { display_mutex[0].unlock(); return false; }
    }

    if (device_on) {
        QMessageBox::warning(this, "PROMPT", tr("Cannot read local image while cam is on"));
        display_mutex[0].unlock();
        return true;
    }

    if (init) {
        if (tiff) start_static_display(mat_temp.cols, mat_temp.rows, false, 0, 8 * (mat_temp.depth() / 2 + 1));
        else      start_static_display(qimage_temp.width(), qimage_temp.height(), qimage_temp.format() != QImage::Format_Indexed8);
    }
    qimage_temp = qimage_temp.convertToFormat(is_color[0] ? QImage::Format_RGB888 : QImage::Format_Indexed8);

    if (tiff) q_img[0].push(mat_temp.clone());
    else      q_img[0].push(cv::Mat(h[0], w[0], is_color[0] ? CV_8UC3 : CV_8UC1, (uchar*)qimage_temp.bits(), qimage_temp.bytesPerLine()).clone());

    display_mutex[0].unlock();
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

static int ffmpeg_interrupt_callback(void *p) {
    time_t *t0 = (time_t *)p;
    if (*t0 > 0 && time(NULL) - *t0 > 30) return 1;
    return 0;
}

// TODO: release ptrs after exited half-way
int UserPanel::load_video_file(QString filename, bool format_gray, void (*process_frame)(cv::Mat &, void *), void *ptr, int display_idx, bool display)
{
//    video_input->setMedia(QUrl(filename));
//    video_input->setMuted(true);
//    video_input->play();
//    video_surface->frame_count = 0;

    if (filename.startsWith("cv:") || filename.startsWith("CV:")) {
        std::thread t([this, filename, process_frame, ptr, display_idx, display](){
            if (current_video_filename != filename) current_video_filename = "";
            if (display) {
                grab_image[display_idx] = false;
                if (!display_mutex[display_idx].tryLock(1e3)) return -1;
            }

            cv::VideoCapture vin;
            bool convert_to_number = false;
            int device_idx = filename.right(filename.length() - 3).toUInt(&convert_to_number);
            if (convert_to_number) vin.open(device_idx, cv::CAP_DSHOW);
            else                   vin.open(filename.right(filename.length() - 3).toLatin1().constData());
            if (!vin.isOpened()) { if (display) display_mutex[display_idx].unlock(); return -2; }

            if (convert_to_number) {
                vin.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
                vin.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
//                vin.set(cv::CAP_PROP_FPS, 10);
            }
            cv::Mat frame;
            vin >> frame;
            if (display) start_static_display(frame.cols, frame.rows, frame.channels() == 3, display_idx, 8, -2);
//            if (display) start_static_display(1920, 1080, frame.channels() == 3, display_idx, 8, -2);

            while (grab_image[display_idx]) {
                vin >> frame;
                if (frame.empty()) { QThread::msleep(5); continue; }
                cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
                if (process_frame) (*process_frame)(frame, ptr);
            }

            if (display) display_mutex[display_idx].unlock();

            if (!display) this->video_stopped();
        });
        t.detach();
    }
    else {
        std::thread t([this, filename, format_gray, process_frame, ptr, display_idx, display](){
            if (current_video_filename != filename) current_video_filename = "";
            if (display) {
                qDebug() << display_idx;
                grab_image[display_idx] = false;
                if (!display_mutex[display_idx].tryLock(1e3)) return -1;
            }

            bool not_rtsp_stream = !filename.contains("rtsp://");

            AVFormatContext *format_context = avformat_alloc_context();
            std::shared_ptr<AVFormatContext*> closer_format_context(&format_context, avformat_close_input);
            AVCodecParameters *codec_param = NULL;
            //        std::shared_ptr<AVCodecParameters*> deleter_codec_param(&codec_param, avcodec_parameters_free);
            const AVCodec *codec = NULL;
            AVCodecContext *codec_context = avcodec_alloc_context3(NULL);
            std::shared_ptr<AVCodecContext*> closer_codec_context(&codec_context, avcodec_free_context);
            AVFrame *frame = av_frame_alloc();
            std::shared_ptr<AVFrame*> deleter_frame(&frame, av_frame_free);
            AVFrame *frame_result = av_frame_alloc();
            std::shared_ptr<AVFrame*> deleter_frame_result(&frame_result, av_frame_free);
            AVFrame *frame_filter = av_frame_alloc();
            std::shared_ptr<AVFrame*> deleter_frame_filter(&frame_filter, av_frame_free);
            uint8_t *buffer = NULL;
            std::shared_ptr<uint8_t> deleter_buffer(buffer, av_free);
            SwsContext *sws_context = NULL;
            std::shared_ptr<SwsContext> deleter_sws_context(sws_context, sws_freeContext);
            static AVFilterContext *buffersink_ctx = NULL;
            static AVFilterContext *buffersrc_ctx = NULL;
            AVFilterGraph *filter_graph = avfilter_graph_alloc();
            std::shared_ptr<AVFilterGraph*> deleter_filter_graph(&filter_graph, avfilter_graph_free);

            // process timeout
            time_t start_time = 0;

            format_context->interrupt_callback.callback = ffmpeg_interrupt_callback;
            format_context->interrupt_callback.opaque = &start_time;

            // set buffer size //TODO: set frame buffer size
//            AVIOContext *avio_ctx = nullptr;
//            uint8_t *frame_buffer = (uint8_t *)av_malloc(33554432); // Allocate a buffer (32 MB)
//            avio_ctx = avio_alloc_context(frame_buffer, 33554432, 0, nullptr, nullptr, nullptr, nullptr);
//            format_context->pb = avio_ctx;

            // open input video
            AVInputFormat *input_format = (AVInputFormat *)av_find_input_format("dshow");
            start_time = time(NULL);
            if (avformat_open_input(&format_context, filename.toUtf8().constData(), input_format, NULL) != 0) { if (display) display_mutex[display_idx].unlock(); return -2; }

            // fetch video info
            if (avformat_find_stream_info(format_context, NULL) < 0) { if (display) display_mutex[display_idx].unlock(); return -2; }

            // find the video stream with max resolution (in width)
            int video_stream_idx = -1;
            int max_res_w = 0;
            for (uint i = 0; i < format_context->nb_streams; i++) {
                if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    if (format_context->streams[i]->codecpar->width > max_res_w) {
                        video_stream_idx = i;
                        max_res_w = format_context->streams[i]->codecpar->width;
                    }
                }
            }
            if (video_stream_idx == -1) { if (display) display_mutex[display_idx].unlock(); return -2; }

            // point the codec parameter to the first stream's
            codec_param = format_context->streams[video_stream_idx]->codecpar;

//            if (display) ptr_tcu->ccd_freq = frame_rate_edit = std::round(format_context->streams[video_stream_idx]->r_frame_rate.num / format_context->streams[video_stream_idx]->r_frame_rate.den);
            if (display) emit send_double_tcu_msg(TCU::CCD_FREQ, frame_rate_edit = std::round(format_context->streams[video_stream_idx]->r_frame_rate.num / format_context->streams[video_stream_idx]->r_frame_rate.den));

            codec = avcodec_find_decoder(codec_param->codec_id);
            if (codec == NULL) { if (display) display_mutex[display_idx].unlock(); return -3; }

            if (avcodec_parameters_to_context(codec_context, codec_param) < 0) { if (display) display_mutex[display_idx].unlock(); return -3; }

            if (avcodec_open2(codec_context, codec, NULL) < 0) { if (display) display_mutex[display_idx].unlock(); return -3; }

            cv::Mat cv_frame(codec_context->height, codec_context->width, CV_MAKETYPE(CV_8U, format_gray ? 1 : 3));

            // allocate data buffer
            AVPixelFormat pixel_fmt = format_gray ? AV_PIX_FMT_NV12 : AV_PIX_FMT_RGB24;
            int byte_num = av_image_get_buffer_size(pixel_fmt, codec_context->width, codec_context->height, 1);
            buffer = (uint8_t *)av_malloc(byte_num * sizeof(uint8_t));

            av_image_fill_arrays(frame_result->data, frame_result->linesize, buffer, pixel_fmt, codec_context->width, codec_context->height, 1);

            sws_context = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt,
                                         codec_context->width, codec_context->height, pixel_fmt, SWS_BILINEAR, NULL, NULL, NULL);

            if (display) start_static_display(codec_context->width, codec_context->height, !format_gray, display_idx, 8, -2);

            AVPacket packet;
            if (format_gray) init_filter_graph(format_context, video_stream_idx, filter_graph, codec_context, &buffersink_ctx, &buffersrc_ctx);

            AVRational time_base = format_context->streams[video_stream_idx]->time_base;
            AVRational time_base_q = { 1, AV_TIME_BASE };
            long long last_pts = AV_NOPTS_VALUE;
            long long delay;
            QElapsedTimer elapsed_timer;
            int result;
            uint frame_count = 0;
            while (grab_image[display_idx]) {
                start_time = time(NULL);
                if ((result = av_read_frame(format_context, &packet)) < 0) {
                    //                if (result == AVERROR_EOF) grab_image = false;
                    if (result == AVERROR_EOF) break;
                    continue;
                }

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
                            if (not_rtsp_stream && display && frame->pts != AV_NOPTS_VALUE) {
                                if (last_pts != AV_NOPTS_VALUE) {
                                    delay = av_rescale_q(frame->pts - last_pts, time_base, time_base_q) - elapsed_timer.nsecsElapsed() / 1e6;
//                                    qDebug() << av_rescale_q(frame->pts - last_pts, time_base, time_base_q) << elapsed_timer.nsecsElapsed() / 1000;
                                    if (delay > 0 && delay < 1000000) av_usleep(delay);
                                    elapsed_timer.restart();
                                }
                                last_pts = frame->pts;
                            }
                            //                        av_frame_unref(frame_result);
                            sws_scale(sws_context, frame->data, frame->linesize, 0, codec_context->height, frame_result->data, frame_result->linesize);
                        }

                        cv_frame.data = (format_gray ? frame_filter : frame_result)->data[0];
                        //                    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

                        if (process_frame) (*process_frame)(cv_frame, ptr);
                        //                    qInfo() << filename << frame_count++;
                        //                    img_q.push(cv_frame.clone());

                        av_frame_unref(frame);
                    }
                }
            }
            //        avfilter_graph_free(&filter_graph);
            //        sws_freeContext(sws_context);
            //        av_free(&buffer);
            //        av_frame_free(&frame_filter);
            //        av_frame_free(&frame_result);
            //        av_frame_free(&frame);
            //        avcodec_free_context(&codec_context);
            //        avcodec_parameters_free(&codec_param);
            //        avformat_close_input(&format_context);

            if (display) display_mutex[display_idx].unlock();

            if (!display) this->video_stopped();
            return 0;
        });
        t.detach();
    }

    return 0;
}

void UserPanel::update_pixel_depth(int depth, int display_idx)
{
    this->pixel_depth[display_idx] = depth;
    status_bar->img_pixel_depth->set_text(QString::number(depth) + "-bit");
}

inline void UserPanel::update_tcu_param_pos(int dir, QLabel *u_unit, QLineEdit *n_input, QLabel *n_unit, QLineEdit *p_input)
{
//    switch (ptr_tcu->tcu_type) {
    switch (dir) {
        case -1:
            u_unit->setText("μs");
            n_unit->setText("ns");
            u_unit->move(u_unit->pos() + QPoint(3, 0));
            n_input->move(n_input->pos() + QPoint(14, 0));
            n_unit->move(n_unit->pos() + QPoint(17, 0));
            p_input->hide();
            break;
        case 1:
            u_unit->setText(",");
            n_unit->setText(".");
            u_unit->move(u_unit->pos() - QPoint(3, 0));
            n_input->move(n_input->pos() - QPoint(14, 0));
            n_unit->move(n_unit->pos() - QPoint(17, 0));
            p_input->show();
            break;
        default: break;
    }
}

inline void UserPanel::split_value_by_unit(float val, uint &us, uint &ns, uint &ps, int idx) {
    static float ONE = 1.f;
    switch (p_tcu->type())
    {
    case 0:
    case 1:
    {
        us = uint(val + 0.001) / 1000;
        ns = uint(val + 0.001) % 1000;
        if (idx < 0) ps = 0;
//        else         ps = std::round(std::modf(val + 0.001, &ONE) * 1000 / ptr_tcu->ps_step[idx]) * ptr_tcu->ps_step[idx];
        else         ps = std::round(std::modf(val + 0.001, &ONE) * 1000 / p_tcu->get(TCU::PS_STEP_1 + idx)) * p_tcu->get(TCU::PS_STEP_1 + idx);
        while (ps > 999) ps -= 1000, ns++;
        while (ns > 999) ns -= 1000, us++;
        break;
    }
    case 2:
    {
        double step = 14.9;
        if (idx == 1) step = 11.23;
        us = uint(val + 0.001) / 1000;
        ns = uint(val + 0.001) % 1000;
        ps = std::round(std::modf(val + 0.001, &ONE) * 1000 / step) * step;
        while (ps > 999) ps -= 1000, ns++;
        while (ns > 999) ns -= 1000, us++;
        break;
    }
    default: break;
    }
}

void UserPanel::rotate(int angle)
{
    image_mutex[0].lock();
    image_rotate[0] = angle;
    image_mutex[0].unlock();

    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
}

//inline uint UserPanel::get_width_in_us(float val)
//{
//    return uint(val + 0.001) / 1000;
//}

//inline uint UserPanel::get_width_in_ns(float val)
//{
//    return uint(val + 0.001) % 1000;
//}

//inline uint UserPanel::get_width_in_ps(float val, int idx)
//{
//    static float ONE = 1;
//    // deprecated 50-ps stepping calculation
////    return std::round(std::modf(val + 0.001, &ONE) * 20.) * 50;
//    return std::round(std::modf(val + 0.001, &ONE) * 1000 / ptr_tcu->ps_step[idx]) * ptr_tcu->ps_step[idx];
//}

void UserPanel::start_static_display(int width, int height, bool is_color, int display_idx, int pixel_depth, int device_type)
{
    int select_display_thread = display_thread_idx[display_idx];

    if (select_display_thread != -1) {
        if (grab_image[select_display_thread] || h_grab_thread[select_display_thread]) {
            grab_image[select_display_thread] = false;
            if (h_grab_thread[select_display_thread]) {
                h_grab_thread[select_display_thread]->quit();
                h_grab_thread[select_display_thread]->wait();
                delete h_grab_thread[select_display_thread];
                h_grab_thread[select_display_thread] = NULL;
            }
        }
        while (grab_thread_state[select_display_thread]) ;
        std::queue<cv::Mat>().swap(q_img[select_display_thread]);
    }

    image_mutex[display_idx].lock();
    w[display_idx] = width;
    h[display_idx] = height;
    if (display_idx == 0) status_bar->img_resolution->emit set_text(QString::asprintf("%d x %d", width, height));
    this->is_color[display_idx] = is_color;
//    pixel_depth = img.depth();
    update_pixel_depth(pixel_depth, display_idx);
    this->device_type = device_type;
    image_mutex[display_idx].unlock();


    if (display_idx) fw_display[display_idx]->resize_display(width, height);
    else {
        QResizeEvent e(this->size(), this->size());
        resizeEvent(&e);
    }

    grab_image[display_idx] = true;
    h_grab_thread[display_idx] = new GrabThread((void*)this, display_idx);
    if (h_grab_thread[display_idx] == NULL) {
        grab_image[display_idx] = false;
        QMessageBox::warning(this, "PROMPT", tr("Cannot display image"));
        return;
    }
    h_grab_thread[display_idx]->start();
    start_grabbing = true;
    displays[display_idx]->is_grabbing = true;
    if (display_idx == 0) {
        enable_controls(true);
        ui->ENUM_BUTTON->setEnabled(false);
        ui->START_BUTTON->setEnabled(false);
        ui->STOP_GRABBING_BUTTON->setEnabled(true);
    }
    display_thread_idx[display_idx] = display_idx;
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
    if (record_original[0]) {
//        curr_cam->stop_recording(0);
        image_mutex[0].lock();
        vid_out[0].release();
        image_mutex[0].unlock();
        QString dest = save_location + "/" + raw_avi.section('/', -1, -1);
        std::thread t(UserPanel::move_to_dest, QString(raw_avi), QString(dest));
        t.detach();
//        QFile::rename(raw_avi, dest);
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        image_mutex[0].lock();
        raw_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_raw.avi");
//        vid_out[0].open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w[0], h[0]), is_color[0]);
        vid_out[0].open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('X', '2', '6', '4'), frame_rate_edit, cv::Size(w[0], h[0]), is_color[0]);
        image_mutex[0].unlock();
    }
    record_original[0] ^= 1;
    ui->SAVE_AVI_BUTTON->setText(record_original[0] ? tr("STOP") : tr("ORI"));
}

void UserPanel::on_SCAN_BUTTON_clicked()
{
    bool start_scan = ui->SCAN_BUTTON->text() == tr("Scan");

//    while (updated);

    if (start_scan) {
        scan_3d = cv::Mat::zeros(h[0], w[0], CV_64F);
        scan_sum = cv::Mat::zeros(h[0], w[0], CV_64F);
        scan_idx = 0;

        scan_ptz_idx = -1;
        scan_tcu_idx = -1;
        scan_ptz_route = scan_config->get_ptz_route();
        scan_tcu_route = scan_config->get_tcu_route();

        rep_freq = scan_config->rep_freq;
        delay_dist = scan_config->starting_delay * dist_ns;
//        scan_stopping_delay = scan_config->ending_delay;
        scan_step = scan_config->step_size_delay * dist_ns;
//        depth_of_view = scan_config->starting_gw * dist_ns;

        if (scan_config->capture_scan_ori || scan_config->capture_scan_res) {
            scan_name = "scan_" + QDateTime::currentDateTime().toString("MMdd_hhmmss");
            if (!QDir(save_location + "/" + scan_name).exists()) {
                QDir().mkdir(save_location + "/" + scan_name);
//                QDir().mkdir(save_location + "/" + scan_name + "/ori_bmp");
//                QDir().mkdir(save_location + "/" + scan_name + "/res_bmp");
            }

            QFile params(save_location + "/" + scan_name + "/scan_params");
            params.open(QIODevice::WriteOnly);
            params.write(QString::asprintf("starting delay:     %06d ns\n"
                                           "ending delay:       %06d ns\n"
                                           "frames count:       %06d\n"
                                           "stepping size:      %.2f ns\n"
                                           "repeated frequency: %06d Hz\n"
                                           "laser width:        %06d ns\n"
                                           "gate width:         %06d ns\n"
                                           "MCP:                %04d",
                                           scan_config->starting_delay, scan_config->ending_delay, scan_config->frame_count, scan_config->step_size_delay,
//                                           (int)(rep_freq * 1000), (int)laser_width, scan_config->starting_gw, ptr_tcu->mcp).toUtf8());
                                           (int)(rep_freq * 1000), (int)laser_width, std::round(p_tcu->get(TCU::GATE_WIDTH_A)), std::round(p_tcu->get(TCU::MCP))).toUtf8());
            qDebug() << "scan param" << p_tcu->get(TCU::DELAY_A) << std::round(p_tcu->get(TCU::DELAY_A));
            qDebug() << "scan param" << p_tcu->get(TCU::GATE_WIDTH_A) << std::round(p_tcu->get(TCU::GATE_WIDTH_A));
            params.close();
        }

        on_CONTINUE_SCAN_BUTTON_clicked();
    }
    else {
        emit update_scan(false);
        scan = false;

//        ui->CONTINUE_SCAN_BUTTON->show();
//        ui->RESTART_SCAN_BUTTON->show();
        ui->SCAN_BUTTON->setText(tr("Scan"));

//        for (uint i = scan_q.size(); i; i--) qDebug("dist %f", scan_q.front()), scan_q.pop_front();

//        scan_3d = scan_3d.mul(1 / scan_sum);
//        scan_3d *= 2.25;
//        cv::Mat scan_result_3d;
//        ImageProc::paint_3d(scan_3d, scan_result_3d, 0, scan_config->starting_delay * dist_ns, scan_config->ending_delay * dist_ns);
//        cv::imwrite((TEMP_SAVE_LOCATION + "/" + scan_name + "_result.bmp").toLatin1().constData(), scan_result_3d);
//        QFile::rename(TEMP_SAVE_LOCATION + "/" + scan_name + "_result.bmp", save_location + "/" + scan_name + "/result.bmp");
    }

//    ui->SCAN_BUTTON->setText(start_scan ? tr("Pause") : tr("Scan"));
}

void UserPanel::on_CONTINUE_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);

//    ui->CONTINUE_SCAN_BUTTON->hide();
//    ui->RESTART_SCAN_BUTTON->hide();
    ui->SCAN_BUTTON->setText(tr("Pause"));

//    update_delay();
//    update_gate_width();
//    ptr_tcu->communicate_display(convert_to_send_tcu(0x00, (uint)(1.25e5 / ptr_tcu->rep_freq)), 7, 1, false);
//    ptr_tcu->set_user_param(TCUThread::REPEATED_FREQ, rep_freq);
//    emit send_double_tcu_msg(TCUThread::REPEATED_FREQ, rep_freq);
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
//    ui->RESTART_SCAN_BUTTON->setEnabled(!scan);
    ui->RESTART_SCAN_BUTTON->setEnabled(false);

//    if (show) {
//        ui->CONTINUE_SCAN_BUTTON->show();
//        ui->RESTART_SCAN_BUTTON->show();
//    }
//    else {
//        ui->CONTINUE_SCAN_BUTTON->hide();
//        ui->RESTART_SCAN_BUTTON->hide();
//    }
}

void UserPanel::on_FRAME_AVG_CHECK_stateChanged(int arg1)
{
//    if (arg1) {
//        for (int i = 0; i < 3; i++) {
//            seq_sum[i].release();
//            frame_a_sum[i].release();
//            frame_b_sum[i].release();
//            for (cv::Mat &m: seq[i]) m.release();
//            seq_idx[i] = 0;
//        }
//    }
    calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 4 + 4;
}

void UserPanel::on_SAVE_RESULT_BUTTON_clicked()
{
    if (!QDir(save_location + "/res_bmp").exists()) QDir().mkdir(save_location + "/res_bmp");
    if (device_type == -1 || !pref->consecutive_capture) save_modified[0] = 1;
    else{
        save_modified[0] ^= 1;
        ui->SAVE_RESULT_BUTTON->setText(save_modified[0] ? tr("STOP") : tr("RES"));
        pref->ui->CONSECUTIVE_CAPTURE_CHK->setEnabled(!save_modified[0]);
    }
}

void UserPanel::on_IRIS_OPEN_BTN_pressed()
{
    ui->IRIS_OPEN_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x20, 0x00, 0x00, 0x22}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x01, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04}, 7), 7, 1, false);

//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03}, 7), 7, 0, false);
//    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x20, 0x00, 0x00, 0x22}, 7), 7, 0, false);
//    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x01, 0x00, 0x00, 0x00, 0x03}, 7), 7, 0, false);
//    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04}, 7), 7, 0, false);

//    ptr_lens->lens_control(LensThread::RADIUS_UP, nullptr);
    emit send_lens_msg(Lens::RADIUS_UP);
}

void UserPanel::on_IRIS_CLOSE_BTN_pressed()
{
    ui->IRIS_CLOSE_BTN->setText("x");

//    (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x04, 0x00, 0x00, 0x00, 0x05}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x40, 0x00, 0x00, 0x42}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x80, 0x00, 0x00, 0x82}, 7), 7, 1, false);
//    if (multi_laser_lenses) (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x04, 0x00, 0x00, 0x00, 0x06}, 7), 7, 1, false);

//    ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x04, 0x00, 0x00, 0x00, 0x05}, 7), 7, 1, false);
//    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x40, 0x00, 0x00, 0x42}, 7), 7, 0, false);
//    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x80, 0x00, 0x00, 0x82}, 7), 7, 0, false);
//    if (multi_laser_lenses) ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x02, 0x04, 0x00, 0x00, 0x00, 0x06}, 7), 7, 0, false);

//    ptr_lens->lens_control(LensThread::RADIUS_DOWN, nullptr);
    emit send_lens_msg(Lens::RADIUS_DOWN);
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
    if (ui->LASER_BTN->text() == tr("ON")) {
        ui->LASER_BTN->setEnabled(false);
//        ptr_tcu->communicate_display(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 7, 0, false);
        p_tcu->emit send_data(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 0, 0);
        QTimer::singleShot(4000, this, SLOT(start_laser()));
    }
    else {
//        if (serial_port[3] && serial_port[3]->isOpen()) {
//            serial_port[3]->write("OFF\r", 4);
//            serial_port[3]->waitForBytesWritten(100);
//            QThread::msleep(100);
//            serial_port[3]->readAll();
//        }
//        ptr_laser->communicate_display(QByteArray("OFF\r"), 4, 0, true, false);
//        p_laser->emit send_data(QByteArray("OFF\r"), 0, 0);
//        emit send_laser_msg("OFF\r");
//        qDebug() << QString("OFF\r");
        if (ui->FIRE_LASER_BTN->text() == tr("STOP")) ui->FIRE_LASER_BTN->click();
//        ptr_tcu->communicate_display(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x02, 0x99}, 7), 7, 0, false);
        p_tcu->send_data(generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x02, 0x99}, 7), 0, 0);

        ui->LASER_BTN->setText(tr("ON"));
        ui->CURRENT_EDIT->setEnabled(false);
        ui->FIRE_LASER_BTN->setEnabled(false);
        ui->SIMPLE_LASER_CHK->setEnabled(false);
    }
#else
    pref->ui->LASER_ENABLE_CHK->click();
    ui->LASER_BTN->setText(ui->LASER_BTN->text() == tr("ON") ? tr("OFF") : tr("ON"));
#endif
}

void UserPanel::on_GET_LENS_PARAM_BTN_clicked()
{
//    QByteArray read = (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56}, 7), 7, 7, true);
//    QByteArray read = ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56}, 7), 7, 7, true);
//    zoom = (uint(read[4] & 0xFF) << 8) + uint(read[5] & 0xFF);

//    read = (pref->share_port && serial_port[0]->isOpen() ? (ControlPort*) ptr_tcu : ptr_lens)->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57}, 7), 7, 7, true);
//    read = ptr_lens->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57}, 7), 7, 7, true);
//    focus = (uint(read[4] & 0xFF) << 8) + uint(read[5] & 0xFF);

//    ptr_lens->lens_control(LensThread::ZOOM_POS, &zoom);
//    ptr_lens->lens_control(LensThread::FOCUS_POS, &focus);
    emit send_lens_msg(Lens::ZOOM_POS);
    emit send_lens_msg(Lens::FOCUS_POS);
    emit send_lens_msg(Lens::LASER_RADIUS);

//    ui->ZOOM_EDIT->setText(QString::asprintf("%d", zoom));
//    ui->FOCUS_EDIT->setText(QString::asprintf("%d", focus));
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

void UserPanel::on_MISC_RADIO_1_clicked()
{
    alt_display_option = ui->MISC_OPTION_1->currentIndex() + 1;
    ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
}

void UserPanel::on_MISC_RADIO_2_clicked()
{
    alt_display_option = ui->MISC_OPTION_2->currentIndex() + 1;
    ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
}

void UserPanel::on_MISC_RADIO_3_clicked()
{
    alt_display_option = ui->MISC_OPTION_3->currentIndex() + 1;
    ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
}

void UserPanel::on_MISC_OPTION_1_currentIndexChanged(int index)
{
    alt_display_option = index + 1;
    ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
    ui->MISC_RADIO_1->setChecked(true);
}

void UserPanel::on_MISC_OPTION_2_currentIndexChanged(int index)
{
    alt_display_option = index + 1;
    ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
    ui->MISC_RADIO_2->setChecked(true);
}

void UserPanel::on_MISC_OPTION_3_currentIndexChanged(int index)
{
    alt_display_option = index + 1;
    ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
    ui->MISC_RADIO_3->setChecked(true);
}
#if 0
void UserPanel::on_COM_DATA_RADIO_clicked()
{
    display_option = 0;
    ui->DATA_EXCHANGE->show();
    ui->PLUGIN_DISPLAY_1->hide();
    ui->PTZ_GRP->hide();
}

void UserPanel::on_PTZ_RADIO_clicked()
{
    display_option = 2;
    ui->DATA_EXCHANGE->hide();
    ui->PLUGIN_DISPLAY_1->hide();
    ui->PTZ_GRP->show();
}

void UserPanel::on_PLUGIN_RADIO_clicked()
{
    display_option = 3;
    ui->DATA_EXCHANGE->hide();
    ui->PLUGIN_DISPLAY_1->show();
    ui->PTZ_GRP->hide();
}
#endif
void UserPanel::screenshot()
{
//    image_mutex[0].lock();
    QPixmap screenshot = window()->grab();
    screenshot.save(save_location + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    QApplication::clipboard()->setPixmap(screenshot);
//    image_mutex[0].unlock();
}

void UserPanel::on_ZOOM_TOOL_clicked()
{
    ui->SELECT_TOOL->setChecked(false);
    ui->PTZ_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->mode = 0;
    ui->SOURCE_DISPLAY->setContextMenuPolicy(Qt::PreventContextMenu);
}

void UserPanel::on_SELECT_TOOL_clicked()
{
    ui->ZOOM_TOOL->setChecked(false);
    ui->PTZ_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->mode = 1;
    ui->SOURCE_DISPLAY->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void UserPanel::on_PTZ_TOOL_clicked()
{
    ui->ZOOM_TOOL->setChecked(false);
    ui->SELECT_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->mode = 2;
    ui->SOURCE_DISPLAY->setContextMenuPolicy(Qt::PreventContextMenu);
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
    MvCam *temp_mvcam = dynamic_cast<MvCam*>(curr_cam);
    if (!temp_mvcam) return;
    int binning = arg1 ? 2 : 1;
    temp_mvcam->binning(false, &binning);
    temp_mvcam->frame_size(true, &w[0], &h[0]);
    qInfo("frame w: %d, h: %d", w[0], h[0]);
    status_bar->img_resolution->set_text(QString::asprintf("%d x %d", w[0], h[0]));
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
        int write_retries = 5; // 5 * 10ms = 50ms max
        while (serial_port[0]->waitForBytesWritten(10) && --write_retries > 0) ;

        int read_retries = 2; // 2 * 100ms = 200ms max
        while (serial_port[0]->waitForReadyRead(100) && --read_retries > 0) ;

        QByteArray read = serial_port[0]->readAll();
        for (char i = 0; i < read.length(); i++) str_r += QString::asprintf(" %02X", i < 0 ? 0 : (uchar)read[i]);
        emit append_text(str_r);

        QThread().msleep(5);
    }
    f.close();
}

void UserPanel::config_gatewidth(QString filename)
{
    QFile config_file(filename);
    if (!config_file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "PROMPT", tr("cannot open config file"));
        return;
    }
    
    QByteArray line;
//    for (int i = 0; i < 100; i++) ptr_tcu->gw_lut[i] = -1;
    while (!(line = config_file.readLine(128)).isEmpty()) {
        line = line.simplified();
        // ori: user input, res: output to tcu
        int ori = line.left(line.indexOf(',')).toInt();
        int res = line.right(line.length() - line.indexOf(',') - 1).toInt();
//        ptr_tcu->gw_lut[ori] = res;
    }
//    ptr_tcu->use_gw_lut = true;
    config_file.close();
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

//    ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 1, false);
}

void UserPanel::ptz_button_pressed(int id) {
//    qDebug("%dp", id);
#if 0
    switch(id){
    case 0: send_ctrl_cmd(0x08); send_ctrl_cmd(0x04); break;
    case 1: send_ctrl_cmd(0x08);                      break;
    case 2: send_ctrl_cmd(0x08); send_ctrl_cmd(0x02); break;
    case 3: send_ctrl_cmd(0x04);                      break;
    case 4: {
        if (QMessageBox::warning(this, "PTZ", "Initialize?", QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel) == QMessageBox::StandardButton::Ok) {
            uchar buffer_out[7] = {0xFF, 0x01, 0x00, 0x07, 0x00, 0x77, 0x7F};
            ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 1, false);
        }
        break;
    }
    case 5: send_ctrl_cmd(0x02);                      break;
    case 6: send_ctrl_cmd(0x10); send_ctrl_cmd(0x04); break;
    case 7: send_ctrl_cmd(0x10);                      break;
    case 8: send_ctrl_cmd(0x10); send_ctrl_cmd(0x02); break;
    default:                                          break;
    }
#endif
//    ptr_ptz->ptz_control(static_cast<PTZThread::PARAMS>(id + 1));

    if (id == 4) if (QMessageBox::warning(nullptr, "PTZ", tr("Initialize?"), QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel) != QMessageBox::StandardButton::Ok) return;
    if (!pref->gcan) emit send_ptz_msg(id + 1);
    else {
        switch (id) {
        case 0:
            p_usbcan->emit control(USBCAN::LEFT, ui->PTZ_SPEED_SLIDER->value());
            p_usbcan->emit control(USBCAN::UP, ui->PTZ_SPEED_SLIDER->value());
            break;
        case 1:
            p_usbcan->emit control(USBCAN::UP, ui->PTZ_SPEED_SLIDER->value());
            break;
        case 2:
            p_usbcan->emit control(USBCAN::RIGHT, ui->PTZ_SPEED_SLIDER->value());
            p_usbcan->emit control(USBCAN::UP, ui->PTZ_SPEED_SLIDER->value());
            break;
        case 3:
            p_usbcan->emit control(USBCAN::LEFT, ui->PTZ_SPEED_SLIDER->value());
            break;
        case 4:
            p_usbcan->emit transmit(USBCAN::OFF);
            QThread::msleep(1000);
            p_usbcan->emit transmit(USBCAN::SELF_CHECK);
            QThread::msleep(1000);
            p_usbcan->emit transmit(USBCAN::LOCK);
            QThread::msleep(1000);
            break;
        case 5:
            p_usbcan->emit control(USBCAN::RIGHT, ui->PTZ_SPEED_SLIDER->value());
            break;
        case 6:
            p_usbcan->emit control(USBCAN::LEFT, ui->PTZ_SPEED_SLIDER->value());
            p_usbcan->emit control(USBCAN::DOWN, ui->PTZ_SPEED_SLIDER->value());
            break;
        case 7:
            p_usbcan->emit control(USBCAN::DOWN, ui->PTZ_SPEED_SLIDER->value());
            break;
        case 8:
            p_usbcan->emit control(USBCAN::RIGHT, ui->PTZ_SPEED_SLIDER->value());
            p_usbcan->emit control(USBCAN::DOWN, ui->PTZ_SPEED_SLIDER->value());
            break;
        default: break;
        }
    }
}

void UserPanel::ptz_button_released(int id) {
//    qDebug("%dr", id);
    if (id == 4) return;
//    ptr_ptz->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
//    ptr_ptz->ptz_control(PTZThread::STOP);
    if (!pref->gcan) emit send_ptz_msg(PTZ::STOP);
    else             p_usbcan->emit control(USBCAN::STOP, 0);
}

void UserPanel::on_PTZ_SPEED_SLIDER_valueChanged(int value)
{
    emit send_ptz_msg(PTZ::SPEED, ptz_speed = value);
    ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
}

void UserPanel::on_PTZ_SPEED_EDIT_editingFinished()
{
    ptz_speed = ui->PTZ_SPEED_EDIT->text().toInt();
    if (ptz_speed > 64) ptz_speed = 64;
    if (ptz_speed < 1)  ptz_speed = 1;
    emit send_ptz_msg(PTZ::SPEED, ptz_speed);
    ui->PTZ_SPEED_SLIDER->setValue(ptz_speed);
    ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
}

void UserPanel::on_GET_ANGLE_BTN_clicked()
{
#if 0
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
#endif
//    ptr_ptz->ptz_control(PTZThread::ANGLE_H, &angle_h);
//    ptr_ptz->ptz_control(PTZThread::ANGLE_V, &angle_v);
    emit send_ptz_msg(PTZ::ANGLE_H);
    emit send_ptz_msg(PTZ::ANGLE_V);
}

void UserPanel::set_ptz_angle()
{
#if 0
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
    if (angle < 0) angle += 36000;
    buffer_out[3] = 0x4D;
    buffer_out[4] = (angle >> 8) & 0xFF;
    buffer_out[5] = angle & 0xFF;
    sum = 0;
    for (int i = 1; i < 6; i++) sum += buffer_out[i];
    buffer_out[6] = sum & 0xFF;
    ptr_ptz->communicate_display(QByteArray((char*)buffer_out, 7), 7, 0, false);
    ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", angle_v));
#endif
//    ptr_ptz->ptz_control(PTZThread::SET_H, &angle_h);
//    ptr_ptz->ptz_control(PTZThread::SET_V, &angle_v);

    if (!pref->gcan) {
        emit send_ptz_msg(PTZ::SET_H, angle_h);
        emit send_ptz_msg(PTZ::SET_V, angle_v);
    }
    else {
        p_usbcan->emit control(USBCAN::POSITION, angle_h);
        p_usbcan->emit transmit(USBCAN::POSITION);
        p_usbcan->emit control(USBCAN::PITCH, angle_v);
        p_usbcan->emit transmit(USBCAN::PITCH);
    }
}

void UserPanel::on_STOP_BTN_clicked()
{
//    ptr_ptz->communicate_display(generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
//    ptr_ptz->ptz_control(PTZThread::STOP);
    if (!pref->gcan) emit send_ptz_msg(PTZ::STOP);
    else             p_usbcan->emit control(USBCAN::STOP, 0);
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
    
    // Ensure horizontal angle is always positive (0 to 360)
    angle_h = fmod(angle_h + 360.0, 360.0);

    set_ptz_angle();
}

void UserPanel::alt_display_control(int cmd)
{
    int display_idx = (cmd & 1) + 1;
    if (!grab_thread_state[display_idx]) return;
    switch (cmd / 2) {
    case 0: {
//        if (record_original[display_idx]) on_SAVE_AVI_BUTTON_clicked();
//        if (record_modified[display_idx]) on_SAVE_FINAL_BUTTON_clicked();

        grab_image[display_idx] = false;
        if (h_grab_thread[display_idx]) {
            h_grab_thread[display_idx]->quit();
            h_grab_thread[display_idx]->wait();
            delete h_grab_thread[display_idx];
            h_grab_thread[display_idx] = NULL;
        }
        QTimer::singleShot(100, displays[display_idx], SLOT(clear()));

        break;
    }
    case 1: save_modified[display_idx] = 1; break;
    case 2: {
        static QString raw_avi[2];
        if (record_modified[display_idx]) {
            image_mutex[display_idx].lock();
            vid_out[display_idx + 1].release();
            image_mutex[display_idx].unlock();
            QString dest = save_location + "/" + raw_avi[display_idx - 1].section('/', -1, -1);
            std::thread t(UserPanel::move_to_dest, QString(raw_avi[display_idx - 1]), QString(dest));
            t.detach();
        }
        else {
            image_mutex[display_idx].lock();
            raw_avi[display_idx - 1] = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_raw_alt" + QString::number(display_idx) + ".avi");
//            vid_out[display_idx + 1].open(raw_avi[display_idx - 1].toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w[display_idx], h[display_idx]), is_color[display_idx]);
            vid_out[display_idx + 1].open(raw_avi[display_idx - 1].toLatin1().data(), cv::VideoWriter::fourcc('X', '2', '6', '4'), frame_rate_edit, cv::Size(w[display_idx], h[display_idx]), is_color[display_idx]);
            image_mutex[display_idx].unlock();
        }
        record_modified[display_idx] ^= 1;

        alt_ctrl_grp->button(cmd)->setText(record_modified[display_idx] ? "STOP" : "REC");
        break;
    }
    default: break;
    }
}

void UserPanel::on_DUAL_LIGHT_BTN_clicked()
{
    QPluginLoader pluginLoader("plugins/hik_control/plugin_hik_control.dll");
    QObject *plugin = pluginLoader.instance();
    qDebug() << plugin;
    if (plugin) {
        pluginInterface = qobject_cast<PluginInterface *>(plugin);
//        pluginInterface = (PluginInterface *)plugin;
        qDebug() << pluginInterface;
        if (pluginInterface) {
            pluginInterface->create(ui->PLUGIN_DISPLAY_1);
            pluginInterface->start();
        }
    }
}

void UserPanel::on_RESET_3D_BTN_clicked()
{
    frame_a_3d ^= 1;
//    for (cv::Mat &m: frame_a_sum) m = 0;
//    for (cv::Mat &m: frame_b_sum) m = 0;
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
    if (ui->FIRE_LASER_BTN->text() == tr("FIRE")) {
//        serial_port[3]->write("MODE:STBY 0\r", 12);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
//        ptr_laser->communicate_display(QByteArray("MODE:STBY 0\r"), 12, 0, true, false);
//        ptr_laser->send_data(PortData{QByteArray("MODE:STBY 0\r"), 12, 0, true, false});
//        p_laser->emit send_data(QByteArray("MODE:STBY 0\r"), 0, 100);
//        emit send_laser_msg("MODE:STBY 0\r");
//        qDebug() << QString("MODE:STBY 0\r");

//        serial_port[3]->write("ON\r", 3);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
//        ptr_laser->communicate_display(QByteArray("ON\r"), 3, 0, true, false);
//        ptr_laser->send_data(PortData{QByteArray("ON\r"), 3, 0, true, false});
//        p_laser->emit send_data(QByteArray("ON\r"), 0, 100);
        emit send_laser_msg("ON\r");
        qDebug() << QString("ON\r");

        if (!pref->ui->LASER_ENABLE_CHK->isChecked()) pref->ui->LASER_ENABLE_CHK->click();

        ui->FIRE_LASER_BTN->setText(tr("STOP"));
        ui->SIMPLE_LASER_CHK->setChecked(true);
    } else {
//        serial_port[3]->write("OFF\r", 4);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
//        ptr_laser->communicate_display(QByteArray("OFF\r"), 4, 0, true, false);
//        ptr_laser->send_data(PortData{QByteArray("OFF\r"), 4, 0, true, false});
//        p_laser->emit send_data(QByteArray("OFF\r"), 0, 100);
        emit send_laser_msg("OFF\r");
        qDebug() << QString("OFF\r");

//        serial_port[3]->write("MODE:STBY 1\r", 12);
//        serial_port[3]->waitForBytesWritten(100);
//        QThread::msleep(100);
//        serial_port[3]->readAll();
//        ptr_laser->communicate_display(QByteArray("MODE:STBY 1\r"), 12, 0, true, false);
//        ptr_laser->send_data(PortData{QByteArray("MODE:STBY 1\r"), 12, 0, true, false});
//        p_laser->emit send_data(QByteArray("MODE:STBY 1\r"), 0, 100);
//        emit send_laser_msg("MODE:STBY 1\r");
//        qDebug() << QString("MODE:STBY 1\r");

        if (pref->ui->LASER_ENABLE_CHK->isChecked()) pref->ui->LASER_ENABLE_CHK->click();

        ui->FIRE_LASER_BTN->setText(tr("FIRE"));
        ui->SIMPLE_LASER_CHK->setChecked(false);
    }
}

void UserPanel::on_IMG_REGION_BTN_clicked()
{
    int max_w = 0, max_h = 0, new_w = 0, new_h = 0, new_x = 0, new_y = 0;
    int inc_w = 1, inc_h = 1, inc_x = 1, inc_y = 1;
    curr_cam->get_max_frame_size(&max_w, &max_h);
    curr_cam->frame_size(true, &new_w, &new_h, &inc_w, &inc_h);
    curr_cam->frame_offset(true, &new_x, &new_y, &inc_x, &inc_y);
    new_w = ui->SHAPE_INFO->pair.x();
    new_h = ui->SHAPE_INFO->pair.y();
    new_x += ui->START_COORD->pair.x();
    new_y += ui->START_COORD->pair.y();
    if (new_w == 0) new_w = max_w;
    if (new_h == 0) new_h = max_h;

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
        if (curr_cam) {
            curr_cam->frame_size(false, &new_w, &new_h);
            curr_cam->frame_offset(false, &new_x, &new_y);
        }
        if (device_on && curr_cam) curr_cam->frame_size(true, &w[0], &h[0]);
        status_bar->img_resolution->set_text(QString::asprintf("%d x %d", w[0], h[0]));
        QResizeEvent e(this->size(), this->size());
        resizeEvent(&e);
        if (was_playing) ui->START_GRABBING_BUTTON->click();
    }
}

void UserPanel::on_SENSOR_TAPS_BTN_clicked()
{
    EbusCam *cam = static_cast<EbusCam *>(curr_cam);
    if (!cam) return;
    static int taps = 1;
    taps ^= 1;
    cam->set_sensor_digitization_taps(taps + 1);
    ui->SENSOR_TAPS_BTN->setIcon(QIcon(":/nums/" + QString::number(taps + 1)));
}

void UserPanel::on_SWITCH_TCU_UI_BTN_clicked()
{
    static int show_diff = 1;
    if (show_diff) {
        ui->DELAY_B_GRP->hide();
        ui->GATE_WIDTH_B_GRP->hide();
        ui->DELAY_DIFF_GRP->show();
        ui->GATE_WIDTH_DIFF_GRP->show();
    }
    else {
        ui->DELAY_B_GRP->show();
        ui->GATE_WIDTH_B_GRP->show();
        ui->DELAY_DIFF_GRP->hide();
        ui->GATE_WIDTH_DIFF_GRP->hide();
    }
    show_diff ^= 1;
}

void UserPanel::on_SIMPLE_LASER_CHK_clicked()
{
#ifdef LVTONG
    if (!qobject_cast<QCheckBox *>(sender())) return;
    ui->FIRE_LASER_BTN->click();
#else //LVTONG
    pref->ui->LASER_CHK_1->click();
#endif
}

void UserPanel::on_AUTO_MCP_CHK_clicked()
{
    pref->ui->AUTO_MCP_CHK->click();
}

void UserPanel::on_PSEUDOCOLOR_CHK_stateChanged(int arg1)
{
    image_mutex[0].lock();
    pseudocolor[0] = arg1;
    image_mutex[0].unlock();
}

