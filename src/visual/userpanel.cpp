#include "userpanel.h"
#include "ui_user_panel.h"
#include "ui_preferences.h"
#include "util/constants.h"

static bool throttle_check(QElapsedTimer &t)
{
    if (t.isValid() && t.elapsed() < THROTTLE_MS) return true;
    t.restart();
    return false;
}

static void setup_slider(QSlider *slider, int min, int max, int step, int page, int initial)
{
    slider->setMinimum(min);
    slider->setMaximum(max);
    slider->setSingleStep(step);
    slider->setPageStep(page);
    slider->setValue(initial);
}

GrabThread::GrabThread(PipelineProcessor *pipeline, int idx)
{
    m_pipeline = pipeline;
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
    m_pipeline->run(&_display_idx);
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
    }
}

UserPanel::UserPanel(QWidget *parent) :
    QMainWindow(parent),
    mouse_pressed(false),
    ui(new Ui::UserPanel),
    status_bar(NULL),
    pref(NULL),
    m_device_mgr(nullptr),
    m_tcu_ctrl(nullptr),
    m_lens_ctrl(nullptr),
#ifdef DISTANCE_3D_VIEW
    view_3d(NULL),
#endif //DISTANCE_3D_VIEW
    m_laser_ctrl(nullptr),
    m_rf_ctrl(nullptr),
    m_scan_ctrl(nullptr),
    m_aux_panel(nullptr),
    scan_config(NULL),
    laser_settings(NULL),
    config(NULL),
    aliasing(NULL),
#if SIMPLE_UI
    gain_analog_edit(23),
#else //SIMPLE_UI
    fw_display{NULL},
#endif //SIMPLE_UI
    secondary_display(NULL),
    simple_ui(false),
    calc_avg_option(5),
    trigger_by_software(false),
    curr_cam(NULL),
    time_exposure_edit(95000),
    gain_analog_edit(0),
    frame_rate_edit(10),
    q_fps_calc(5),
    device_type(0),
    device_on(false),
    start_grabbing(false),
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
    video_thread_state(false),
    h_joystick_thread(NULL),
    // multi_laser_lenses removed (dead variable)
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
    // lens_adjust_ongoing → LensController
    ptz_adjust_ongoing(0),
    lang(0),
    tp(THREAD_POOL_SIZE),
    alt_ctrl_grp(NULL)
{
    for (int i = 0; i < 3; ++i) {
        record_original[i].store(false);
        record_modified[i].store(false);
        save_original[i].store(false);
        save_modified[i].store(false);
        grab_thread_state[i].store(false);
    }

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

    QFont temp_f(consolas);
    temp_f.setPixelSize(11);

    // connect title bar to the main window (UserPanel*)
    ui->TITLE->setup(this);
    pref = ui->TITLE->preferences;
    pref->init();
    scan_config = ui->TITLE->scan_config;

    // initialize config system
    config = new Config(this);
    pref->set_config(config);

//    SerialServer *s = new SerialServer();
//    s->show();

    // Language will be set from JSON config during auto-load
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
                QMutexLocker lk(&m_roi_mutex);
                list_roi.push_back(roi);
                user_mask[0](roi).setTo(255);
            });
    connect(ui->SOURCE_DISPLAY, &Display::clear_roi, this,
            [this](){
                if (!grab_image[0]) return;
                QMutexLocker lk(&m_roi_mutex);
                std::vector<cv::Rect>().swap(list_roi);
                user_mask[0] = 0;
            });
    // set_hist_pixmap connection moved to PipelineProcessor signal wiring below

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
    calc_avg_options->addItem("ECC");
    calc_avg_options->setCurrentIndex(0);

    // COM label/edit arrays → DeviceManager

//    setup_com(com + 0, 0, com_edit[0]->text(), 9600);
//    setup_com(com + 1, 1, com_edit[1]->text(), 115200);
//    setup_com(com + 2, 2, com_edit[2]->text(), 9600);
//    setup_com(com + 3, 3, com_edit[3]->text(), 9600);

    // - set up sliders
    setup_slider(ui->MCP_SLIDER, 0, MAX_MCP_8BIT, 1, 10, 0);
    connect(ui->MCP_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_mcp(int)));
    connect(ui->MCP_SLIDER, &QSlider::sliderMoved, this, [this](int){ if (ui->AUTO_MCP_CHK->isChecked()) ui->AUTO_MCP_CHK->click(); });

    setup_slider(ui->DELAY_SLIDER, 0, config->get_data().tcu.max_dist, 10, 100, 0);
    connect(ui->DELAY_SLIDER, SIGNAL(sliderMoved(int)), SLOT(change_delay(int)));

    setup_slider(ui->GW_SLIDER, 0, config->get_data().tcu.max_dov, 5, 25, 0);
    connect(ui->GW_SLIDER, SIGNAL(sliderMoved(int)), SLOT(change_gatewidth(int)));

    setup_slider(ui->FOCUS_SPEED_SLIDER, 1, 63, 1, 4, 31);
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
    // update_scan → ScanController::enable_scan_options wired in ScanController::init()
    // update_delay_in_thread, update_mcp_in_thread, save_screenshot_signal,
    // finish_scan_signal, set_model_list_enabled connections moved to PipelineProcessor signal wiring below

    // task_queue_full connection moved to PipelineProcessor signal wiring below

    connect(ui->RANGE_THRESH_EDIT, &QLineEdit::textChanged, this,
            [this](QString arg1){
                config->get_data().image_proc.lower_3d_thresh = arg1.toFloat() / (1 << pixel_depth[0]);
                pref->ui->LOWER_3D_THRESH_EDT->setText(QString::number(config->get_data().image_proc.lower_3d_thresh, 'f', 3));
    });

    setup_slider(ui->BRIGHTNESS_SLIDER, -5, 5, 1, 2, 0);
    ui->BRIGHTNESS_SLIDER->setTickInterval(5);

    setup_slider(ui->CONTRAST_SLIDER, -10, 10, 1, 3, 0);
    ui->CONTRAST_SLIDER->setTickInterval(5);

    setup_slider(ui->GAMMA_SLIDER, 0, 20, 1, 3, 10);
    ui->GAMMA_SLIDER->setTickInterval(5);

    // ProcessingParams: connect widget signals to keep snapshot up-to-date
    connect(ui->FRAME_AVG_CHECK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->FRAME_AVG_OPTIONS, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){ update_processing_params(); });
    connect(ui->IMG_3D_CHECK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->IMG_ENHANCE_CHECK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->ENHANCE_OPTIONS, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){ update_processing_params(); });
    connect(ui->BRIGHTNESS_SLIDER, &QSlider::valueChanged, this, [this](int){ update_processing_params(); });
    connect(ui->CONTRAST_SLIDER, &QSlider::valueChanged, this, [this](int){ update_processing_params(); });
    connect(ui->GAMMA_SLIDER, &QSlider::valueChanged, this, [this](int){ update_processing_params(); });
    connect(ui->PSEUDOCOLOR_CHK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->INFO_CHECK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->CENTER_CHECK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->SELECT_TOOL, &QPushButton::toggled, this, [this](bool){ update_processing_params(); });
    connect(ui->DUAL_DISPLAY_CHK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    // MCP slider focus changes (also picks up hasFocus on any focus switch)
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget*){ update_processing_params(); });
    // Pref text field changes
    connect(pref->ui->CUSTOM_3D_DELAY_EDT, &QLineEdit::textChanged, this, [this](const QString&){ update_processing_params(); });
    connect(pref->ui->CUSTOM_3D_GW_EDT, &QLineEdit::textChanged, this, [this](const QString&){ update_processing_params(); });
    connect(pref->ui->CUSTOM_INFO_EDT, &QLineEdit::textChanged, this, [this](const QString&){ update_processing_params(); });
    // packet_lost_updated connection moved to PipelineProcessor signal wiring below
    // initial snapshot
    update_processing_params();

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
            case 3: m_device_mgr->point_ptz_to_target(pos, disp->width(), disp->height()); break;
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

    // PTZ button group, VID camera buttons, VID toggle buttons → DeviceManager

    ui->RESET_3D_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    pref->ui->REFRESH_AVAILABLE_PORTS_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    ui->SWITCH_TCU_UI_BTN->setIcon(QIcon(":/tools/" + QString(app_theme ? "light" : "dark") + "/switch"));

    // PTZ speed slider → DeviceManager

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

    // AuxPanelManager: MISC_OPTION combos, display_grp, radio buttons
    m_aux_panel = new AuxPanelManager(this);
    {
        QStringList alt_options_str;
        alt_options_str << "DATA" << "HIST" << "PTZ" << "ALT" << "ADDON" << "VID" << "YOLO";
        ui->MISC_OPTION_1->addItems(alt_options_str);
        ui->MISC_OPTION_2->addItems(alt_options_str);
        ui->MISC_OPTION_3->addItems(alt_options_str);
        ui->MISC_OPTION_1->setCurrentIndex(0);
        ui->MISC_OPTION_2->setCurrentIndex(2);
        ui->MISC_OPTION_3->setCurrentIndex(5);

        QFont misc_f("consolas", 8);
        connect(ui->MISC_OPTION_1, SIGNAL(selected()), ui->MISC_RADIO_1, SLOT(click()));
        connect(ui->MISC_OPTION_2, SIGNAL(selected()), ui->MISC_RADIO_2, SLOT(click()));
        connect(ui->MISC_OPTION_3, SIGNAL(selected()), ui->MISC_RADIO_3, SLOT(click()));
        ui->MISC_OPTION_1->view()->setFont(misc_f);
        ui->MISC_OPTION_2->view()->setFont(misc_f);
        ui->MISC_OPTION_3->view()->setFont(misc_f);

        QButtonGroup *display_grp = new QButtonGroup(this);
        display_grp->addButton(ui->MISC_RADIO_1);
        display_grp->addButton(ui->MISC_RADIO_2);
        display_grp->addButton(ui->MISC_RADIO_3);
        display_grp->setExclusive(true);
        ui->MISC_RADIO_1->setChecked(true);
        ui->MISC_DISPLAY->setCurrentIndex(1);

        connect(m_aux_panel, &AuxPanelManager::display_page_changed, this, [this](int page) {
            ui->MISC_DISPLAY->setCurrentIndex(page);
        });
    }

    ui->DATA_EXCHANGE->document()->setMaximumBlockCount(200);
    ui->DATA_EXCHANGE->setFont(temp_f);
    ui->FILE_PATH_EDIT->setFont(consolas);

    // connect status bar to the main window
    status_bar = ui->STATUS;

    m_device_mgr = new DeviceManager(config, this);
    m_device_mgr->init();

    // Create TCUController
    m_tcu_ctrl = new TCUController(config, m_device_mgr, this);
    m_tcu_ctrl->init(scan_config, aliasing);

    // Forward DeviceManager signals
    connect(m_device_mgr, &DeviceManager::tcu_param_updated, m_tcu_ctrl, &TCUController::update_tcu_params);
    // LensController
    m_lens_ctrl = new LensController(m_device_mgr, this);
    m_lens_ctrl->init(&simple_ui, &lang);
    connect(m_device_mgr, &DeviceManager::lens_param_updated, m_lens_ctrl, &LensController::update_lens_params);
    connect(m_lens_ctrl, &LensController::send_lens_msg, m_device_mgr, &DeviceManager::send_lens_msg);
    // LaserController
    m_laser_ctrl = new LaserController(m_device_mgr, m_lens_ctrl, m_tcu_ctrl, this);
    connect(m_laser_ctrl, &LaserController::send_laser_msg, m_device_mgr, &DeviceManager::send_laser_msg);
    // RFController
    m_rf_ctrl = new RFController(this);
    connect(m_device_mgr, &DeviceManager::distance_updated, m_rf_ctrl, &RFController::update_distance);
    connect(m_device_mgr, &DeviceManager::port_io_log, this, &UserPanel::append_data);
    // Forward TCUController signals to DeviceManager
    connect(m_tcu_ctrl, &TCUController::send_double_tcu_msg, m_device_mgr, &DeviceManager::send_double_tcu_msg);
    connect(m_tcu_ctrl, &TCUController::send_uint_tcu_msg, m_device_mgr, &DeviceManager::send_uint_tcu_msg);
    connect(m_tcu_ctrl, &TCUController::send_laser_msg, m_device_mgr, &DeviceManager::send_laser_msg);
    // Forward UserPanel signals to DeviceManager (non-TCU)
    connect(this, &UserPanel::send_double_tcu_msg, m_device_mgr, &DeviceManager::send_double_tcu_msg);
    connect(this, &UserPanel::send_uint_tcu_msg, m_device_mgr, &DeviceManager::send_uint_tcu_msg);
    connect(this, &UserPanel::send_lens_msg, m_device_mgr, &DeviceManager::send_lens_msg);
    connect(this, &UserPanel::set_lens_pos, m_device_mgr, &DeviceManager::set_lens_pos);
    connect(this, &UserPanel::send_laser_msg, m_device_mgr, &DeviceManager::send_laser_msg);
    connect(this, &UserPanel::send_ptz_msg, m_device_mgr, &DeviceManager::send_ptz_msg);

    // ScanController
    m_scan_ctrl = new ScanController(m_tcu_ctrl, m_device_mgr, this);
    m_scan_ctrl->init(scan_config, &save_location, &w[0], &h[0]);

    // --- Wire controller UI signals ---
    // DeviceManager → UI
    connect(m_device_mgr, &DeviceManager::port_label_style_changed, this, [this](int idx, QString style) {
        QLabel *labels[] = { ui->TCU_COM, ui->LENS_COM, ui->LASER_COM, ui->PTZ_COM, ui->RANGE_COM };
        if (idx >= 0 && idx < 5) labels[idx]->setStyleSheet(style);
    });
    connect(m_device_mgr, &DeviceManager::angle_display_updated, this, [this](QString h, QString v) {
        ui->ANGLE_H_EDIT->setText(h);
        ui->ANGLE_V_EDIT->setText(v);
    });
    connect(m_device_mgr, &DeviceManager::ptz_speed_display_changed, this, [this](int val, QString text) {
        ui->PTZ_SPEED_SLIDER->setValue(val);
        if (!text.isEmpty()) ui->PTZ_SPEED_EDIT->setText(text);
    });
    connect(m_device_mgr, &DeviceManager::vid_camera_btn_text, this, [this](int op, QString text) {
        QPushButton *btn = nullptr;
        switch (op) {
        case UDPPTZ::VL_ZOOM_IN:  btn = ui->VL_ZOOM_IN_BTN;  break;
        case UDPPTZ::VL_ZOOM_OUT: btn = ui->VL_ZOOM_OUT_BTN; break;
        case UDPPTZ::VL_FOCUS_FAR:  btn = ui->VL_FOCUS_FAR_BTN;  break;
        case UDPPTZ::VL_FOCUS_NEAR: btn = ui->VL_FOCUS_NEAR_BTN; break;
        case UDPPTZ::IR_ZOOM_IN:  btn = ui->IR_ZOOM_IN_BTN;  break;
        case UDPPTZ::IR_ZOOM_OUT: btn = ui->IR_ZOOM_OUT_BTN; break;
        case UDPPTZ::IR_FOCUS_FAR:  btn = ui->IR_FOCUS_FAR_BTN;  break;
        case UDPPTZ::IR_FOCUS_NEAR: btn = ui->IR_FOCUS_NEAR_BTN; break;
        }
        if (btn) btn->setText(text);
    });
    connect(m_device_mgr, &DeviceManager::vid_camera_buttons_reset, this, [this]() {
        ui->VL_ZOOM_IN_BTN->setText(""); ui->VL_ZOOM_OUT_BTN->setText("");
        ui->VL_FOCUS_FAR_BTN->setText(""); ui->VL_FOCUS_NEAR_BTN->setText("");
        ui->IR_ZOOM_IN_BTN->setText(""); ui->IR_ZOOM_OUT_BTN->setText("");
        ui->IR_FOCUS_FAR_BTN->setText(""); ui->IR_FOCUS_NEAR_BTN->setText("");
    });
    connect(m_device_mgr, &DeviceManager::vid_btn_style_changed, this, [this](QString name, QString style) {
        QWidget *w = findChild<QWidget*>(name);
        if (w) w->setStyleSheet(style);
    });
    connect(m_device_mgr, &DeviceManager::vid_btn_text_changed, this, [this](QString name, QString text) {
        QAbstractButton *b = findChild<QAbstractButton*>(name);
        if (b) b->setText(text);
    });
    connect(m_device_mgr, &DeviceManager::ir_power_btn_revert, this, [this]() {
        ui->IR_POWER_BTN->setChecked(!ui->IR_POWER_BTN->isChecked());
    });
    connect(m_device_mgr, &DeviceManager::ptz_type_enable_changed, this, [this](bool enabled) {
        pref->set_ptz_type_enabled(enabled);
    });
    connect(m_device_mgr, &DeviceManager::baudrate_display_requested, this, [this](int idx, int baud) {
        pref->display_baudrate(idx, baud);
    });
    connect(m_device_mgr, &DeviceManager::tcp_server_chk_update, this, [this](bool enabled, bool checked) {
        pref->ui->TCP_SERVER_CHK->setEnabled(enabled);
        pref->ui->TCP_SERVER_CHK->setChecked(checked);
    });
    connect(m_device_mgr, &DeviceManager::tcp_server_connect_failed, this, [this]() {
        pref->ui->TCP_SERVER_CHK->setEnabled(true);
        pref->ui->TCP_SERVER_CHK->setChecked(false);
    });
    connect(m_device_mgr, &DeviceManager::get_lens_param_requested, m_lens_ctrl, &LensController::on_GET_LENS_PARAM_BTN_clicked);

    // DeviceManager UI setup (PTZ button group, VID buttons, PTZ speed slider)
    {
        QString theme_dir = app_theme ? "light" : "dark";
        QButtonGroup *ptz_grp = new QButtonGroup(this);
        ui->UP_LEFT_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/up_left"));
        ptz_grp->addButton(ui->UP_LEFT_BTN, 0);
        ui->UP_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/up"));
        ptz_grp->addButton(ui->UP_BTN, 1);
        ui->UP_RIGHT_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/up_right"));
        ptz_grp->addButton(ui->UP_RIGHT_BTN, 2);
        ui->LEFT_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/left"));
        ptz_grp->addButton(ui->LEFT_BTN, 3);
        ui->SELF_TEST_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/self_test"));
        ptz_grp->addButton(ui->SELF_TEST_BTN, 4);
        ui->RIGHT_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/right"));
        ptz_grp->addButton(ui->RIGHT_BTN, 5);
        ui->DOWN_LEFT_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/down_left"));
        ptz_grp->addButton(ui->DOWN_LEFT_BTN, 6);
        ui->DOWN_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/down"));
        ptz_grp->addButton(ui->DOWN_BTN, 7);
        ui->DOWN_RIGHT_BTN->setIcon(QIcon(":/directions/" + theme_dir + "/down_right"));
        ptz_grp->addButton(ui->DOWN_RIGHT_BTN, 8);
        connect(ptz_grp, QOverload<int>::of(&QButtonGroup::buttonPressed), m_device_mgr, &DeviceManager::ptz_button_pressed);
        connect(ptz_grp, QOverload<int>::of(&QButtonGroup::buttonReleased), m_device_mgr, &DeviceManager::ptz_button_released);

        QButtonGroup *vid_grp = new QButtonGroup(this);
        vid_grp->addButton(ui->VL_ZOOM_OUT_BTN, UDPPTZ::VL_ZOOM_OUT);
        vid_grp->addButton(ui->VL_ZOOM_IN_BTN, UDPPTZ::VL_ZOOM_IN);
        vid_grp->addButton(ui->VL_FOCUS_FAR_BTN, UDPPTZ::VL_FOCUS_FAR);
        vid_grp->addButton(ui->VL_FOCUS_NEAR_BTN, UDPPTZ::VL_FOCUS_NEAR);
        vid_grp->addButton(ui->IR_ZOOM_OUT_BTN, UDPPTZ::IR_ZOOM_OUT);
        vid_grp->addButton(ui->IR_ZOOM_IN_BTN, UDPPTZ::IR_ZOOM_IN);
        vid_grp->addButton(ui->IR_FOCUS_FAR_BTN, UDPPTZ::IR_FOCUS_FAR);
        vid_grp->addButton(ui->IR_FOCUS_NEAR_BTN, UDPPTZ::IR_FOCUS_NEAR);
        connect(vid_grp, QOverload<int>::of(&QButtonGroup::buttonPressed), m_device_mgr, &DeviceManager::vid_camera_pressed);
        connect(vid_grp, QOverload<int>::of(&QButtonGroup::buttonReleased), this, [this](int) { m_device_mgr->vid_camera_released(); });

        setup_slider(ui->PTZ_SPEED_SLIDER, 1, 64, 1, 4, 32);
        connect(ui->PTZ_SPEED_SLIDER, &QSlider::valueChanged, m_device_mgr, &DeviceManager::set_ptz_speed);
        ui->PTZ_SPEED_EDIT->setText("32");
        connect(ui->PTZ_SPEED_EDIT, &QLineEdit::editingFinished, this, [this]() {
            m_device_mgr->set_ptz_speed(ui->PTZ_SPEED_EDIT->text().toInt());
        });

        // VID toggle button connections
        connect(ui->VL_DEFOG_BTN, &QAbstractButton::toggled, m_device_mgr, &DeviceManager::vid_defog_toggled);
        connect(ui->IR_POWER_BTN, &QAbstractButton::toggled, m_device_mgr, &DeviceManager::vid_ir_power_toggled);
        connect(ui->IR_AUTO_FOCUS_BTN, &QAbstractButton::clicked, m_device_mgr, &DeviceManager::vid_ir_auto_focus_clicked);
        connect(ui->LDM_BTN, &QAbstractButton::toggled, m_device_mgr, &DeviceManager::vid_ldm_toggled);
        connect(ui->VIDEO_SOURCE_BTN, &QAbstractButton::toggled, m_device_mgr, &DeviceManager::vid_video_source_toggled);
        connect(ui->OSD_BTN, &QAbstractButton::toggled, m_device_mgr, &DeviceManager::vid_osd_toggled);

        // COM port edit connections
        connect(ui->TCU_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { m_device_mgr->connect_port(0, ui->TCU_COM_EDIT->text()); });
        connect(ui->LENS_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { m_device_mgr->connect_port(1, ui->LENS_COM_EDIT->text()); });
        connect(ui->LASER_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { m_device_mgr->connect_port(2, ui->LASER_COM_EDIT->text()); });
        connect(ui->PTZ_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { m_device_mgr->connect_ptz_port(ui->PTZ_COM_EDIT->text()); });
        connect(ui->RANGE_COM_EDIT, &QLineEdit::returnPressed, this, [this]() { m_device_mgr->connect_port(4, ui->RANGE_COM_EDIT->text()); });
    }

    // TCUController → UI
    connect(m_tcu_ctrl, &TCUController::freq_display_updated, this, [this](QString unit, QString val) {
        ui->FREQ_UNIT->setText(unit);
        ui->FREQ_EDIT->setText(val);
    });
    connect(m_tcu_ctrl, &TCUController::stepping_display_updated, this, [this](QString unit, QString val) {
        ui->STEPPING_UNIT->setText(unit);
        ui->STEPPING_EDIT->setText(val);
    });
    connect(m_tcu_ctrl, &TCUController::delay_slider_max_changed, ui->DELAY_SLIDER, &QSlider::setMaximum);
    connect(m_tcu_ctrl, &TCUController::gw_slider_max_changed, ui->GW_SLIDER, &QSlider::setMaximum);
    connect(m_tcu_ctrl, &TCUController::mcp_slider_max_changed, ui->MCP_SLIDER, &QSlider::setMaximum);
    connect(m_tcu_ctrl, &TCUController::mcp_display_updated, this, [this](int val, QString text) {
        ui->MCP_SLIDER->setValue(val);
        ui->MCP_EDIT->setText(text);
    });
    connect(m_tcu_ctrl, &TCUController::delay_slider_value_changed, ui->DELAY_SLIDER, &QSlider::setValue);
    connect(m_tcu_ctrl, &TCUController::gw_slider_value_changed, ui->GW_SLIDER, &QSlider::setValue);
    connect(m_tcu_ctrl, &TCUController::auto_mcp_chk_changed, ui->AUTO_MCP_CHK, &QCheckBox::setChecked);
    connect(m_tcu_ctrl, &TCUController::distance_text_updated, ui->DISTANCE, &QLabel::setText);
    connect(m_tcu_ctrl, &TCUController::est_dist_text_updated, ui->EST_DIST, &QLabel::setText);
    connect(m_tcu_ctrl, &TCUController::laser_width_display_updated, this, [this](QString u, QString n, QString p) {
        ui->LASER_WIDTH_EDIT_U->setText(u);
        ui->LASER_WIDTH_EDIT_N->setText(n);
        ui->LASER_WIDTH_EDIT_P->setText(p);
    });
    connect(m_tcu_ctrl, &TCUController::delay_display_updated, this,
            [this](QString au, QString an, QString ap, QString bu, QString bn, QString bp) {
        ui->DELAY_A_EDIT_U->setText(au); ui->DELAY_A_EDIT_N->setText(an); ui->DELAY_A_EDIT_P->setText(ap);
        ui->DELAY_B_EDIT_U->setText(bu); ui->DELAY_B_EDIT_N->setText(bn); ui->DELAY_B_EDIT_P->setText(bp);
    });
    connect(m_tcu_ctrl, &TCUController::gw_display_updated, this,
            [this](QString au, QString an, QString ap, QString bu, QString bn, QString bp, QString gw) {
        ui->GATE_WIDTH_A_EDIT_U->setText(au); ui->GATE_WIDTH_A_EDIT_N->setText(an); ui->GATE_WIDTH_A_EDIT_P->setText(ap);
        ui->GATE_WIDTH_B_EDIT_U->setText(bu); ui->GATE_WIDTH_B_EDIT_N->setText(bn); ui->GATE_WIDTH_B_EDIT_P->setText(bp);
        if (!gw.isEmpty()) ui->GATE_WIDTH->setText(gw);
    });
    connect(m_tcu_ctrl, &TCUController::tcu_type_layout_changed, this, [this](int idx, int diff) {
        // Show/hide picosecond fields and diff group based on TCU type
        bool show_ps = (idx >= 2);
        ui->LASER_WIDTH_EDIT_P->setVisible(show_ps);
        ui->DELAY_A_EDIT_P->setVisible(show_ps);
        ui->DELAY_B_EDIT_P->setVisible(show_ps);
        ui->GATE_WIDTH_A_EDIT_P->setVisible(show_ps);
        ui->GATE_WIDTH_B_EDIT_P->setVisible(show_ps);
        bool show_diff = (diff != 0);
        ui->DELAY_DIFF_GRP->setVisible(show_diff);
        ui->GATE_WIDTH_DIFF_GRP->setVisible(show_diff);
    });
    connect(m_tcu_ctrl, &TCUController::tcu_diff_view_toggled, this, [this](bool show) {
        ui->DELAY_DIFF_GRP->setVisible(show);
        ui->GATE_WIDTH_DIFF_GRP->setVisible(show);
    });
    connect(m_tcu_ctrl, &TCUController::ps_config_display_updated, this, [this](QString stepping, QString max_step) {
        pref->ui->PS_STEPPING_EDT->setText(stepping);
        pref->ui->MAX_PS_STEP_EDT->setText(max_step);
    });
    connect(m_tcu_ctrl, &TCUController::auto_mcp_chk_click_requested, this, [this]() {
        ui->AUTO_MCP_CHK->click();
    });
    connect(m_tcu_ctrl, &TCUController::fire_laser_btn_click_requested, this, [this]() {
        ui->FIRE_LASER_BTN->click();
    });
    connect(m_tcu_ctrl, &TCUController::laser_chk_click_requested, this, [this]() {
        pref->ui->LASER_ENABLE_CHK->click();
    });
    connect(m_tcu_ctrl, &TCUController::dist_ns_changed, this, [this](float dist_ns) {
        pref->dist_ns = dist_ns;
    });
    connect(m_tcu_ctrl, &TCUController::update_distance_display_requested, pref, &Preferences::update_distance_display);

    // LensController → UI
    connect(m_lens_ctrl, &LensController::button_text_changed, this, [this](int id, QString text) {
        QAbstractButton *btns[] = {
            ui->ZOOM_IN_BTN, ui->ZOOM_OUT_BTN, ui->FOCUS_FAR_BTN, ui->FOCUS_NEAR_BTN,
            ui->RADIUS_INC_BTN, ui->RADIUS_DEC_BTN, ui->IRIS_OPEN_BTN, ui->IRIS_CLOSE_BTN
        };
        if (id >= 0 && id < 8) btns[id]->setText(text);
    });
    connect(m_lens_ctrl, &LensController::zoom_text_updated, this, [this](QString text) {
        if (!ui->ZOOM_EDIT->hasFocus()) ui->ZOOM_EDIT->setText(text);
    });
    connect(m_lens_ctrl, &LensController::focus_text_updated, this, [this](QString text) {
        if (!ui->FOCUS_EDIT->hasFocus()) ui->FOCUS_EDIT->setText(text);
    });
    connect(m_lens_ctrl, &LensController::focus_speed_display_changed, this, [this](int val, QString text) {
        ui->FOCUS_SPEED_SLIDER->setValue(val);
        if (!text.isEmpty()) ui->FOCUS_SPEED_EDIT->setText(text);
    });

    // LaserController → UI
    connect(m_laser_ctrl, &LaserController::laser_btn_update, this, [this](bool enabled, QString text) {
        ui->LASER_BTN->setEnabled(enabled);
        if (!text.isEmpty()) ui->LASER_BTN->setText(text);
    });
    connect(m_laser_ctrl, &LaserController::fire_btn_update, this, [this](bool enabled, QString text) {
        ui->FIRE_LASER_BTN->setEnabled(enabled);
        if (!text.isEmpty()) ui->FIRE_LASER_BTN->setText(text);
    });
    connect(m_laser_ctrl, &LaserController::current_edit_enabled, ui->CURRENT_EDIT, &QWidget::setEnabled);
    connect(m_laser_ctrl, &LaserController::simple_laser_chk_update, this, [this](bool enabled, int state) {
        ui->SIMPLE_LASER_CHK->setEnabled(enabled);
        if (state == 0) ui->SIMPLE_LASER_CHK->setChecked(false);
        else if (state == 1) ui->SIMPLE_LASER_CHK->setChecked(true);
        else if (state == 2) ui->SIMPLE_LASER_CHK->click();
    });
    connect(m_laser_ctrl, &LaserController::laser_enable_requested, this, [this](bool enable) {
        pref->ui->LASER_ENABLE_CHK->setChecked(enable);
    });
    connect(m_laser_ctrl, &LaserController::update_current_requested, this, [this]() {
        update_current();
    });

    // RFController → UI
    connect(m_rf_ctrl, &RFController::distance_text_updated, ui->DISTANCE, &QLabel::setText);

    // ScanController → UI
    connect(m_scan_ctrl, &ScanController::scan_button_text_changed, ui->SCAN_BUTTON, &QPushButton::setText);
    connect(m_scan_ctrl, &ScanController::scan_options_changed, this, [this](bool cont, bool restart) {
        ui->CONTINUE_SCAN_BUTTON->setVisible(cont);
        ui->RESTART_SCAN_BUTTON->setVisible(restart);
    });

    // PipelineProcessor — all pipeline methods run by GrabThread
    {
        PipelineProcessor::SharedState ss;
        ss.img_mem = img_mem;
        ss.modified_result = modified_result;
        ss.image_mutex = image_mutex;
        ss.vid_out = vid_out;
        ss.w = w;
        ss.h = h;
        ss.pixel_depth = pixel_depth;
        ss.is_color = is_color;
        ss.user_mask = user_mask;
        ss.grab_image = grab_image;
        ss.save_original = save_original;
        ss.save_modified = save_modified;
        ss.record_original = record_original;
        ss.record_modified = record_modified;
        ss.grab_thread_state = grab_thread_state;
        ss.q_img = q_img;
        ss.q_frame_info = &q_frame_info;
        ss.q_fps_calc = &q_fps_calc;
        ss.frame_info_mutex = &frame_info_mutex;
        ss.m_params_mutex = &m_params_mutex;
        ss.m_processing_params = &m_processing_params;
        ss.m_pipeline_config = &m_pipeline_config;
        ss.m_roi_mutex = &m_roi_mutex;
        ss.list_roi = &list_roi;
        ss.m_yolo_detector = m_yolo_detector;
        ss.m_yolo_init_mutex = &m_yolo_init_mutex;
        ss.m_yolo_last_model = m_yolo_last_model;
        ss.tcu_ctrl = m_tcu_ctrl;
        ss.scan_ctrl = m_scan_ctrl;
        ss.device_mgr = m_device_mgr;
        ss.aux_panel = m_aux_panel;
        ss.displays = displays;
        ss.secondary_display = secondary_display;
        ss.scan_config = scan_config;
        ss.save_location = &save_location;
        ss.tp = &tp;
        ss.device_type = &device_type;
        ss.frame_rate_edit = &frame_rate_edit;
        m_pipeline = new PipelineProcessor(ss, this);
    }

    // Connect PipelineProcessor signals
    connect(m_pipeline, &PipelineProcessor::packet_lost_updated, this,
            [this](int packets_lost){ ui->STATUS->packet_lost->set_text("packets lost: " + QString::number(packets_lost)); }, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::update_mcp_in_thread, this, &UserPanel::change_mcp, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::set_hist_pixmap, ui->HIST_DISPLAY, &Display::setPixmap, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::finish_scan_signal, m_scan_ctrl, &ScanController::on_SCAN_BUTTON_clicked, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::save_screenshot_signal, this, [this](QString path) {
        window()->grab().save(path);
    }, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::update_delay_in_thread, this, [this](){
        m_tcu_ctrl->update_delay();
    }, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::update_dist_mat, this, [this](cv::Mat mat, double min, double max){
#ifdef DISTANCE_3D_VIEW
        if (distance_3d_view) distance_3d_view->update_dist(mat, min, max);
#endif
    }, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::task_queue_full, this, &UserPanel::stop_image_writing, Qt::UniqueConnection);
#ifdef LVTONG
    connect(m_pipeline, &PipelineProcessor::set_model_list_enabled, this, [this](bool enabled) {
        pref->ui->MODEL_LIST->setEnabled(enabled);
    }, Qt::QueuedConnection);
#endif

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

    laser_settings = new LaserControl(nullptr, m_device_mgr->lens());
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
    connect(pref, &Preferences::set_auto_rep_freq, m_device_mgr->tcu(), [this](bool arg1){ emit send_uint_tcu_msg(TCU::AUTO_PRF, arg1); });
    connect(pref, &Preferences::set_ab_lock, m_device_mgr->tcu(), [this](bool arg1){ emit send_uint_tcu_msg(TCU::AB_LOCK, arg1); });

#ifdef DISTANCE_3D_VIEW
    view_3d = new Distance3DView(nullptr, this, &list_roi);
#endif //DISTANCE_3D_VIEW
    aliasing = new Aliasing();
    connect(aliasing, &Aliasing::delay_set_selected, this, &UserPanel::set_distance_set);

    preset = new PresetPanel();
    connect(preset, SIGNAL(preset_updated(int)), this, SLOT(generate_preset_data(int)), Qt::QueuedConnection);
    connect(this, SIGNAL(save_to_preset(int, nlohmann::json)), preset, SLOT(save_preset(int, nlohmann::json)), Qt::QueuedConnection);
    connect(preset, &PresetPanel::preset_selected, this, &UserPanel::apply_preset, Qt::QueuedConnection);

    // serial_port/tcp_port creation → DeviceManager

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

    // Thread/port cleanup handled by ~DeviceManager (destroyed as child of this)

    delete ui;
}

void UserPanel::init()
{
    // auto-load default config if it exists
//    QString default_config_path = QCoreApplication::applicationDirPath() + "/default.json";
    QString default_config_path = "default.json";
    if (QFile::exists(default_config_path)) {
        if (config->load_from_file(default_config_path)) {
            syncConfigToPreferences();
        }
    }

    // Set default PTZ type to udp-scw
    pref->ui->PTZ_TYPE_LIST->setCurrentIndex(2);

    // Initialize YOLO comboboxes from config
    ui->MAIN_MODEL_LIST->setCurrentIndex(config->get_data().yolo.main_display_model);
    ui->ALT1_MODEL_LIST->setCurrentIndex(config->get_data().yolo.alt1_display_model);
    ui->ALT2_MODEL_LIST->setCurrentIndex(config->get_data().yolo.alt2_display_model);

    // Connect YOLO combobox signals
    connect(ui->MAIN_MODEL_LIST, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        config->get_data().yolo.main_display_model = index;
//        config->auto_save();
    });
    connect(ui->ALT1_MODEL_LIST, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        config->get_data().yolo.alt1_display_model = index;
//        config->auto_save();
    });
    connect(ui->ALT2_MODEL_LIST, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        config->get_data().yolo.alt2_display_model = index;
//        config->auto_save();
    });

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
    m_tcu_ctrl->set_delay_dist(1000);
    m_tcu_ctrl->set_depth_of_view(75);
    m_tcu_ctrl->set_laser_width(500);
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

    connect(this, SIGNAL(update_fishnet_result(int)),
            SLOT(display_fishnet_result(int)), Qt::QueuedConnection);

    ui->START_BUTTON->click();
    ui->HIDE_BTN->click();
    ui->START_GRABBING_BUTTON->click();
    switch_ui();

    m_tcu_ctrl->set_delay_dist(9);
    m_tcu_ctrl->set_depth_of_view(2.25);
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

#ifdef USING_CAMERALINK
    pref->ui->CAMERALINK_CHK->click();
#else //USING_CAMERALINK
    pref->ui->CAMERALINK_CHK->hide();
#endif //USING_CAMERALINK
    ui->SENSOR_TAPS_BTN->hide();

    // COM ports will be loaded from JSON config during auto-load
    QLineEdit *com_edits[] = { ui->TCU_COM_EDIT, ui->LENS_COM_EDIT, ui->LASER_COM_EDIT, ui->PTZ_COM_EDIT, ui->RANGE_COM_EDIT };
    for (int i = 0; i < 5; i++) com_edits[i]->emit returnPressed();

    if (m_device_mgr->serial(0)->isOpen() && m_device_mgr->serial(3)->isOpen()) on_LASER_BTN_clicked();

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

        switch (m_tcu_ctrl->get_hz_unit()) {
        // kHz
        case 0: m_tcu_ctrl->set_rep_freq(ui->FREQ_EDIT->text().toFloat()); break;
        // Hz
        case 1: m_tcu_ctrl->set_rep_freq(ui->FREQ_EDIT->text().toFloat() / 1000); break;
        default: break;
        }
//        ptr_tcu->laser_width = ui->LASER_WIDTH_EDIT_U->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt() + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->gate_width_a = ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt() + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->gate_width_b = ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->delay_a = ui->DELAY_A_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->delay_b = ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.;
//        ptr_tcu->gate_width_n = ui->GATE_WIDTH_N_EDIT_N->text().toFloat();
//        ptr_tcu->delay_n = ui->DELAY_N_EDIT_N->text().toFloat();

        switch (m_tcu_ctrl->get_base_unit()) {
        // ns
        case 0: m_tcu_ctrl->set_stepping(ui->STEPPING_EDIT->text().toFloat()); break;
        // μs
        case 1: m_tcu_ctrl->set_stepping(ui->STEPPING_EDIT->text().toFloat() * 1000); break;
        // m
        case 2: m_tcu_ctrl->set_stepping(ui->STEPPING_EDIT->text().toFloat() / m_tcu_ctrl->get_dist_ns()); break;
        default: break;
        }
        // use toFloat() since float representation in line edit
//        ptr_tcu->ccd_freq = ui->CCD_FREQ_EDIT->text().toFloat();
//        ptr_tcu->duty = ui->DUTY_EDIT->text().toFloat() * 1000;
//        ptr_tcu->mcp = ui->MCP_EDIT->text().toInt();

//        zoom = ui->ZOOM_EDIT->text().toInt();
//        focus = ui->FOCUS_EDIT->text().toInt();

        m_tcu_ctrl->set_delay_dist(std::round(m_device_mgr->tcu()->get(TCU::DELAY_A) * m_tcu_ctrl->get_dist_ns()));
        m_tcu_ctrl->set_depth_of_view(std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A) * m_tcu_ctrl->get_dist_ns()));
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

        setup_hz(m_tcu_ctrl->get_hz_unit());
        // Update all TCU display widgets (laser width, delay, gate width, MCP)
        m_tcu_ctrl->update_tcu_params(TCU::NO_PARAM);
        ui->GATE_WIDTH_N_EDIT_N->setText(QString::asprintf("%d", int(std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N)))));
        ui->DELAY_N_EDIT_N->setText(QString::asprintf("%d", int(std::round(m_device_mgr->tcu()->get(TCU::DELAY_N)))));
        ui->MCP_EDIT->setText(QString::number((int)std::round(m_device_mgr->tcu()->get(TCU::MCP))));

        setup_stepping(m_tcu_ctrl->get_base_unit());

        ui->ZOOM_EDIT->setText(QString::asprintf("%d", m_device_mgr->lens()->get(Lens::ZOOM_POS)));
        ui->FOCUS_EDIT->setText(QString::asprintf("%d", m_device_mgr->lens()->get(Lens::FOCUS_POS)));

        ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", m_device_mgr->ptz()->get(PTZ::ANGLE_H)));
        ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", m_device_mgr->ptz()->get(PTZ::ANGLE_V)));
    }
}

// Pipeline methods moved to src/pipeline/pipelineprocessor.cpp

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
    ui->TCU_COM->setStyleSheet(m_device_mgr->tcu()->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");
    ui->LENS_COM->setStyleSheet(m_device_mgr->lens()->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");
    ui->LASER_COM->setStyleSheet(m_device_mgr->laser()->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");
    ui->PTZ_COM->setStyleSheet(m_device_mgr->ptz()->get_port_status() & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");

    QFont temp_f(consolas);
    temp_f.setPixelSize(11);
    ui->DATA_EXCHANGE->setFont(temp_f);

    setCursor(cursor_curr_pointer);

    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s/%s);", app_theme ? "light" : "dark", hide_left ? "right" : "left"));

    laser_settings->set_theme();

    config->get_data().ui.dark_theme = (app_theme == 0);
    config->auto_save();
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

    config->get_data().ui.english = (lang == 0);
    config->auto_save();
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

// connect_port_edit, init_control_port → DeviceManager


// setup_serial_port → DeviceManager

void UserPanel::on_ENUM_BUTTON_clicked()
{
    if (config->get_data().device.ebus) ui->SENSOR_TAPS_BTN->show();
    else                ui->SENSOR_TAPS_BTN->hide();

    if (curr_cam) delete curr_cam;
#ifdef WIN32
#ifdef USING_CAMERALINK
    if (pref->cameralink)    curr_cam = new EuresysCam;
    else if (config->get_data().device.ebus) curr_cam = new EbusCam;
    else                     curr_cam = new MvCam;
#else
    if (config->get_data().device.ebus) curr_cam = new EbusCam;
    else                curr_cam = new MvCam;
#endif
#else
    curr_cam = new MvCam;
#endif
//    if (ui->TITLE->prog_settings->cameralink) curr_cam->cameralink = true;
    enable_controls(device_type = (curr_cam->search_for_devices() ?  (pref->cameralink ? 3 : (config->get_data().device.ebus ? 2 : 1)) : 0));
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

    if (curr_cam->start(config->get_data().tcu.type)) {
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

    static struct main_ui_info cam_union = {&q_frame_info, q_img + 0, &frame_info_mutex, &image_mutex[0]};
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

    h_grab_thread[0] = new GrabThread(m_pipeline, 0);
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
    if (device_type == -1 || !config->get_data().save.consecutive_capture) save_original[0] = 1;
    else{
        save_original[0] = !save_original[0];
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
            res_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_res.mp4");
//            vid_out[1].open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(image_3d[0] ? w[0] + 104 : w[0], h[0]), is_color[0] || image_3d[0]);
            vid_out[1].open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), frame_rate_edit, cv::Size(image_3d[0] ? w[0] + 104 : w[0], h[0]), is_color[0] || pseudocolor[0]);
        }
    }
    record_modified[0] = !record_modified[0];
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

// setup_hz, setup_stepping, setup_max_dist, setup_max_dov, update_delay_offset,
// update_gate_width_offset, update_laser_offset, setup_laser → TCUController (inline delegations in header)

// set_baudrate, set_tcp_status_on_port, set_tcu_as_shared_port,
// com_write_data, display_port_info → DeviceManager (inline delegations in header)

void UserPanel::update_dev_ip()
{
//    ui->TITLE->prog_settings->enable_ip_editing(device_type == 1);
    pref->enable_ip_editing(device_type > 0 && config->get_data().tcu.type < curr_cam->gige_device_num());
    if (device_type > 0) {
        int ip, gateway, nic_address;
        curr_cam->ip_address(true, config->get_data().tcu.type, &ip, &gateway, &nic_address);
//        ui->TITLE->prog_settings->config_ip(true, ip, gateway);
        pref->config_ip(true, ip, gateway, nic_address);
    }
}

void UserPanel::set_dev_ip(int ip, int gateway)
{
    int static_ip = MV_IP_CFG_STATIC;
    curr_cam->ip_config(false, &static_ip);
    curr_cam->ip_address(false, config->get_data().tcu.type, &ip, &gateway);
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
    ui->RANGE_THRESH_EDIT->setText(QString::number((int)(config->get_data().image_proc.lower_3d_thresh * (1 << pixel_depth[0]))));
}

void UserPanel::reset_custom_3d_params()
{
//    pref->ui->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(ptr_tcu->delay_a)));
//    pref->ui->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(ptr_tcu->gate_width_a)));
    pref->ui->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(m_device_mgr->tcu()->get(TCU::DELAY_A))));
    pref->ui->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A))));
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

// set_tcu_type → TCUController (inline delegation in header)

// update_ps_config → TCUController (inline delegation in header)

// set_auto_mcp → TCUController (inline delegation in header)

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
        if      (joybtn_X) on_ZOOM_IN_BTN_pressed(),       m_lens_ctrl->set_lens_adjust_ongoing(0b001);
        else if (joybtn_Y) on_FOCUS_NEAR_BTN_pressed(),    m_lens_ctrl->set_lens_adjust_ongoing(0b010);
        else if (joybtn_A) on_IRIS_OPEN_BTN_pressed(),     m_lens_ctrl->set_lens_adjust_ongoing(0b100);
        break;
    case JoystickThread::BUTTON::R1: joybtn_R1 = true; break;
    case JoystickThread::BUTTON::R2:
        joybtn_R2 = true;
        if      (joybtn_X) on_ZOOM_OUT_BTN_pressed(),       m_lens_ctrl->set_lens_adjust_ongoing(0b001);
        else if (joybtn_Y) on_FOCUS_FAR_BTN_pressed(),      m_lens_ctrl->set_lens_adjust_ongoing(0b010);
        else if (joybtn_A) on_IRIS_CLOSE_BTN_pressed(),     m_lens_ctrl->set_lens_adjust_ongoing(0b100);
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
        if (ptz_adjust_ongoing) m_device_mgr->ptz_button_released(-1), ptz_adjust_ongoing = false;
        break;
    case JoystickThread::BUTTON::B:  joybtn_B  = false; break;
    case JoystickThread::BUTTON::X:  joybtn_X  = false; break;
    case JoystickThread::BUTTON::Y:  joybtn_Y  = false; break;
    case JoystickThread::BUTTON::L1: joybtn_L1 = false; break;
    case JoystickThread::BUTTON::L2:
        joybtn_L2 = false;
        if (m_lens_ctrl->get_lens_adjust_ongoing() & 0b001) on_ZOOM_IN_BTN_released(),       m_lens_ctrl->set_lens_adjust_ongoing(0);
        if (m_lens_ctrl->get_lens_adjust_ongoing() & 0b010) on_FOCUS_NEAR_BTN_released(),    m_lens_ctrl->set_lens_adjust_ongoing(0);
        if (m_lens_ctrl->get_lens_adjust_ongoing() & 0b100) on_IRIS_OPEN_BTN_released(), m_lens_ctrl->set_lens_adjust_ongoing(0);
        break;
    case JoystickThread::BUTTON::R1: joybtn_R1 = false; break;
    case JoystickThread::BUTTON::R2:
        joybtn_R2 = false;
        if (m_lens_ctrl->get_lens_adjust_ongoing() & 0b001) on_ZOOM_OUT_BTN_released(),       m_lens_ctrl->set_lens_adjust_ongoing(0);
        if (m_lens_ctrl->get_lens_adjust_ongoing() & 0b010) on_FOCUS_FAR_BTN_released(),      m_lens_ctrl->set_lens_adjust_ongoing(0);
        if (m_lens_ctrl->get_lens_adjust_ongoing() & 0b100) on_IRIS_CLOSE_BTN_released(), m_lens_ctrl->set_lens_adjust_ongoing(0);
        break;
    case JoystickThread::BUTTON::HOME: ui->SHUTDOWN_BUTTON->click(); ui->ENUM_BUTTON->click(); break;
    case JoystickThread::BUTTON::SELECT: ui->START_BUTTON->click(); break;
    case JoystickThread::BUTTON::MINUS:
        if (joybtn_L1) m_tcu_ctrl->set_stepping(m_tcu_ctrl->get_stepping() / 10), setup_stepping(m_tcu_ctrl->get_base_unit());
        if (joybtn_R1) change_focus_speed(ui->FOCUS_SPEED_EDIT->text().toInt() - 8);
        if (joybtn_L2) ui->PTZ_SPEED_SLIDER->setValue(ui->PTZ_SPEED_SLIDER->value() - 8);
        break;
    case JoystickThread::BUTTON::PLUS:
        if (joybtn_L1) m_tcu_ctrl->set_stepping(m_tcu_ctrl->get_stepping() * 10), setup_stepping(m_tcu_ctrl->get_base_unit());
        if (joybtn_R1) change_focus_speed(ui->FOCUS_SPEED_EDIT->text().toInt() + 8);
        if (joybtn_L2) ui->PTZ_SPEED_SLIDER->setValue(ui->PTZ_SPEED_SLIDER->value() + 8);
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
        case JoystickThread::DIRECTION::UPLEFT:    m_device_mgr->ptz_button_pressed(0);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::UP:        m_device_mgr->ptz_button_pressed(1);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::UPRIGHT:   m_device_mgr->ptz_button_pressed(2);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::LEFT:      m_device_mgr->ptz_button_pressed(3);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::RIGHT:     m_device_mgr->ptz_button_pressed(5);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::DOWNLEFT:  m_device_mgr->ptz_button_pressed(6);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::DOWN:      m_device_mgr->ptz_button_pressed(7);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::DOWNRIGHT: m_device_mgr->ptz_button_pressed(8);   ptz_adjust_ongoing = true;  break;
        case JoystickThread::DIRECTION::STOP:      m_device_mgr->ptz_button_released(-1); ptz_adjust_ongoing = false; break;
        default:                                                                                        break;
        }
    }
}

// update_port_status(ControlPort*, QLabel*) → DeviceManager

void UserPanel::append_data(QString str)
{
    static QScrollBar *scroll_bar = ui->DATA_EXCHANGE->verticalScrollBar();
    ui->DATA_EXCHANGE->append(str);
    scroll_bar->setValue(scroll_bar->maximum());
}

// update_port_status(int) → DeviceManager

// update_lens_params → LensController (inline delegation in header)

// update_ptz_params → DeviceManager

// update_distance → RFController (inline delegation in header)

// update_ptz_angle, update_ptz_status → DeviceManager

// set_distance_set → TCUController (inline delegation in header)

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

void UserPanel::save_config_to_file()
{
    QString config_name = QFileDialog::getSaveFileName(this, tr("Save Configuration"), save_location + "/config.json", tr("JSON config(*.json);;All Files()"));
    if (config_name.isEmpty()) return;
    if (!config_name.endsWith(".json")) config_name.append(".json");

    // Update config with current settings
    syncPreferencesToConfig();

    if (!config->save_to_file(config_name)) {
        QMessageBox::warning(this, "PROMPT", tr("Cannot save config file"));
    }
}

void UserPanel::prompt_for_config_file()
{
    QString config_filename = QFileDialog::getOpenFileName(this, tr("Load Configuration"), save_location, tr("JSON config(*.json);;All Files()"));
    load_config(config_filename);
}

void UserPanel::load_config(QString config_name)
{
    if (config_name.isEmpty()) return;

    if (!config->load_from_file(config_name)) {
        QMessageBox::warning(this, "PROMPT", tr("Cannot load config file"));
        return;
    }

    // Apply config to current settings
    syncConfigToPreferences();

    // Update UI elements
    pref->data_exchange(false);
    update_delay();
    update_gate_width();
    update_laser_width();
    ui->MCP_SLIDER->setValue(std::round(m_device_mgr->tcu()->get(TCU::MCP)));
    on_SET_PARAMS_BUTTON_clicked();
}


void UserPanel::update_processing_params()
{
    QMutexLocker lk(&m_params_mutex);
    m_processing_params.frame_avg_enabled    = ui->FRAME_AVG_CHECK->isChecked();
    m_processing_params.frame_avg_mode       = ui->FRAME_AVG_OPTIONS->currentIndex();
    m_processing_params.enable_3d            = ui->IMG_3D_CHECK->isChecked();
    m_processing_params.enhance_enabled      = ui->IMG_ENHANCE_CHECK->isChecked();
    m_processing_params.enhance_option       = ui->ENHANCE_OPTIONS->currentIndex();
    m_processing_params.brightness           = ui->BRIGHTNESS_SLIDER->value();
    m_processing_params.contrast             = ui->CONTRAST_SLIDER->value();
    m_processing_params.gamma_slider         = ui->GAMMA_SLIDER->value();
    m_processing_params.pseudocolor_enabled  = ui->PSEUDOCOLOR_CHK->isChecked();
    m_processing_params.show_info            = ui->INFO_CHECK->isChecked();
    m_processing_params.show_center          = ui->CENTER_CHECK->isChecked();
    m_processing_params.select_tool          = ui->SELECT_TOOL->isChecked();
    m_processing_params.dual_display         = ui->DUAL_DISPLAY_CHK->isChecked();
    m_processing_params.mcp_slider_focused   = ui->MCP_SLIDER->hasFocus();
    m_processing_params.hist_display_size    = ui->HIST_DISPLAY->size();
    m_processing_params.split                = pref->split;
    m_processing_params.custom_info_text     = pref->ui->CUSTOM_INFO_EDT->text();
    m_processing_params.image_rotate         = image_rotate[0];
    m_processing_params.pseudocolor          = pseudocolor[0];
    m_processing_params.image_3d             = image_3d[0];
    m_processing_params.selection_v1         = displays[0]->selection_v1;
    m_processing_params.selection_v2         = displays[0]->selection_v2;
    m_processing_params.display_region       = displays[0]->display_region;
    m_processing_params.display_width        = displays[0]->width();
    m_processing_params.display_height       = displays[0]->height();

    // Snapshot Config fields read by pipeline methods
    const auto& data = config->get_data();
    m_pipeline_config.flip                = data.device.flip;
    m_pipeline_config.accu_base           = data.image_proc.accu_base;
    m_pipeline_config.gamma               = data.image_proc.gamma;
    m_pipeline_config.low_in              = data.image_proc.low_in;
    m_pipeline_config.high_in             = data.image_proc.high_in;
    m_pipeline_config.low_out             = data.image_proc.low_out;
    m_pipeline_config.high_out            = data.image_proc.high_out;
    m_pipeline_config.dehaze_pct          = data.image_proc.dehaze_pct;
    m_pipeline_config.colormap            = data.image_proc.colormap;
    m_pipeline_config.lower_3d_thresh     = data.image_proc.lower_3d_thresh;
    m_pipeline_config.upper_3d_thresh     = data.image_proc.upper_3d_thresh;
    m_pipeline_config.truncate_3d         = data.image_proc.truncate_3d;
    m_pipeline_config.custom_3d_param     = data.image_proc.custom_3d_param;
    m_pipeline_config.custom_3d_delay     = data.image_proc.custom_3d_delay;
    m_pipeline_config.custom_3d_gate_width = data.image_proc.custom_3d_gate_width;
    m_pipeline_config.model_idx           = data.image_proc.model_idx;
    m_pipeline_config.fishnet_recog       = data.image_proc.fishnet_recog;
    m_pipeline_config.fishnet_thresh      = data.image_proc.fishnet_thresh;
    m_pipeline_config.ecc_window_mode     = data.image_proc.ecc_window_mode;
    m_pipeline_config.ecc_warp_mode       = data.image_proc.ecc_warp_mode;
    m_pipeline_config.ecc_fusion_method   = data.image_proc.ecc_fusion_method;
    m_pipeline_config.ecc_backward        = data.image_proc.ecc_backward;
    m_pipeline_config.ecc_forward         = data.image_proc.ecc_forward;
    m_pipeline_config.ecc_levels          = data.image_proc.ecc_levels;
    m_pipeline_config.ecc_max_iter        = data.image_proc.ecc_max_iter;
    m_pipeline_config.ecc_eps             = data.image_proc.ecc_eps;
    m_pipeline_config.ecc_half_res_reg    = data.image_proc.ecc_half_res_reg;
    m_pipeline_config.ecc_half_res_fuse   = data.image_proc.ecc_half_res_fuse;
    m_pipeline_config.save_info           = data.save.save_info;
    m_pipeline_config.custom_topleft_info = data.save.custom_topleft_info;
    m_pipeline_config.save_in_grayscale   = data.save.save_in_grayscale;
    m_pipeline_config.consecutive_capture = data.save.consecutive_capture;
    m_pipeline_config.integrate_info      = data.save.integrate_info;
    m_pipeline_config.img_format          = data.save.img_format;
    m_pipeline_config.config_path         = data.yolo.config_path;
    m_pipeline_config.main_display_model  = data.yolo.main_display_model;
    m_pipeline_config.alt1_display_model  = data.yolo.alt1_display_model;
    m_pipeline_config.alt2_display_model  = data.yolo.alt2_display_model;
    m_pipeline_config.visible_model_path  = data.yolo.visible_model_path;
    m_pipeline_config.visible_classes_file = data.yolo.visible_classes_file;
    m_pipeline_config.thermal_model_path  = data.yolo.thermal_model_path;
    m_pipeline_config.thermal_classes_file = data.yolo.thermal_classes_file;
    m_pipeline_config.gated_model_path    = data.yolo.gated_model_path;
    m_pipeline_config.gated_classes_file  = data.yolo.gated_classes_file;
}

void UserPanel::syncPreferencesToConfig()
{
    Config::ConfigData& data = config->get_data();

    // Sync COM settings from UI
#ifdef WIN32
    data.com_tcu.port = "COM" + ui->TCU_COM_EDIT->text();
    data.com_lens.port = "COM" + ui->LENS_COM_EDIT->text();
    data.com_laser.port = "COM" + ui->LASER_COM_EDIT->text();
    data.com_ptz.port = "COM" + ui->PTZ_COM_EDIT->text();
    data.com_range.port = "COM" + ui->RANGE_COM_EDIT->text();
#else
    data.com_tcu.port = ui->TCU_COM_EDIT->text();
    data.com_lens.port = ui->LENS_COM_EDIT->text();
    data.com_laser.port = ui->LASER_COM_EDIT->text();
    data.com_ptz.port = ui->PTZ_COM_EDIT->text();
    data.com_range.port = ui->RANGE_COM_EDIT->text();
#endif

    // Sync network settings
    if (pref->ui->TCP_SERVER_IP_EDIT) {
        data.network.tcp_server_address = pref->ui->TCP_SERVER_IP_EDIT->text();
    }

    // Sync UI settings
    data.ui.simplified = simple_ui;
    data.ui.dark_theme = !app_theme; // app_theme seems to be inverted
    data.ui.english = (lang == 0);

    // Sync camera settings
    data.camera.continuous_mode = !trigger_mode_on;
    data.camera.frequency = static_cast<int>(frame_rate_edit);
    data.camera.time_exposure = time_exposure_edit;
    data.camera.gain = gain_analog_edit;

    // Read pending Preferences UI changes (text fields) into Config
    pref->data_exchange(true);
}

void UserPanel::syncConfigToPreferences()
{
    const Config::ConfigData& data = config->get_data();

    // Sync COM settings to UI
#ifdef WIN32
    ui->TCU_COM_EDIT->setText(data.com_tcu.port.startsWith("COM") ? data.com_tcu.port.mid(3) : data.com_tcu.port);
    ui->LENS_COM_EDIT->setText(data.com_lens.port.startsWith("COM") ? data.com_lens.port.mid(3) : data.com_lens.port);
    ui->LASER_COM_EDIT->setText(data.com_laser.port.startsWith("COM") ? data.com_laser.port.mid(3) : data.com_laser.port);
    ui->PTZ_COM_EDIT->setText(data.com_ptz.port.startsWith("COM") ? data.com_ptz.port.mid(3) : data.com_ptz.port);
    ui->RANGE_COM_EDIT->setText(data.com_range.port.startsWith("COM") ? data.com_range.port.mid(3) : data.com_range.port);
#else
    ui->TCU_COM_EDIT->setText(data.com_tcu.port);
    ui->LENS_COM_EDIT->setText(data.com_lens.port);
    ui->LASER_COM_EDIT->setText(data.com_laser.port);
    ui->PTZ_COM_EDIT->setText(data.com_ptz.port);
    ui->RANGE_COM_EDIT->setText(data.com_range.port);
#endif

    // Sync network settings
    if (pref->ui->TCP_SERVER_IP_EDIT) {
        pref->ui->TCP_SERVER_IP_EDIT->setText(data.network.tcp_server_address);
    }
    m_device_mgr->udpptz()->set_target(QHostAddress(data.network.udp_target_ip), data.network.udp_target_port);
    m_device_mgr->udpptz()->set_listen(QHostAddress(data.network.udp_listen_ip), data.network.udp_listen_port);

    // Sync UI settings
    bool simplify_ui = data.ui.simplified;
    if (simple_ui != simplify_ui) switch_ui();
    bool new_theme = data.ui.dark_theme ? 0 : 1;
    if (new_theme != app_theme) set_theme();
    lang = data.ui.english ? 0 : 1;
    if (!data.ui.english) switch_language();

    // Sync camera settings
    trigger_mode_on = !data.camera.continuous_mode;
    frame_rate_edit = static_cast<float>(data.camera.frequency);
    time_exposure_edit = data.camera.time_exposure;
    gain_analog_edit = data.camera.gain;

    // TCU, device, save, and image_proc widgets are now populated by pref->data_exchange(false)
    data_exchange(false);
    pref->data_exchange(false);
}

void UserPanel::generate_preset_data(int idx)
{
#ifdef DEPRECATED_PRESET_STRUCT
    // move attribute_get to function
    PresetData preset_data{0};
    memset(&preset_data, 0, sizeof(PresetData));
    preset_data.rep_freq     = m_device_mgr->tcu()->get(TCU::REPEATED_FREQ);
    preset_data.laser_width  = m_device_mgr->tcu()->get(TCU::LASER_WIDTH);
    preset_data.delay_a      = m_device_mgr->tcu()->get(TCU::DELAY_A);
    preset_data.gw_a         = m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A);
    preset_data.delay_b      = m_device_mgr->tcu()->get(TCU::DELAY_B);
    preset_data.gw_b         = m_device_mgr->tcu()->get(TCU::GATE_WIDTH_B);
    preset_data.ccd_freq     = m_device_mgr->tcu()->get(TCU::CCD_FREQ);
    preset_data.duty         = m_device_mgr->tcu()->get(TCU::DUTY);
    preset_data.mcp          = m_device_mgr->tcu()->get(TCU::MCP);
    preset_data.delay_n      = m_device_mgr->tcu()->get(TCU::DELAY_N);
    preset_data.gatewidth_n  = m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N);
    preset_data.zoom         = m_device_mgr->lens()->get(Lens::ZOOM_POS);
    preset_data.focus        = m_device_mgr->lens()->get(Lens::FOCUS_POS);
    preset_data.laser_radius = m_device_mgr->lens()->get(Lens::LASER_RADIUS);
    preset_data.lens_speed   = m_device_mgr->lens()->get(Lens::STEPPING);
    preset_data.angle_h      = m_device_mgr->ptz()->get(PTZ::ANGLE_H);
    preset_data.angle_v      = m_device_mgr->ptz()->get(PTZ::ANGLE_V);
    preset_data.ptz_speed    = m_device_mgr->ptz()->get(PTZ::SPEED);
    emit save_to_preset(idx, preset_data);
#endif

//    nlohmann::json json_tcu;
//    json_tcu["TCU"] = m_device_mgr->tcu()->to_json();
//    nlohmann::json json_lens;
//    json_lens["Lens"] = m_device_mgr->lens()->to_json();
//    nlohmann::json json_ptz;
//    json_ptz["PTZ"] = m_device_mgr->ptz()->to_json();
//    emit save_to_preset(idx, nlohmann::json::array({json_tcu, json_lens, json_ptz}));
    emit save_to_preset(idx, nlohmann::json{{"TCU", m_device_mgr->tcu()->to_json()}, {"Lens", m_device_mgr->lens()->to_json()}, {"PTZ", m_device_mgr->ptz()->to_json()}});
}

#if DEPRECATED_PRESET_STRUCT
void UserPanel::apply_preset(PresetData preset)
{
    m_tcu_ctrl->set_delay_dist(preset.delay_a * m_tcu_ctrl->get_dist_ns());
//    ptr_tcu->delay_n = preset.delay_n;
    emit send_double_tcu_msg(TCU::DELAY_N, preset.delay_n);
    update_delay();
    m_tcu_ctrl->set_depth_of_view(preset.gw_a * m_tcu_ctrl->get_dist_ns());
//    ptr_tcu->gate_width_n = preset.gatewidth_n;
    emit send_double_tcu_msg(TCU::GATE_WIDTH_N, preset.gatewidth_n);
    update_gate_width();
    change_mcp(preset.mcp);
    m_lens_ctrl->set_zoom(preset.zoom);
    m_lens_ctrl->set_focus(preset.focus);
    ui->ZOOM_EDIT->setText(QString::number(preset.zoom));
    ui->FOCUS_EDIT->setText(QString::number(preset.focus));
    set_zoom();
    set_focus();
    change_focus_speed(preset.lens_speed);
    m_device_mgr->set_angle_h(preset.angle_h);
    m_device_mgr->set_angle_v(preset.angle_v);
    m_device_mgr->set_ptz_angle();
    ui->PTZ_SPEED_SLIDER->setValue(preset.ptz_speed);
}
#endif

void UserPanel::apply_preset(nlohmann::json preset_data)
{
    if (preset_data.contains("TCU") && preset_data.contains("Lens") && preset_data.contains("PTZ")) {
        m_device_mgr->tcu()->emit load_json(preset_data["TCU"]);
        m_device_mgr->lens()->emit load_json(preset_data["Lens"]);
        m_device_mgr->ptz()->emit load_json(preset_data["PTZ"]);
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
    static QStringList file_paths = (QStringList() << "rtsp://192.168.1.xxx/mainstream"
                                     << "rtsp://admin:abcd1234@192.168.1.xxx:554/h264/ch1/main/av_stream"
                                     << "rtsp://admin:abcd1234@192.168.1.xxx:554");
    static QStringList sp_paths = (QStringList() << "https://d3s7x6kmqcnb6b.cloudfront.net/hls/TW6CI6Y/m.m3u8"
                                   << "https://jmp2.uk/plu-64edf6eaa7ec0d000812f58c.m3u8"
                                   << "https://travelxp-travelxp-1-nz.samsung.wurl.tv/playlist.m3u8"
                                   << "https://ntv1.akamaized.net/hls/live/2014075/NASA-NTV1-HLS/master.m3u8");
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

// update_light_speed → TCUController (inline delegation in header)

// convert_to_send_tcu → TCUController (inline delegation in header)

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

    if (true /* use_tcp always false; legacy serial path */) {
        if (!m_device_mgr->serial(id)->isOpen()) {
            port_mutex.unlock();
//            image_mutex.unlock();
            return QByteArray();
        }
        m_device_mgr->serial(id)->clear();
        qint64 bytes_written = m_device_mgr->serial(id)->write(write, write_size);
        if (bytes_written == -1) {
            qWarning("Serial port write error on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }

        int write_retries = 5; // 5 * 10ms = 50ms max
        while (m_device_mgr->serial(id)->waitForBytesWritten(10) && --write_retries > 0 && m_device_mgr->serial(id)->isOpen()) ;

        if (fb && m_device_mgr->serial(id)->isOpen()) {
            int read_retries = 2; // 2 * 100ms = 200ms max
            while (m_device_mgr->serial(id)->waitForReadyRead(100) && --read_retries > 0 && m_device_mgr->serial(id)->isOpen()) ;
        }

        if (!m_device_mgr->serial(id)->isOpen()) {
            qWarning("Serial port disconnected during communication on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }

        read = read_size ? m_device_mgr->serial(id)->read(read_size) : m_device_mgr->serial(id)->readAll();
        for (char i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
        emit append_text(str_r);
    }
    else {
        if (!m_device_mgr->tcp(id)->isOpen()) {
            port_mutex.unlock();
//            image_mutex.unlock();
            return QByteArray();
        }
        qint64 bytes_written = m_device_mgr->tcp(id)->write(write, write_size);
        if (bytes_written == -1) {
            qWarning("TCP port write error on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }

        int write_retries = 5; // 5 * 10ms = 50ms max
        while (m_device_mgr->tcp(id)->waitForBytesWritten(10) && --write_retries > 0 && m_device_mgr->tcp(id)->isOpen()) ;

        if (fb && m_device_mgr->tcp(id)->isOpen()) {
            int read_retries = 2; // 2 * 100ms = 200ms max
            while (m_device_mgr->tcp(id)->waitForReadyRead(100) && --read_retries > 0 && m_device_mgr->tcp(id)->isOpen()) ;
        }

        if (!m_device_mgr->tcp(id)->isOpen()) {
            qWarning("TCP port disconnected during communication on port %d", id);
            port_mutex.unlock();
            return QByteArray();
        }

        read = read_size ? m_device_mgr->tcp(id)->read(read_size) : m_device_mgr->tcp(id)->readAll();
        for (char i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
        emit append_text(str_r);
    }

    port_mutex.unlock();
//    image_mutex.unlock();
    QThread().msleep(10);
    return read;
}

// update_laser_width → TCUController (inline delegation in header)

// update_delay → TCUController (inline delegation in header)

// update_gate_width → TCUController (inline delegation in header)

// update_tcu_params → TCUController (inline delegation in header)

void UserPanel::filter_scan()
{
    static cv::Mat filter = img_mem[0].clone();
    filter = img_mem[0] / 64;
//    qDebug("ratio %f\n", cv::countNonZero(filter) / (float)filter.total());
//    if (cv::countNonZero(filter) / (float)filter.total() > 0.3) /*scan = !scan, *//*emit update_scan(true);*/q_scan.push_back(ptr_tcu->get_tcu_params());
}

// auto_scan_for_target → ScanController (inline delegation in header)

// update_current → TCUController (inline delegation in header)

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
        { const auto& ip = config->get_data().image_proc;
        out << ip.accu_base << ip.gamma << ip.low_in << ip.low_out << ip.high_in << ip.high_out << ip.dehaze_pct << ip.sky_tolerance << ip.fast_gf; }
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
        { auto& ip = config->get_data().image_proc;
        out >> ip.accu_base >> ip.gamma >> ip.low_in >> ip.low_out >> ip.high_in >> ip.high_out >> ip.dehaze_pct >> ip.sky_tolerance >> ip.fast_gf; }
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

// connect_to_serial_server_tcp, disconnect_from_serial_server_tcp → DeviceManager

void UserPanel::on_DIST_BTN_clicked() {
    int distance_value;
    if (m_device_mgr->serial(1)->isOpen()) {
        QByteArray read = communicate_display(1, QByteArray(1, 0xA5), 1, 6, true);
//        qDebug("%s", read_dist.toLatin1().data());

        read.clear();
        uchar buf_dist[] = {0xEE, 0x16, 0x02, 0x03, 0x02, 0x05};
        read = communicate_display(1, QByteArray((char*)buf_dist, 6), 6, 10, true);
//        qDebug("%s", read_dist.toLatin1().data());
//        communicate_display(com[1], 7, 7, true);
//        QString ascii;
//        for (int i = 0; i < 7; i++) ascii += QString::asprintf(" %2c", in_buffer[i]);
//        emit append_text("received " + ascii);

        distance_value = read[7] + (read[6] << 8);
    }
    else {
        bool ok = false;
        distance_value = QInputDialog::getInt(this, "DISTANCE INPUT", "DETECTED DISTANCE: ", 100, 100, config->get_data().tcu.max_dist, 100, &ok, Qt::FramelessWindowHint);
        if (!ok) return;
    }

    m_tcu_ctrl->apply_distance(distance_value);
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
    if (!arg1) m_tcu_ctrl->set_frame_a_3d(false)/*, prev_3d[0] = cv::Mat(w[0], h[0], CV_8UC3)*/;
    image_mutex[0].unlock();
    update_processing_params();

    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);

    if (simple_ui) {
        emit send_double_tcu_msg(TCU::DELAY_N, arg1 ? m_tcu_ctrl->get_depth_of_view() / m_tcu_ctrl->get_dist_ns() : 0);
        if (pref->ui->AUTO_MCP_CHK->isChecked()) pref->ui->AUTO_MCP_CHK->click();
    }
}

// on_ZOOM_IN/OUT_BTN_pressed/released, on_FOCUS_FAR/NEAR_BTN_pressed/released,
// on_RADIUS_INC/DEC_BTN_pressed/released, focus_far, focus_near → LensController (inline delegation in header)

// set_laser_preset_target, goto_laser_preset → LaserController (inline delegation in header)

// lens_stop, set_zoom, set_focus → LensController (inline delegation in header)


// start_laser, init_laser → LaserController (inline delegation in header)

// change_mcp → TCUController (inline delegation in header)

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

// change_delay → TCUController (inline delegation in header)

// change_gatewidth → TCUController (inline delegation in header)

// change_focus_speed, on_*_BTN_released → LensController (inline delegation in header)

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
////                    if (m_device_mgr->serial(i]) m_device_mgr->serial(i]->close();
////                    setup_serial_port(serial_port + i, i, edit->text(), 9600);
//                }
//            }
            if (edit == ui->FREQ_EDIT) {
                switch (m_tcu_ctrl->get_hz_unit()) {
                // kHz
                case 0: m_tcu_ctrl->set_rep_freq(ui->FREQ_EDIT->text().toFloat()); break;
                // Hz
                case 1: m_tcu_ctrl->set_rep_freq(ui->FREQ_EDIT->text().toFloat() / 1000); break;
                default: break;
                }
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x00, 1.25e5 / ptr_tcu->rep_freq), 7, 1, false);
//                ptr_tcu->set_user_param(TCUThread::REPEATED_FREQ, rep_freq);
                emit send_double_tcu_msg(TCU::REPEATED_FREQ, m_tcu_ctrl->get_rep_freq());
            }
            else if (edit == ui->LASER_WIDTH_EDIT_U) {
                m_tcu_ctrl->set_laser_width(edit->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt() + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.);
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//                ptr_tcu->set_user_param(TCU::LASER_WIDTH, laser_width);
                update_laser_width();
                if (simple_ui) ;
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_U) {
                m_tcu_ctrl->set_depth_of_view((edit->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt() + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.) * m_tcu_ctrl->get_dist_ns());
                update_gate_width();
            }
            else if (edit == ui->GATE_WIDTH_B_EDIT_U) {
//                depth_of_view = (edit->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->gate_width_n) * dist_ns;
                if (m_device_mgr->tcu()->get(TCU::AB_LOCK)) {
                    m_tcu_ctrl->set_depth_of_view((edit->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N)) * m_tcu_ctrl->get_dist_ns());
                    update_gate_width();
                }
                else {
                    double temp_gw_b;
                    temp_gw_b = edit->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N);
                    emit send_double_tcu_msg(TCU::GATE_WIDTH_B, temp_gw_b);
                }
            }
            else if (edit == ui->LASER_WIDTH_EDIT_N) {
                if (edit->text().toInt() > 999) m_tcu_ctrl->set_laser_width(edit->text().toInt() + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.);
                else m_tcu_ctrl->set_laser_width(edit->text().toInt() + ui->LASER_WIDTH_EDIT_U->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_P->text().toInt() / 1000.);
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//                ptr_tcu->set_user_param(TCU::LASER_WIDTH, ptr_tcu->laser_width);
                update_laser_width();
                if (simple_ui) ;
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_N) {
                if (edit->text().toInt() > 999) m_tcu_ctrl->set_depth_of_view((edit->text().toInt() + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.) * m_tcu_ctrl->get_dist_ns());
                else m_tcu_ctrl->set_depth_of_view((edit->text().toInt() + ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_P->text().toInt() / 1000.) * m_tcu_ctrl->get_dist_ns());
                update_gate_width();
            }
            else if (edit == ui->GATE_WIDTH_B_EDIT_N) {
//                if (edit->text().toInt() > 999) depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->gate_width_n) * dist_ns;
//                else depth_of_view = (edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->gate_width_n) * dist_ns;
                if (m_device_mgr->tcu()->get(TCU::AB_LOCK)) {
                    if (edit->text().toInt() > 999) m_tcu_ctrl->set_depth_of_view((edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N)) * m_tcu_ctrl->get_dist_ns());
                    else m_tcu_ctrl->set_depth_of_view((edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N)) * m_tcu_ctrl->get_dist_ns());
                    update_gate_width();
                }
                else {
                    double temp_gw_b;
                    if (edit->text().toInt() > 999) temp_gw_b = edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N);
                    else temp_gw_b = edit->text().toInt() + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N);
                    emit send_double_tcu_msg(TCU::GATE_WIDTH_B, temp_gw_b);
                }
            }
            else if (edit == ui->LASER_WIDTH_EDIT_P) {
                m_tcu_ctrl->set_laser_width(edit->text().toInt() / 1000. + ui->LASER_WIDTH_EDIT_U->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt());
//                ptr_tcu->communicate_display(convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
//                ptr_tcu->set_user_param(TCU::LASER_WIDTH, ptr_tcu->laser_width);
                update_laser_width();
                if (simple_ui) ;
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_P) {
                m_tcu_ctrl->set_depth_of_view((edit->text().toInt() / 1000. + ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt()) * m_tcu_ctrl->get_dist_ns());
                update_gate_width();
            }
            else if (edit == ui->GATE_WIDTH_B_EDIT_P) {
//                depth_of_view = (edit->text().toInt() / 1000. + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() - ptr_tcu->gate_width_n) * dist_ns;
                if (m_device_mgr->tcu()->get(TCU::AB_LOCK)) {
                    m_tcu_ctrl->set_depth_of_view((edit->text().toInt() / 1000. + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N)) * m_tcu_ctrl->get_dist_ns());
                    update_gate_width();
                }
                else {
                    double temp_gw_b;
                    temp_gw_b = edit->text().toInt() / 1000. + ui->GATE_WIDTH_B_EDIT_U->text().toInt() * 1000 + ui->GATE_WIDTH_B_EDIT_N->text().toInt() - m_device_mgr->tcu()->get(TCU::GATE_WIDTH_N);
                    emit send_double_tcu_msg(TCU::GATE_WIDTH_B, temp_gw_b);
                }
            }
            else if (edit == ui->DELAY_A_EDIT_U) {
                m_tcu_ctrl->set_delay_dist((edit->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.) * m_tcu_ctrl->get_dist_ns());
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_U) {
//                delay_dist = (edit->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000. - ptr_tcu->delay_n) * dist_ns;
                if (m_device_mgr->tcu()->get(TCU::AB_LOCK)) {
                    m_tcu_ctrl->set_delay_dist((edit->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::DELAY_N)) * m_tcu_ctrl->get_dist_ns());
                    update_delay();
                }
                else {
                    double temp_delay_b;
                    temp_delay_b = edit->text().toInt() * 1000 + ui->DELAY_B_EDIT_N->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::DELAY_N);
                    emit send_double_tcu_msg(TCU::DELAY_B, temp_delay_b);
                }
            }
            else if (edit == ui->DELAY_A_EDIT_N) {
                if (edit->text().toInt() > 999) m_tcu_ctrl->set_delay_dist((edit->text().toInt() + ui->DELAY_A_EDIT_P->text().toInt() / 1000.) * m_tcu_ctrl->get_dist_ns());
                else m_tcu_ctrl->set_delay_dist((edit->text().toInt() + ui->DELAY_A_EDIT_U->text().toInt() * 1000+ ui->DELAY_A_EDIT_P->text().toInt() / 1000.) * m_tcu_ctrl->get_dist_ns());
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_N) {
//                if (edit->text().toInt() > 999) delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_P->text().toInt() / 1000.) * dist_ns - ptr_tcu->delay_n;
//                else delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - ptr_tcu->delay_n) * dist_ns;
                if (m_device_mgr->tcu()->get(TCU::AB_LOCK)) {
                    if (edit->text().toInt() > 999) m_tcu_ctrl->set_delay_dist((edit->text().toInt() + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::DELAY_N)) * m_tcu_ctrl->get_dist_ns());
                    else m_tcu_ctrl->set_delay_dist((edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::DELAY_N)) * m_tcu_ctrl->get_dist_ns());
                    update_delay();
                }
                else {
                    double temp_delay_b;
                    if (edit->text().toInt() > 999) temp_delay_b = edit->text().toInt() + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::DELAY_N);
                    else temp_delay_b = edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_B_EDIT_P->text().toInt() / 1000. - m_device_mgr->tcu()->get(TCU::DELAY_N);
                    emit send_double_tcu_msg(TCU::DELAY_B, temp_delay_b);
                }
            }
            else if (edit == ui->DELAY_A_EDIT_P) {
                m_tcu_ctrl->set_delay_dist((edit->text().toInt() / 1000. + ui->DELAY_A_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt()) * m_tcu_ctrl->get_dist_ns());
                update_delay();
            }
            else if (edit == ui->DELAY_B_EDIT_P) {
//                delay_dist = (edit->text().toInt() / 1000. + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() - ptr_tcu->delay_n) * dist_ns;
                if (m_device_mgr->tcu()->get(TCU::AB_LOCK)) {
                    m_tcu_ctrl->set_delay_dist((edit->text().toInt() / 1000. + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() - m_device_mgr->tcu()->get(TCU::DELAY_N)) * m_tcu_ctrl->get_dist_ns());
                    update_delay();
                }
                else {
                    double temp_delay_b;
                    temp_delay_b = edit->text().toInt() / 1000. + ui->DELAY_B_EDIT_U->text().toInt() * 1000 + ui->DELAY_A_EDIT_N->text().toInt() - m_device_mgr->tcu()->get(TCU::DELAY_N);
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
                switch (m_tcu_ctrl->get_base_unit()) {
                case 0: m_tcu_ctrl->set_stepping(edit->text().toFloat()); break;
                case 1: m_tcu_ctrl->set_stepping(edit->text().toFloat() * 1000); break;
                case 2: m_tcu_ctrl->set_stepping(edit->text().toFloat() / m_tcu_ctrl->get_dist_ns()); break;
                default: break;
                }
                setup_stepping(m_tcu_ctrl->get_base_unit());
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
                float ah = ui->ANGLE_H_EDIT->text().toDouble();
                ah = fmod(ah + 360.0, 360.0);
                m_device_mgr->set_angle_h(ah);
                ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", ah));
                m_device_mgr->send_ptz_angle_h(ah);
            }
            else if (edit == ui->ANGLE_V_EDIT) {
                float av = ui->ANGLE_V_EDIT->text().toDouble();
                m_device_mgr->set_angle_v(av);
                ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", av));
                m_device_mgr->send_ptz_angle_v(av);
            }
            this->focusWidget()->clearFocus();
            break;
        // 100m => 667ns, 10m => 67ns
        case Qt::Key_W:
            m_tcu_ctrl->set_delay_dist(m_tcu_ctrl->get_delay_dist() + m_tcu_ctrl->get_stepping() * 5 * m_tcu_ctrl->get_dist_ns());
            update_delay();
            break;
        case Qt::Key_S:
            m_tcu_ctrl->set_delay_dist(m_tcu_ctrl->get_delay_dist() - m_tcu_ctrl->get_stepping() * 5 * m_tcu_ctrl->get_dist_ns());
            update_delay();
            break;
        case Qt::Key_D:
            m_tcu_ctrl->set_delay_dist(m_tcu_ctrl->get_delay_dist() + m_tcu_ctrl->get_stepping() * m_tcu_ctrl->get_dist_ns());
            update_delay();
            break;
        case Qt::Key_A:
            m_tcu_ctrl->set_delay_dist(m_tcu_ctrl->get_delay_dist() - m_tcu_ctrl->get_stepping() * m_tcu_ctrl->get_dist_ns());
            update_delay();
            break;
        // 50m => 333ns, 5m => 33ns
        case Qt::Key_I:
            m_tcu_ctrl->set_depth_of_view(m_tcu_ctrl->get_depth_of_view() + m_tcu_ctrl->get_stepping() * 5 * m_tcu_ctrl->get_dist_ns());
            update_gate_width();
            break;
        case Qt::Key_K:
            m_tcu_ctrl->set_depth_of_view(m_tcu_ctrl->get_depth_of_view() - m_tcu_ctrl->get_stepping() * 5 * m_tcu_ctrl->get_dist_ns());
            update_gate_width();
            break;
        case Qt::Key_L:
            m_tcu_ctrl->set_depth_of_view(m_tcu_ctrl->get_depth_of_view() + m_tcu_ctrl->get_stepping() * m_tcu_ctrl->get_dist_ns());
            update_gate_width();
            break;
        case Qt::Key_J:
            m_tcu_ctrl->set_depth_of_view(m_tcu_ctrl->get_depth_of_view() - m_tcu_ctrl->get_stepping() * m_tcu_ctrl->get_dist_ns());
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
//            std::accumulate(use_tcp, use_tcp + 5, 0) ? m_device_mgr->disconnect_from_serial_server_tcp() : m_device_mgr->connect_to_serial_server_tcp();
//            (ptr_tcu->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
//            (ptr_lens->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
//            (ptr_laser->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ||
//            (ptr_ptz->get_port_status() & ControlPortThread::PortStatus::TCP_CONNECTED) ?
//                m_device_mgr->disconnect_from_serial_server_tcp() : m_device_mgr->connect_to_serial_server_tcp();
            (m_device_mgr->tcu()->get_port_status() & ControlPort::TCP_CONNECTED) ||
            (m_device_mgr->lens()->get_port_status() & ControlPort::TCP_CONNECTED) ||
            (m_device_mgr->laser()->get_port_status() & ControlPort::TCP_CONNECTED) ||
            (m_device_mgr->ptz()->get_port_status() & ControlPort::TCP_CONNECTED) ?
                m_device_mgr->disconnect_from_serial_server_tcp() : m_device_mgr->connect_to_serial_server_tcp(pref->ui->TCP_SERVER_IP_EDIT->text());
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
            aliasing->show_ui();
            break;
        case Qt::Key_D:
            aliasing->update_distance(m_tcu_ctrl->get_delay_dist());
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

    update_processing_params();
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

            bool is_rtsp_stream = filename.contains("rtsp://");
            bool is_http_stream = filename.contains("http://") || filename.contains("https://");
            bool is_local_file = !is_rtsp_stream && !is_http_stream;

            AVFormatContext *format_context = avformat_alloc_context();
            std::shared_ptr<AVFormatContext*> closer_format_context(&format_context, avformat_close_input);
            AVCodecParameters *codec_param = NULL;
            //        std::shared_ptr<AVCodecParameters*> deleter_codec_param(&codec_param, avcodec_parameters_free);
            const AVCodec *codec = NULL;
            AVCodecContext *codec_context = avcodec_alloc_context3(NULL);
            std::shared_ptr<AVCodecContext*> closer_codec_context(&codec_context, avcodec_free_context);
            AVFrame *frame = av_frame_alloc();
            std::shared_ptr<AVFrame*> deleter_frame(&frame, av_frame_free);
            AVFrame *frame_sw = av_frame_alloc();
            std::shared_ptr<AVFrame*> deleter_frame_sw(&frame_sw, av_frame_free);
            AVFrame *frame_result = av_frame_alloc();
            std::shared_ptr<AVFrame*> deleter_frame_result(&frame_result, av_frame_free);
            AVFrame *frame_filter = av_frame_alloc();
            std::shared_ptr<AVFrame*> deleter_frame_filter(&frame_filter, av_frame_free);
            uint8_t *buffer = NULL;
            std::shared_ptr<uint8_t> deleter_buffer(buffer, av_free);
            SwsContext *sws_context = NULL;
            std::shared_ptr<SwsContext> deleter_sws_context(sws_context, sws_freeContext);
            AVFilterContext *buffersink_ctx = nullptr;
            AVFilterContext *buffersrc_ctx = nullptr;
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

            // Helper lambda for cleanup on early return with error code
            auto cleanup_and_return = [&](int error_code) {
                if (display) display_mutex[display_idx].unlock();
                if (!display) this->video_stopped();
                qDebug() << "FFmpeg video capture failed with error code:" << error_code;
                return error_code;
            };

            // open input video — only use dshow for local devices (e.g. "video=...")
            AVInputFormat *input_format = nullptr;
            if (filename.contains("video=") || filename.contains("audio="))
                input_format = (AVInputFormat *)av_find_input_format("dshow");
            AVDictionary *format_opts = nullptr;
            if (is_rtsp_stream) {
                av_dict_set(&format_opts, "rtsp_transport", "tcp", 0);
                av_dict_set(&format_opts, "buffer_size", "2097152", 0);
            }
            start_time = time(NULL);
            if (avformat_open_input(&format_context, filename.toUtf8().constData(), input_format, &format_opts) != 0) { av_dict_free(&format_opts); return cleanup_and_return(-2); }
            av_dict_free(&format_opts);

            // fetch video info
            if (avformat_find_stream_info(format_context, NULL) < 0) return cleanup_and_return(-2);

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
            if (video_stream_idx == -1) return cleanup_and_return(-2);

            // point the codec parameter to the first stream's
            codec_param = format_context->streams[video_stream_idx]->codecpar;

            if (display) emit send_double_tcu_msg(TCU::CCD_FREQ, frame_rate_edit = std::round(format_context->streams[video_stream_idx]->r_frame_rate.num / format_context->streams[video_stream_idx]->r_frame_rate.den));

            codec = avcodec_find_decoder(codec_param->codec_id);
            if (codec == NULL) return cleanup_and_return(-3);

            if (avcodec_parameters_to_context(codec_context, codec_param) < 0) return cleanup_and_return(-3);

            // try HW-accelerated decoding (D3D11VA), fall back to multi-threaded SW
            AVBufferRef *hw_device_ctx = nullptr;
            bool hw_decoding = false;
            if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) == 0) {
                codec_context->hw_device_ctx = av_buffer_ref(hw_device_ctx);
                hw_decoding = true;
                qDebug() << "FFmpeg: using D3D11VA hardware decoding";
            } else {
                qDebug() << "FFmpeg: D3D11VA unavailable, using multi-threaded software decoding";
            }
            if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);

            codec_context->thread_count = 0;
            codec_context->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

            if (avcodec_open2(codec_context, codec, NULL) < 0) {
                // if HW open failed, retry with SW
                if (hw_decoding) {
                    qDebug() << "FFmpeg: HW codec open failed, retrying with software decoding";
                    hw_decoding = false;
                    avcodec_free_context(&codec_context);
                    codec_context = avcodec_alloc_context3(NULL);
                    if (avcodec_parameters_to_context(codec_context, codec_param) < 0) return cleanup_and_return(-3);
                    codec_context->thread_count = 0;
                    codec_context->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
                    if (avcodec_open2(codec_context, codec, NULL) < 0) return cleanup_and_return(-3);
                } else {
                    return cleanup_and_return(-3);
                }
            }

            cv::Mat cv_frame(codec_context->height, codec_context->width, CV_MAKETYPE(CV_8U, format_gray ? 1 : 3));

            // allocate data buffer (alignment=32 for SIMD-friendly sws_scale)
            AVPixelFormat pixel_fmt = format_gray ? AV_PIX_FMT_NV12 : AV_PIX_FMT_RGB24;
            int byte_num = av_image_get_buffer_size(pixel_fmt, codec_context->width, codec_context->height, 32);
            buffer = (uint8_t *)av_malloc(byte_num * sizeof(uint8_t));

            av_image_fill_arrays(frame_result->data, frame_result->linesize, buffer, pixel_fmt, codec_context->width, codec_context->height, 32);

            // sws_context created lazily — HW decode produces different pix_fmt than codec_context reports
            AVPixelFormat sws_src_fmt = AV_PIX_FMT_NONE;

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
                        // transfer HW frame to CPU memory if needed
                        AVFrame *sw_frame = frame;
                        if (hw_decoding && frame->format != AV_PIX_FMT_NONE && frame->hw_frames_ctx) {
                            av_frame_unref(frame_sw);
                            if (av_hwframe_transfer_data(frame_sw, frame, 0) < 0) {
                                av_frame_unref(frame);
                                continue;
                            }
                            sw_frame = frame_sw;
                        }

                        if (format_gray) {
                            // push the decoded frame into the filtergraph
                            if (av_buffersrc_add_frame_flags(buffersrc_ctx, sw_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) break;

                            // pull filtered frames from the filtergraph
                            av_frame_unref(frame_filter);
                            int ret = av_buffersink_get_frame(buffersink_ctx, frame_filter);
                            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                        }
                        else {
                            if (is_local_file && display && sw_frame->pts != AV_NOPTS_VALUE) {
                                if (last_pts != AV_NOPTS_VALUE) {
                                    delay = av_rescale_q(sw_frame->pts - last_pts, time_base, time_base_q) - elapsed_timer.nsecsElapsed() / 1e6;
//                                    qDebug() << av_rescale_q(sw_frame->pts - last_pts, time_base, time_base_q) << elapsed_timer.nsecsElapsed() / 1000;
                                    if (delay > 0 && delay < 1000000) av_usleep(delay);
                                    elapsed_timer.restart();
                                }
                                last_pts = sw_frame->pts;
                            }
                            // lazily create/recreate sws_context on first frame or format change
                            if (sws_src_fmt != (AVPixelFormat)sw_frame->format) {
                                if (sws_context) sws_freeContext(sws_context);
                                sws_src_fmt = (AVPixelFormat)sw_frame->format;
                                sws_context = sws_getContext(codec_context->width, codec_context->height, sws_src_fmt,
                                                             codec_context->width, codec_context->height, pixel_fmt, SWS_BILINEAR, NULL, NULL, NULL);
                            }
                            sws_scale(sws_context, sw_frame->data, sw_frame->linesize, 0, codec_context->height, frame_result->data, frame_result->linesize);
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


void UserPanel::rotate(int angle)
{
    image_mutex[0].lock();
    image_rotate[0] = angle;
    image_mutex[0].unlock();
    update_processing_params();

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
        while (grab_thread_state[select_display_thread].load()) QThread::msleep(1);
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
    h_grab_thread[display_idx] = new GrabThread(m_pipeline, display_idx);
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
        raw_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_raw.mp4");
//        vid_out[0].open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w[0], h[0]), is_color[0]);
        vid_out[0].open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), frame_rate_edit, cv::Size(w[0], h[0]), is_color[0]);
        image_mutex[0].unlock();
    }
    record_original[0] = !record_original[0];
    ui->SAVE_AVI_BUTTON->setText(record_original[0] ? tr("STOP") : tr("ORI"));
}

// on_SCAN_BUTTON_clicked, on_CONTINUE_SCAN_BUTTON_clicked, on_RESTART_SCAN_BUTTON_clicked,
// on_SCAN_CONFIG_BTN_clicked, enable_scan_options → ScanController (inline delegations in header)

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
    if (device_type == -1 || !config->get_data().save.consecutive_capture) save_modified[0] = 1;
    else{
        save_modified[0] = !save_modified[0];
        ui->SAVE_RESULT_BUTTON->setText(save_modified[0] ? tr("STOP") : tr("RES"));
        pref->ui->CONSECUTIVE_CAPTURE_CHK->setEnabled(!save_modified[0]);
    }
}

// on_IRIS_*_BTN_pressed/released → LensController (inline delegation in header)

// laser_preset_reached, on_LASER_BTN_clicked → LaserController (inline delegation in header)

// on_GET_LENS_PARAM_BTN_clicked, on_AUTO_FOCUS_BTN_clicked → LensController (inline delegation in header)

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
    m_aux_panel->select_display(ui->MISC_OPTION_1->currentIndex());
}

void UserPanel::on_MISC_RADIO_2_clicked()
{
    m_aux_panel->select_display(ui->MISC_OPTION_2->currentIndex());
}

void UserPanel::on_MISC_RADIO_3_clicked()
{
    m_aux_panel->select_display(ui->MISC_OPTION_3->currentIndex());
}

void UserPanel::on_MISC_OPTION_1_currentIndexChanged(int index)
{
    m_aux_panel->select_display(index);
    ui->MISC_RADIO_1->setChecked(true);
}

void UserPanel::on_MISC_OPTION_2_currentIndexChanged(int index)
{
    m_aux_panel->select_display(index);
    ui->MISC_RADIO_2->setChecked(true);
}

void UserPanel::on_MISC_OPTION_3_currentIndexChanged(int index)
{
    m_aux_panel->select_display(index);
    ui->MISC_RADIO_3->setChecked(true);
}

void UserPanel::set_zoom()
{
    m_lens_ctrl->set_zoom_value(ui->ZOOM_EDIT->text().toUInt());
}

void UserPanel::set_focus()
{
    m_lens_ctrl->set_focus_value(ui->FOCUS_EDIT->text().toUInt());
}

void UserPanel::update_current()
{
    m_tcu_ctrl->update_current(ui->CURRENT_EDIT->text().toFloat());
}
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

        if (!m_device_mgr->serial(0)->isOpen()) continue;
        m_device_mgr->serial(0)->clear();
        m_device_mgr->serial(0)->write(cmd, write_size);
        int write_retries = 5; // 5 * 10ms = 50ms max
        while (m_device_mgr->serial(0)->waitForBytesWritten(10) && --write_retries > 0) ;

        int read_retries = 2; // 2 * 100ms = 200ms max
        while (m_device_mgr->serial(0)->waitForReadyRead(100) && --read_retries > 0) ;

        QByteArray read = m_device_mgr->serial(0)->readAll();
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
    Q_UNUSED(dir);
    // Dead code — Pelco-D frame building was commented out; PTZ control now via IPTZController
}

// PTZ button handlers, VID camera handlers, PTZ speed/angle handlers → DeviceManager

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
            raw_avi[display_idx - 1] = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_raw_alt" + QString::number(display_idx) + ".mp4");
//            vid_out[display_idx + 1].open(raw_avi[display_idx - 1].toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w[display_idx], h[display_idx]), is_color[display_idx]);
            vid_out[display_idx + 1].open(raw_avi[display_idx - 1].toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), frame_rate_edit, cv::Size(w[display_idx], h[display_idx]), is_color[display_idx]);
            image_mutex[display_idx].unlock();
        }
        record_modified[display_idx] = !record_modified[display_idx];

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
    m_tcu_ctrl->set_frame_a_3d(!m_tcu_ctrl->get_frame_a_3d());
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

// on_FIRE_LASER_BTN_clicked → LaserController (inline delegation in header)

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

// on_SWITCH_TCU_UI_BTN_clicked → TCUController (inline delegation in header)

// on_SIMPLE_LASER_CHK_clicked → TCUController (inline delegation in header)

// on_AUTO_MCP_CHK_clicked → TCUController (inline delegation in header)

void UserPanel::on_PSEUDOCOLOR_CHK_stateChanged(int arg1)
{
    image_mutex[0].lock();
    pseudocolor[0] = arg1;
    image_mutex[0].unlock();
    update_processing_params();
}


