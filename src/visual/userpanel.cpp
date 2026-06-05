#include "userpanel.h"
#include "ui_user_panel.h"
#include "ui_preferences.h"
#include "util/constants.h"
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStyle>
#include <QVBoxLayout>

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

ConsumerThread::ConsumerThread(PipelineProcessor *pipeline, DisplayRouter *router, DisplaySlot *display)
    : m_pipeline(pipeline), m_router(router), m_display(display)
{
}

ConsumerThread::~ConsumerThread()
{
    quit();
    wait();
}

void ConsumerThread::run()
{
    if (m_pipeline && m_router && m_display) m_pipeline->run(*m_router, *m_display);
}

TimedQueue::TimedQueue(double time) : QThread(), _quit(false), expiration_time(time), fps_status(NULL) {}
TimedQueue::~TimedQueue() {
    _quit = true;
    while (!q.empty()) q.pop();
}

inline void TimedQueue::set_display_label(QLabel *lbl) { fps_status = lbl; }

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
    QElapsedTimer et;
    while (!_quit) {
#if 1
        QThread::msleep(5);
        struct timespec t;
        timespec_get(&t, TIME_UTC);
        if (q.empty()) { continue; }
        mtx.lock();
        if (diff_timespec(q.front(), t) > expiration_time * 1e3) q.pop();
        if (et.isValid() && et.elapsed() < 200) { mtx.unlock(); continue; }
        QMetaObject::invokeMethod(fps_status, "setText", Qt::QueuedConnection,
                                  Q_ARG(QString, QString::number(q.size() ? ((q.size() - 1) / diff_timespec(q.front(), q.back())) * 1000 : 0.f, 'f', 2) + " fps"));
        mtx.unlock();
        et.restart();
#endif
    }
}

void UserPanel::position_display_overlays(DisplayId did)
{
    DisplaySlot& d = m_router.display(did);
    Display* w = d.widget;
    if (!w) return;

    auto map_pos = [this, did, w](const QPoint& p) {
        return did == DisplayId::Main ? w->mapTo(ui->CENTRAL, p) : p;
    };

    if (QLabel* fps = d.fps_label) {
        fps->adjustSize();
        const QPoint p(6, qMax(0, w->height() - fps->height() - 6));
        fps->move(map_pos(p));
        fps->raise();
    }

    if (QLabel* sd = d.source_dead_label) {
        sd->adjustSize();
        const QPoint p(qMax(6, w->width() - sd->width() - 6),
                       qMax(0, w->height() - sd->height() - 6));
        sd->move(map_pos(p));
        sd->raise();
    }
}

void UserPanel::set_source_dead_overlay(DisplayId did, bool dead)
{
    if (did == DisplayId::_Count) return;

    DisplaySlot& d = m_router.display(did);
    d.source_dead = dead;
    QLabel* sd = d.source_dead_label;
    if (!sd) return;

    position_display_overlays(did);
    if (!dead) {
        sd->hide();
        return;
    }

    const bool any_overlay = (m_setup_overlay && m_setup_overlay->isVisible())
                          || (m_scan_overlay  && m_scan_overlay ->isVisible());
    sd->setVisible(did != DisplayId::Main || !any_overlay);
    sd->raise();
}

UserPanel::UserPanel(QWidget *parent) :
    QMainWindow(parent),
    mouse_pressed(false),
    ui(new Ui::UserPanel),
    m_center_toolbar(nullptr),
    m_pbar(nullptr),
    m_optics_panel(nullptr),
    m_tcu_panel(nullptr),
    m_console_panel(nullptr),
    m_right_stack(nullptr),
    m_img_proc(nullptr),
    m_save_options(nullptr),
    m_vision(nullptr),
    m_side_display(nullptr),
    m_mapbox(nullptr),
    m_setup_overlay(nullptr),
    m_scan_overlay(nullptr),
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
    device_type(0),
    device_on(false),
    start_grabbing(false),
    main_pixel_format(PixelType_Gvsp_Mono8),
    main_pixel_depth(8),
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
    // Per-display atomics and per-stream meta self-initialize via routing.h's
    // default member initializers (StreamMeta: width=0, height=0,
    // pixel_depth=8, is_color=false, fps=0.0f, source_type=None). Sources
    // populate the real values when they start.

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
    m_router.display(DisplayId::Main).widget = ui->SOURCE_DISPLAY;
    m_router.display(DisplayId::Alt1).widget = fw_display[0]->get_display_widget();
    m_router.display(DisplayId::Alt2).widget = fw_display[1]->get_display_widget();
    secondary_display = new FloatingWindow();

    trans.load("zh_cn.qm");

    ui->CENTRAL->setMouseTracking(true);
    ui->LEFT->setMouseTracking(true);
    ui->MID->setMouseTracking(true);
    ui->RIGHT->setMouseTracking(true);
    ui->TITLE->setMouseTracking(true);
    setMouseTracking(true);
    setCursor(cursor_curr_pointer);

    // FPS overlays — main lives in CENTRAL (shares z-order with floating
    // panels per Sprint 18 fix); alt1/alt2 are children of their respective
    // Display widgets inside the FloatingWindows.
    {
        QLabel* fps_main = new QLabel("0.00 fps", ui->CENTRAL);
        fps_main->setObjectName("FPS_LABEL");
        fps_main->setAttribute(Qt::WA_TranslucentBackground);
        fps_main->setStyleSheet("color: #F5E1CE; background-color: rgba(0, 0, 0, 130); padding: 1px 4px; border-radius: 2px;");
        fps_main->hide(); // shown once resizeEvent places it
        m_router.display(DisplayId::Main).fps_label = fps_main;
    }
    for (int i = 1; i < 3; i++) {
        DisplayId did = static_cast<DisplayId>(i);
        Display* w = m_router.display(did).widget;
        QLabel* lbl = new QLabel("0.00 fps", w);
        lbl->setObjectName("FPS_LABEL");
        lbl->setAttribute(Qt::WA_TranslucentBackground);
        lbl->setStyleSheet("color: #F5E1CE; background-color: rgba(0, 0, 0, 130); padding: 1px 4px; border-radius: 2px;");
        lbl->move(6, w->height() - 20);
        lbl->show();
        lbl->raise();
        m_router.display(did).fps_label = lbl;
        w->installEventFilter(this); // reposition on Display resize
    }

    // Source-death overlay. Mirrors FPS-label parenting asymmetry: main goes
    // into ui->CENTRAL (shares z-order with floating panels), alt1/alt2 are
    // children of their Display widgets. Sits bottom-right; hidden until the
    // consumer reports closed-and-empty queue.
    {
        QLabel* lbl = new QLabel(tr("STREAM ENDED"), ui->CENTRAL);
        lbl->setObjectName("SOURCE_DEAD_LABEL");
        lbl->setAttribute(Qt::WA_TranslucentBackground);
        lbl->setStyleSheet("color: #F08080; background-color: rgba(0, 0, 0, 150); padding: 1px 4px; border-radius: 2px;");
        lbl->hide();
        m_router.display(DisplayId::Main).source_dead_label = lbl;
    }
    for (int i = 1; i < 3; i++) {
        DisplayId did = static_cast<DisplayId>(i);
        Display* w = m_router.display(did).widget;
        QLabel* lbl = new QLabel(tr("STREAM ENDED"), w);
        lbl->setObjectName("SOURCE_DEAD_LABEL");
        lbl->setAttribute(Qt::WA_TranslucentBackground);
        lbl->setStyleSheet("color: #F08080; background-color: rgba(0, 0, 0, 150); padding: 1px 4px; border-radius: 2px;");
        lbl->adjustSize();
        lbl->move(w->width() - lbl->width() - 6, w->height() - 20);
        lbl->hide();
        m_router.display(did).source_dead_label = lbl;
    }
    // Resnapshot ProcessingParams whenever any Display widget resizes — keeps
    // display_region / display_width / display_height in sync with the widget,
    // so the pipeline's crop+scale (pipelineprocessor.cpp:632/652) targets the
    // current widget size. Without this, fullscreen toggle stretches the
    // stale pre-resize crop to fill the new widget.
    for (int i = 0; i < 3; i++)
        connect(m_router.display(static_cast<DisplayId>(i)).widget, &Display::widget_resized, this, [this]() { update_processing_params(); });

    // center toolbar: reparent existing Tools + INFO_CHECK into a framed row
    m_center_toolbar = new QFrame(ui->MID);
    m_center_toolbar->setObjectName("CENTER_TOOLBAR");
    {
        auto *tl = new QHBoxLayout(m_center_toolbar);
        tl->setContentsMargins(4, 2, 4, 2);
        tl->setSpacing(4);
        // Uniform 24×24 so the three tool buttons read as a row, not as
        // QRadioButton sizeHints (which differ per indicator style).
        for (auto *t : { ui->ZOOM_TOOL, ui->SELECT_TOOL, ui->PTZ_TOOL }) {
            t->setParent(m_center_toolbar);
            t->setFixedSize(24, 24);
            tl->addWidget(t);
        }
        tl->addSpacing(6);
        ui->INFO_CHECK ->setParent(m_center_toolbar); tl->addWidget(ui->INFO_CHECK);
        m_center_toolbar->adjustSize();
    }

    // panel toggle bar: 3 checkable buttons, positioned top-right in resizeEvent
    m_pbar = new QFrame(ui->CENTRAL);
    m_pbar->setObjectName("PANEL_TOGGLE_BAR");
    {
        auto *pl = new QHBoxLayout(m_pbar);
        pl->setContentsMargins(3, 2, 3, 2);
        pl->setSpacing(3);
        auto makeToggle = [&](const char *text, const char *name) -> QPushButton * {
            auto *btn = new QPushButton(text, m_pbar);
            btn->setObjectName(name);
            btn->setCheckable(true);
            btn->setChecked(true);
            btn->setFixedHeight(20);
            // stretch=1 so the 3 buttons spread evenly across the pbar row.
            pl->addWidget(btn, 1);
            return btn;
        };
        auto *optics_btn  = makeToggle("Optics",  "PBAR_OPTICS_BTN");
        auto *tcu_btn     = makeToggle("TCU",     "PBAR_TCU_BTN");
        auto *console_btn = makeToggle("Console", "PBAR_CONSOLE_BTN");
        m_pbar->adjustSize();
        m_pbar->raise();

        // m_optics_panel etc. are null here; lambdas guard-check at call time.
        connect(optics_btn,  &QPushButton::toggled, this, [this](bool on) { if (m_optics_panel)  m_optics_panel ->setVisible(on); });
        connect(tcu_btn,     &QPushButton::toggled, this, [this](bool on) { if (m_tcu_panel)     m_tcu_panel    ->setVisible(on); });
        connect(console_btn, &QPushButton::toggled, this, [this](bool on) { if (m_console_panel) m_console_panel->setVisible(on); });
        // hideRequested → uncheck wired in the panel construction block below (after panels exist)
    }

    tp.start();
    for (int i = 0; i < 3; i++) {
        DisplayId did = static_cast<DisplayId>(i);
        TimedQueue* tq = new TimedQueue(5);
        tq->set_display_label(m_router.display(did).fps_label);
        tq->start();
        m_router.display(did).fps_calc = tq;
    }

    QFont temp_f(consolas);
    temp_f.setPixelSize(11);

    // connect title bar to the main window (UserPanel*)
    ui->TITLE->setup(this);

    // initialize config system
    config = new Config(this);

    // SETUP overlay -- owns the legacy Preferences widgets (built via
    // Ui::Preferences::setupUi on a hidden form root, then re-parented into
    // the overlay's row cards). Must be alive before any setup-widget access.
    m_setup_overlay = new SetupOverlay(ui->CENTRAL);
    m_setup_overlay->set_config(config);
    m_setup_overlay->setup_form();
    m_setup_overlay->bind();

    connect(m_setup_overlay, SIGNAL(search_for_devices()),               this, SLOT(search_for_devices()));
    connect(m_setup_overlay, SIGNAL(query_dev_ip()),                     this, SLOT(update_dev_ip()));
    connect(m_setup_overlay, SIGNAL(set_dev_ip(int, int)),               this, SLOT(set_dev_ip(int, int)));
    connect(m_setup_overlay, SIGNAL(change_pixel_format(int)),           this, SLOT(change_pixel_format(int)));
    connect(m_setup_overlay, SIGNAL(rotate_image(int)),                  this, SLOT(rotate(int)));
    connect(m_setup_overlay, SIGNAL(device_underwater(bool)),            this, SLOT(update_light_speed(bool)));
    connect(m_setup_overlay, SIGNAL(get_port_info(int)),                 this, SLOT(display_port_info(int)));
    connect(m_setup_overlay, SIGNAL(change_baudrate(int, int)),          this, SLOT(set_baudrate(int, int)));
    connect(m_setup_overlay, SIGNAL(set_tcp_status(int, bool)),          this, SLOT(set_tcp_status_on_port(int, bool)));
    connect(m_setup_overlay, SIGNAL(share_tcu_port(bool)),               this, SLOT(set_tcu_as_shared_port(bool)));
    connect(m_setup_overlay, SIGNAL(com_write(int, QByteArray)),         this, SLOT(com_write_data(int, QByteArray)));
    connect(m_setup_overlay, SIGNAL(tcu_type_changed(int)),              this, SLOT(set_tcu_type(int)));
    connect(m_setup_overlay, SIGNAL(set_auto_mcp(bool)),                 this, SLOT(set_auto_mcp(bool)));
    connect(m_setup_overlay, SIGNAL(rep_freq_unit_changed(int)),         this, SLOT(setup_hz(int)));
    connect(m_setup_overlay, SIGNAL(base_unit_changed(int)),             this, SLOT(setup_stepping(int)));
    connect(m_setup_overlay, SIGNAL(max_dist_changed(float)),            this, SLOT(setup_max_dist(float)));
    connect(m_setup_overlay, SIGNAL(max_dov_changed(float)),             this, SLOT(setup_max_dov(float)));
    connect(m_setup_overlay, SIGNAL(delay_offset_changed(float)),        this, SLOT(update_delay_offset(float)));
    connect(m_setup_overlay, SIGNAL(gate_width_offset_changed(float)),   this, SLOT(update_gate_width_offset(float)));
    connect(m_setup_overlay, SIGNAL(laser_offset_changed(float)),        this, SLOT(update_laser_offset(float)));
    connect(m_setup_overlay, SIGNAL(ps_config_updated(bool, int, uint)), this, SLOT(update_ps_config(bool, int, uint)));
    connect(m_setup_overlay, SIGNAL(laser_toggled(int)),                 this, SLOT(setup_laser(int)));
    connect(m_setup_overlay, SIGNAL(lower_3d_thresh_updated()),          this, SLOT(update_lower_3d_thresh()));
    connect(m_setup_overlay, SIGNAL(query_tcu_param()),                  this, SLOT(reset_custom_3d_params()));

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
                if (!m_router.display(DisplayId::Main).consumer_run) return;
                cv::Rect roi = cv::Rect(p1 * m_router.stream(StreamId::S0).meta.width / ui->SOURCE_DISPLAY->width(), p2 * m_router.stream(StreamId::S0).meta.width / ui->SOURCE_DISPLAY->width());
                QMutexLocker lk(&m_roi_mutex);
                list_roi.push_back(roi);
                m_router.display(DisplayId::Main).user_mask(roi).setTo(255);
            });
    connect(ui->SOURCE_DISPLAY, &Display::clear_roi, this,
            [this](){
                if (!m_router.display(DisplayId::Main).consumer_run) return;
                QMutexLocker lk(&m_roi_mutex);
                std::vector<cv::Rect>().swap(list_roi);
                m_router.display(DisplayId::Main).user_mask = 0;
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
                const double normalized = arg1.toFloat() / (1 << m_router.stream(StreamId::S0).meta.pixel_depth);
                m_router.display(m_picked_display).pcfg.lower_3d_thresh = normalized;
                // Only Main's threshold round-trips to Config / SETUP — 3D
                // pipeline only fires on Main, so Alt1/Alt2 pcfg values are
                // session-local and don't touch persisted state.
                if (m_picked_display == DisplayId::Main) {
                    config->get_data().image_proc.lower_3d_thresh = normalized;
                    m_setup_overlay->w()->LOWER_3D_THRESH_EDT->setText(QString::number(normalized, 'f', 3));
                }
    });

    setup_slider(ui->BRIGHTNESS_SLIDER, -5, 5, 1, 2, 0);
    ui->BRIGHTNESS_SLIDER->setTickInterval(5);

    setup_slider(ui->CONTRAST_SLIDER, -10, 10, 1, 3, 0);
    ui->CONTRAST_SLIDER->setTickInterval(5);

    setup_slider(ui->GAMMA_SLIDER, 0, 20, 1, 3, 10);
    ui->GAMMA_SLIDER->setTickInterval(5);

    // GENERAL-tab per-display widgets: write to the picked display's pcfg.
    connect(ui->FRAME_AVG_CHECK, &QCheckBox::stateChanged, this, [this](int){ apply_picked_pcfg_from_widgets(); });
    connect(ui->FRAME_AVG_OPTIONS, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){ apply_picked_pcfg_from_widgets(); });
    connect(ui->IMG_3D_CHECK, &QCheckBox::stateChanged, this, [this](int){ apply_picked_pcfg_from_widgets(); });
    connect(ui->IMG_ENHANCE_CHECK, &QCheckBox::stateChanged, this, [this](int){ apply_picked_pcfg_from_widgets(); });
    connect(ui->ENHANCE_OPTIONS, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){ apply_picked_pcfg_from_widgets(); });
    connect(ui->PSEUDOCOLOR_CHK, &QCheckBox::stateChanged, this, [this](int){ apply_picked_pcfg_from_widgets(); });
    // Global params (apply to whichever display is producing now).
    connect(ui->BRIGHTNESS_SLIDER, &QSlider::valueChanged, this, [this](int){ update_processing_params(); });
    connect(ui->CONTRAST_SLIDER, &QSlider::valueChanged, this, [this](int){ update_processing_params(); });
    connect(ui->GAMMA_SLIDER, &QSlider::valueChanged, this, [this](int){ update_processing_params(); });
    connect(ui->INFO_CHECK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->CENTER_CHECK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    connect(ui->SELECT_TOOL, &QPushButton::toggled, this, [this](bool){ update_processing_params(); });
    connect(ui->DUAL_DISPLAY_CHK, &QCheckBox::stateChanged, this, [this](int){ update_processing_params(); });
    // MCP slider focus changes (also picks up hasFocus on any focus switch)
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget*){ update_processing_params(); });
    // Pref text field changes
    connect(m_setup_overlay->w()->CUSTOM_3D_DELAY_EDT, &QLineEdit::textChanged, this, [this](const QString&){ update_processing_params(); });
    connect(m_setup_overlay->w()->CUSTOM_3D_GW_EDT, &QLineEdit::textChanged, this, [this](const QString&){ update_processing_params(); });
    connect(m_setup_overlay->w()->CUSTOM_INFO_EDT, &QLineEdit::textChanged, this, [this](const QString&){ update_processing_params(); });
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
    // Sprint 21 polish: legacy display-overlay labels not in prototype v3.
    // Keep the widgets alive (display_pos slots still wire) but hide them by
    // default — re-enable in a future patch if the workflow needs coordinates.
    ui->START_COORD->hide();
    ui->SHAPE_INFO->hide();
    // HIDE_BTN was the legacy left-panel toggle. ui->LEFT is permanently
    // hidden post-S18, so the button is purposeless; click() calls elsewhere
    // still work and just toggle the unused hide_left flag.
    ui->HIDE_BTN->hide();
    connect(ui->SOURCE_DISPLAY, &Display::updated_pos, this,
        [this](int idx, QPoint pos) {
            static QPoint converted_pos(0, 0);
            static Display *disp = ui->SOURCE_DISPLAY;
            converted_pos.setX(pos.x() * m_router.stream(StreamId::S0).latest_raw.cols / disp->width());
            converted_pos.setY(pos.y() * m_router.stream(StreamId::S0).latest_raw.rows / disp->height());
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
    m_setup_overlay->w()->REFRESH_AVAILABLE_PORTS_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
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

        // display_page_changed re-routed to ConsolePanel tab in setup_panels() below
    }

    // ----- Floating panels (Sprint 16.7) -----
    m_optics_panel  = new OpticsPanel(ui->CENTRAL);
    m_tcu_panel     = new TCUPanel(ui->CENTRAL);
    m_console_panel = new ConsolePanel(ui->CENTRAL);

    // OpticsPanel: LENS_STATIC + PTZ_GRP (from LEFT/PARAMS_STATIC/PTZ_PAGE)
    m_optics_panel->setup(ui->LENS_STATIC, ui->PTZ_GRP,
                          ui->IRIS_OPEN_BTN, ui->IRIS_CLOSE_BTN);

    // TCUPanel: TCU_STATIC + laser buttons
    m_tcu_panel->setup(ui->TCU_STATIC, ui->LASER_BTN, ui->FIRE_LASER_BTN);

    // ConsolePanel: pull pages from MISC_DISPLAY stacked widget
    m_console_panel->setup(ui->MISC_DISPLAY);
    // Replace old MISC_OPTION/MISC_RADIO wiring with tab-widget routing.
    // MISC_DISPLAY_GRP and its combos/radios are no longer the driver — tab
    // clicks set alt_display_option directly. Master used combo_index → option
    // 1..7 over [DATA, HIST, PTZ, ALT, ADDON, VID, YOLO]; with PTZ now in the
    // OpticsPanel that index is dropped, giving the map below.
    ui->MISC_DISPLAY_GRP->hide();
    disconnect(m_aux_panel, &AuxPanelManager::display_page_changed, nullptr, nullptr);
    // tab index → master alt_display_option value (preserving HIST==2 so the
    // pipeline histogram check at pipelineprocessor.cpp:610 still fires).
    // YOLO tab dropped — its content moved to IMG PROC MODELS tab.
    static const int tab_to_alt_opt[] = {
        1,  // DATA
        2,  // HIST
        4,  // ALT  (master combo_index 3 → option 4)
        5,  // ADDON
        6,  // VID
        0,  // 3D placeholder — no aux-panel option
    };
    connect(m_console_panel->tab_widget(), &QTabWidget::currentChanged,
            this, [this](int idx) {
        if (idx < 0 || idx >= (int)(sizeof(tab_to_alt_opt) / sizeof(tab_to_alt_opt[0]))) return;
        // select_display takes combo_index and stores combo_index + 1, so pass
        // (option - 1) here to land at the desired alt_display_option value.
        m_aux_panel->select_display(tab_to_alt_opt[idx] - 1);
    });

    m_optics_panel ->resize(240, 390); m_optics_panel ->move(0, 0);
    m_tcu_panel    ->resize(250, 360); m_tcu_panel    ->move(0, 0);
    m_console_panel->resize(208, 220); m_console_panel->move(0, 0);
    m_optics_panel ->show();
    m_tcu_panel    ->show();
    m_console_panel->show();

    // Wire panel close-button → uncheck corresponding pbar toggle
    {
        auto *ob = m_pbar->findChild<QPushButton*>("PBAR_OPTICS_BTN");
        auto *tb = m_pbar->findChild<QPushButton*>("PBAR_TCU_BTN");
        auto *cb = m_pbar->findChild<QPushButton*>("PBAR_CONSOLE_BTN");
        if (ob) connect(m_optics_panel,  &FloatingPanel::hideRequested, ob, [ob]() { ob->setChecked(false); });
        if (tb) connect(m_tcu_panel,     &FloatingPanel::hideRequested, tb, [tb]() { tb->setChecked(false); });
        if (cb) connect(m_console_panel, &FloatingPanel::hideRequested, cb, [cb]() { cb->setChecked(false); });
    }

    // ----- Right-side widget stack (Sprint 17.5 cutover) -----
    m_right_stack  = new RightStackContainer(ui->CENTRAL);
    m_img_proc     = new ImgProcWidget(m_right_stack);
    m_save_options = new SaveOptionsWidget(m_right_stack);
    m_vision       = new VisionWidget(m_right_stack);

    // Display picker for the GENERAL tab. Items match DisplayId ordering so
    // currentIndex maps 1:1 to the enum value (Main=0, Alt1=1, Alt2=2).
    QComboBox *display_picker = new QComboBox(m_img_proc);
    display_picker->setObjectName("DISPLAY_PICKER");
    display_picker->addItems({"MAIN", "ALT1", "ALT2"});
    display_picker->setCurrentIndex(static_cast<int>(m_picked_display));
    connect(display_picker, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (idx < 0 || idx >= kDisplayCount) return;
        m_picked_display = static_cast<DisplayId>(idx);
        repopulate_general_widgets_from_picked();
    });

    m_img_proc->setup(// MODELS tab — 3 per-display model selectors (from YOLO_PAGE)
                      ui->MAIN_MODEL_LIST, ui->ALT1_MODEL_LIST, ui->ALT2_MODEL_LIST,
                      ui->YOLO_MAIN_LABEL, ui->YOLO_ALT1_LABEL, ui->YOLO_ALT2_LABEL,
                      // GENERAL tab — display picker + per-display toggles
                      display_picker,
                      ui->IMG_ENHANCE_CHECK,   ui->ENHANCE_OPTIONS,
                      ui->FRAME_AVG_CHECK,     ui->FRAME_AVG_OPTIONS,
                      ui->IMG_3D_CHECK,        ui->RANGE_THRESH_EDIT,
                      ui->RESET_3D_BTN,
                      ui->PSEUDOCOLOR_CHK);

    // SaveOptionsWidget slots are (capture ORI, capture RES, record ORI, record RES).
    // The action handlers (on_SAVE_*_clicked) define the real semantic:
    //   SAVE_BMP   → capture ORI    SAVE_RESULT → capture RES
    //   SAVE_AVI   → record  ORI    SAVE_FINAL  → record  RES
    // Earlier wiring swapped the two RES buttons, putting the capture-RES
    // toggle in the record row and vice versa.
    m_save_options->setup(ui->FILE_PATH_EDIT, ui->FILE_PATH_BROWSE,
                          ui->SAVE_BMP_BUTTON,    ui->SAVE_RESULT_BUTTON,
                          ui->SAVE_AVI_BUTTON,    ui->SAVE_FINAL_BUTTON,
                          ui->CAPTURE_LABEL,      ui->RECORD_LABEL);

    m_right_stack->addSection(m_img_proc);
    m_right_stack->addSection(m_save_options);
    // Vision widget dropped from the right stack — detection results are
    // already drawn as overlays on the displays themselves (draw_yolo_boxes
    // at pipelineprocessor.cpp:606), and the Side Display already mirrors
    // MAIN. VisionWidget kept as code under construction for future use
    // (per-channel inference toggle / threshold / class filter).
    m_vision->hide();
    m_right_stack->contentLayout()->addStretch(1);

    // Hide the legacy RIGHT column — its surviving children (SCAN_GRP) are
    // hidden along with it. SCAN_GRP will be absorbed into the SCAN overlay
    // in Sprint 19; the orphaned IMG_PROC_STATIC / IMG_SAVE_STATIC frames
    // are now empty and stay hidden.
    ui->RIGHT->hide();

    // ----- Side display + map placeholder (Sprint 17.6 / 17.7) -----
    // Side display: live ALT1 / ALT2 thumbnails (same pattern as SETUP / SCAN
    // preview sidebars). Mapbox stays a placeholder until the external
    // library lands.
    m_side_display = new SideDisplayWidget(ui->CENTRAL);
    m_mapbox       = new MapboxWidget(ui->CENTRAL);
    m_side_display->setup(m_router.display(DisplayId::Alt1).widget, m_router.display(DisplayId::Alt2).widget);
    // ALT 1/2 thumbnail "⇆" buttons swap their stream with the main display.
    // alt_idx == 1 → swap Main with Alt1; idx == 2 → swap Main with Alt2.
    connect(m_side_display, &SideDisplayWidget::swap_requested,
            this, [this](int alt_idx) {
        if (alt_idx == 1 || alt_idx == 2)
            swap_grab_thread_display(0, alt_idx);
    });
    m_mapbox      ->hide();
    m_right_stack ->setGeometry(0, 0, 220, 400);
    m_right_stack ->raise();
    m_right_stack ->show();

    // ----- SETUP overlay extras -----
    // SetupOverlay::setup_form() handles the 5 legacy Preferences groups +
    // the IMG_PROC FlowLayout card. The rest of the prototype's SETUP layout
    // (Presets, Ports, Camera) is built here because the widgets live in
    // ui->LEFT (Ui::UserPanel), not Ui::Preferences.
    //
    // Layout (per prototype):
    //   Row 1: [Presets+Ports stack] | SAVE_LOAD_GRP | SERIAL_PORTS_GRP | TCU_GRP
    //   Row 2: DEVICES_GRP | Camera (composed)
    //   Row 3: Image Processing (FlowLayout, handled by SetupOverlay)
    {
        auto *r0 = m_setup_overlay->rowLayout(0);
        auto *r1 = m_setup_overlay->rowLayout(1);
        auto *r0_parent = m_setup_overlay->contentRow(0);
        auto *r1_parent = m_setup_overlay->contentRow(1);

        auto take = [](QWidget *w, QWidget *new_parent) {
            if (!w) return;
            w->setParent(new_parent);
            w->setGeometry(0, 0, w->width(), w->height());
            w->show();
        };

        // ----- Row 1: Presets + Ports vertical stack (leftmost) -----
        auto *stack = new QWidget(r0_parent);
        stack->setObjectName("PRESETS_PORTS_STACK");
        auto *sv = new QVBoxLayout(stack);
        sv->setContentsMargins(0, 0, 0, 0);
        sv->setSpacing(8);

        auto *presets = new QGroupBox(tr("Presets"), stack);
        presets->setObjectName("PRESETS_GRP");
        auto *pg = new QGridLayout(presets);
        pg->setContentsMargins(8, 18, 8, 8);
        pg->setSpacing(4);
        auto *export_btn = new QPushButton(tr("Export"), presets);
        auto *import_btn = new QPushButton(tr("Import"), presets);
        export_btn->setObjectName("EXPORT_PREF_BTN");
        import_btn->setObjectName("IMPORT_PREF_BTN");
        pg->addWidget(export_btn, 0, 0);
        pg->addWidget(import_btn, 0, 1);
        connect(export_btn, &QPushButton::clicked, this, &UserPanel::save_config_to_file);
        connect(import_btn, &QPushButton::clicked, this, &UserPanel::prompt_for_config_file);
        sv->addWidget(presets);

        auto *ports = new QGroupBox(tr("Ports"), stack);
        ports->setObjectName("PORTS_GRP");
        auto *pl = new QGridLayout(ports);
        pl->setContentsMargins(8, 18, 8, 8);
        pl->setHorizontalSpacing(8);
        pl->setVerticalSpacing(4);
        auto add_port_row = [&](int row, QLabel *lbl, QLineEdit *edt) {
            take(lbl, ports);
            take(edt, ports);
            pl->addWidget(lbl, row, 0);
            pl->addWidget(edt, row, 1);
        };
        add_port_row(0, ui->TCU_COM,   ui->TCU_COM_EDIT);
        add_port_row(1, ui->RANGE_COM, ui->RANGE_COM_EDIT);
        add_port_row(2, ui->LENS_COM,  ui->LENS_COM_EDIT);
        add_port_row(3, ui->LASER_COM, ui->LASER_COM_EDIT);
        add_port_row(4, ui->PTZ_COM,   ui->PTZ_COM_EDIT);
        sv->addWidget(ports);
        sv->addStretch(1);

        stack->show();
        r0->insertWidget(0, stack);  // leftmost position in Row 1

        // ----- Row 2: Camera (composed) — replaces PARAMS_STATIC + IMG_GRAB_STATIC -----
        // Prototype layout:
        //   [Enumerate] [Turn On] [Turn Off] | [⛶] [⚄]
        //   ○ Continuous   CMOS Freq [N] Hz   Time Expo [N] ms   [START GRABBING]
        //   ○ Trigger      [Get Params]       Gain [N] ━●━━━━     [STOP GRABBING]
        auto *camera = new QGroupBox(tr("Camera"), r1_parent);
        camera->setObjectName("CAMERA_GRP");
        auto *cv = new QVBoxLayout(camera);
        cv->setContentsMargins(8, 18, 8, 8);
        cv->setSpacing(4);

        // Top button row
        auto *btn_row = new QHBoxLayout;
        btn_row->setSpacing(4);
        take(ui->ENUM_BUTTON,     camera);
        take(ui->START_BUTTON,    camera);
        take(ui->SHUTDOWN_BUTTON, camera);
        take(ui->IMG_REGION_BTN,  camera);
        take(ui->SENSOR_TAPS_BTN, camera);
        btn_row->addWidget(ui->ENUM_BUTTON);
        btn_row->addWidget(ui->START_BUTTON);
        btn_row->addWidget(ui->SHUTDOWN_BUTTON);
        btn_row->addSpacing(8);
        btn_row->addWidget(ui->IMG_REGION_BTN);
        btn_row->addWidget(ui->SENSOR_TAPS_BTN);
        btn_row->addStretch(1);
        cv->addLayout(btn_row);

        // Acquisition grid (2 rows × 4 columns)
        auto *acq = new QGridLayout;
        acq->setHorizontalSpacing(6);
        acq->setVerticalSpacing(4);

        // CMOS Freq compound block: label + edit + unit
        take(ui->CCD_FREQ,      camera);
        take(ui->CCD_FREQ_EDIT, camera);
        take(ui->CCD_FREQ_UNIT, camera);
        ui->CCD_FREQ_EDIT->setMaximumWidth(60);
        auto *freq_block = new QWidget(camera);
        auto *fl = new QHBoxLayout(freq_block);
        fl->setContentsMargins(0, 0, 0, 0);
        fl->setSpacing(2);
        fl->addWidget(ui->CCD_FREQ);
        fl->addWidget(ui->CCD_FREQ_EDIT);
        fl->addWidget(ui->CCD_FREQ_UNIT);
        fl->addStretch(1);

        // Time Expo compound block: label + edit + unit
        take(ui->DUTY,      camera);
        take(ui->DUTY_EDIT, camera);
        take(ui->DUTY_UNIT, camera);
        ui->DUTY_EDIT->setMaximumWidth(60);
        auto *expo_block = new QWidget(camera);
        auto *el = new QHBoxLayout(expo_block);
        el->setContentsMargins(0, 0, 0, 0);
        el->setSpacing(2);
        el->addWidget(ui->DUTY);
        el->addWidget(ui->DUTY_EDIT);
        el->addWidget(ui->DUTY_UNIT);
        el->addStretch(1);

        // Gain compound block: label + edit + slider
        take(ui->GAIN_STATIC, camera);
        take(ui->GAIN_EDIT,   camera);
        take(ui->GAIN_SLIDER, camera);
        ui->GAIN_EDIT->setMaximumWidth(60);
        auto *gain_block = new QWidget(camera);
        auto *gl = new QHBoxLayout(gain_block);
        gl->setContentsMargins(0, 0, 0, 0);
        gl->setSpacing(4);
        gl->addWidget(ui->GAIN_STATIC);
        gl->addWidget(ui->GAIN_EDIT);
        gl->addWidget(ui->GAIN_SLIDER, 1);

        // Remaining standalone widgets
        take(ui->CONTINUOUS_RADIO,      camera);
        take(ui->TRIGGER_RADIO,         camera);
        take(ui->GET_PARAMS_BUTTON,     camera);
        take(ui->START_GRABBING_BUTTON, camera);
        take(ui->STOP_GRABBING_BUTTON,  camera);

        acq->addWidget(ui->CONTINUOUS_RADIO,        0, 0);
        acq->addWidget(freq_block,                  0, 1);
        acq->addWidget(expo_block,                  0, 2);
        acq->addWidget(ui->START_GRABBING_BUTTON,   0, 3);
        acq->addWidget(ui->TRIGGER_RADIO,           1, 0);
        acq->addWidget(ui->GET_PARAMS_BUTTON,       1, 1);
        acq->addWidget(gain_block,                  1, 2);
        acq->addWidget(ui->STOP_GRABBING_BUTTON,    1, 3);
        cv->addLayout(acq);
        cv->addStretch(1);

        camera->show();
        r1->addWidget(camera);

        r0->addStretch(1);
        r1->addStretch(1);
        // Row 2 (index 2) gets its full-width Image Processing card from
        // SetupOverlay::setup_form(); no extra stretch needed here.
    }
    m_setup_overlay->hide();  // shown only in SETUP mode

    // ----- SCAN overlay (Sprint 19, rewritten S22) -----
    // Builds 4 titled group cards (Delay / Gate Width / PTZ / Capture+Record)
    // plus a vertical action column. Widgets are hand-created (was previously
    // re-parented from the legacy ScanConfig dialog; that class is gone in S22)
    // and bound to ScanOverlay, which owns the widget→ScanController::params()
    // sync. Matches the prototype.html v3 layout.
    m_scan_overlay = new ScanOverlay(ui->CENTRAL);
    {
        auto *cards  = m_scan_overlay->cardsLayout();
        auto *cw     = m_scan_overlay->contentWidget();

        // Helpers — small widget factories with the alignment/style the
        // legacy ScanConfig .ui used to set.
        auto le = [](QWidget *parent, const QString &init = QString()) -> QLineEdit* {
            auto *e = new QLineEdit(parent);
            e->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
            e->setFixedWidth(36);
            if (!init.isEmpty()) e->setText(init);
            return e;
        };
        auto lbl = [](QWidget *parent, const QString &text) {
            return new QLabel(text, parent);
        };

        // Creates a titled group card, registers it with cardsLayout(), and
        // returns the body QWidget ready to receive a layout.
        auto make_card = [&](const char *title) -> QWidget* {
            auto *card = new QFrame(cw);
            card->setObjectName("SCAN_CARD");
            card->setProperty("scan_card_title", title);  // for diagnostics
            card->setFrameShape(QFrame::StyledPanel);
            auto *vl = new QVBoxLayout(card);
            vl->setContentsMargins(0, 0, 0, 0);
            vl->setSpacing(0);
            auto *ttl = new QWidget(card);
            ttl->setObjectName("SCAN_CARD_TTL");
            ttl->setAttribute(Qt::WA_StyledBackground, true);
            auto *tl = new QHBoxLayout(ttl);
            tl->setContentsMargins(8, 3, 8, 3);
            tl->addWidget(new QLabel(title, ttl));
            vl->addWidget(ttl);
            auto *body = new QWidget(card);
            body->setObjectName("SCAN_CARD_BODY");
            body->setAttribute(Qt::WA_StyledBackground, true);
            auto *bl = new QVBoxLayout(body);
            bl->setContentsMargins(8, 6, 8, 8);
            bl->setSpacing(4);
            vl->addWidget(body, 1);
            // Top-align so cards size to their content height instead of
            // stretching to fill the overlay's vertical viewport.
            cards->addWidget(card, 0, Qt::AlignTop);
            return body;
        };

        ScanOverlay::Widgets sw;

        // ── Delay ──────────────────────────────────────────────────────────
        {
            auto *body = make_card("DELAY");
            auto *bl   = static_cast<QVBoxLayout*>(body->layout());

            sw.start_delay_u = le(body); sw.start_delay_n = le(body); sw.start_delay_m = le(body);
            sw.end_delay_u   = le(body); sw.end_delay_n   = le(body); sw.end_delay_m   = le(body);
            sw.unit_us_1 = lbl(body, "\xCE\xBCs"); sw.unit_ns_1 = lbl(body, "ns"); sw.unit_m_1 = lbl(body, "m");
            sw.unit_us_2 = lbl(body, "\xCE\xBCs"); sw.unit_ns_2 = lbl(body, "ns"); sw.unit_m_2 = lbl(body, "m");

            auto *grid = new QGridLayout;
            grid->setHorizontalSpacing(4); grid->setVerticalSpacing(4);
            grid->setContentsMargins(0, 0, 0, 0);
            grid->addWidget(lbl(body, "Starting"), 0, 0);
            grid->addWidget(sw.start_delay_u, 0, 1); grid->addWidget(sw.unit_us_1, 0, 2);
            grid->addWidget(sw.start_delay_n, 0, 3); grid->addWidget(sw.unit_ns_1, 0, 4);
            grid->addWidget(sw.start_delay_m, 0, 5); grid->addWidget(sw.unit_m_1,  0, 6);
            grid->addWidget(lbl(body, "Ending"),   1, 0);
            grid->addWidget(sw.end_delay_u, 1, 1); grid->addWidget(sw.unit_us_2, 1, 2);
            grid->addWidget(sw.end_delay_n, 1, 3); grid->addWidget(sw.unit_ns_2, 1, 4);
            grid->addWidget(sw.end_delay_m, 1, 5); grid->addWidget(sw.unit_m_2,  1, 6);
            bl->addLayout(grid);

            auto *sep = new QFrame(body);
            sep->setObjectName("SCAN_CARD_SEP");
            sep->setAttribute(Qt::WA_StyledBackground, true);
            sep->setFixedHeight(1);
            bl->addWidget(sep);

            sw.step_delay = le(body); sw.unit_1 = lbl(body, "ns");
            sw.rep_freq = le(body); sw.freq_unit = lbl(body, "kHz");
            auto *grid2 = new QGridLayout;
            grid2->setHorizontalSpacing(4); grid2->setVerticalSpacing(4);
            grid2->setContentsMargins(0, 0, 0, 0);
            grid2->addWidget(lbl(body, "Step"),     0, 0);
            grid2->addWidget(sw.step_delay,         0, 1);
            grid2->addWidget(sw.unit_1,             0, 2);
            grid2->addWidget(lbl(body, "Rep Freq"), 1, 0);
            grid2->addWidget(sw.rep_freq,           1, 1);
            grid2->addWidget(sw.freq_unit,          1, 2);
            bl->addLayout(grid2);
            bl->addStretch(1);
        }

        // ── Gate Width ─────────────────────────────────────────────────────
        {
            auto *body = make_card("GATE WIDTH");
            auto *bl   = static_cast<QVBoxLayout*>(body->layout());

            sw.start_gw_u = le(body); sw.start_gw_n = le(body); sw.start_gw_m = le(body);
            sw.end_gw_u   = le(body); sw.end_gw_n   = le(body); sw.end_gw_m   = le(body);
            sw.unit_us_3 = lbl(body, "\xCE\xBCs"); sw.unit_ns_3 = lbl(body, "ns"); sw.unit_m_3 = lbl(body, "m");
            sw.unit_us_4 = lbl(body, "\xCE\xBCs"); sw.unit_ns_4 = lbl(body, "ns"); sw.unit_m_4 = lbl(body, "m");

            auto *grid = new QGridLayout;
            grid->setHorizontalSpacing(4); grid->setVerticalSpacing(4);
            grid->setContentsMargins(0, 0, 0, 0);
            grid->addWidget(lbl(body, "Starting"), 0, 0);
            grid->addWidget(sw.start_gw_u, 0, 1); grid->addWidget(sw.unit_us_3, 0, 2);
            grid->addWidget(sw.start_gw_n, 0, 3); grid->addWidget(sw.unit_ns_3, 0, 4);
            grid->addWidget(sw.start_gw_m, 0, 5); grid->addWidget(sw.unit_m_3,  0, 6);
            grid->addWidget(lbl(body, "Ending"),   1, 0);
            grid->addWidget(sw.end_gw_u, 1, 1); grid->addWidget(sw.unit_us_4, 1, 2);
            grid->addWidget(sw.end_gw_n, 1, 3); grid->addWidget(sw.unit_ns_4, 1, 4);
            grid->addWidget(sw.end_gw_m, 1, 5); grid->addWidget(sw.unit_m_4,  1, 6);
            bl->addLayout(grid);

            auto *sep = new QFrame(body);
            sep->setObjectName("SCAN_CARD_SEP");
            sep->setAttribute(Qt::WA_StyledBackground, true);
            sep->setFixedHeight(1);
            bl->addWidget(sep);

            sw.step_gw = le(body); sw.unit_2 = lbl(body, "ns");
            auto *grid2 = new QGridLayout;
            grid2->setHorizontalSpacing(4); grid2->setVerticalSpacing(4);
            grid2->setContentsMargins(0, 0, 0, 0);
            grid2->addWidget(lbl(body, "Step"), 0, 0);
            grid2->addWidget(sw.step_gw,        0, 1);
            grid2->addWidget(sw.unit_2,         0, 2);
            bl->addLayout(grid2);
            bl->addStretch(1);
        }

        // ── PTZ ────────────────────────────────────────────────────────────
        {
            auto *body = make_card("PTZ");
            auto *bl   = static_cast<QVBoxLayout*>(body->layout());

            auto *row_w = new QWidget(body);
            auto *hl    = new QHBoxLayout(row_w);
            hl->setContentsMargins(0, 0, 0, 0);
            hl->setSpacing(12);

            auto *gw_w = new QWidget(row_w);
            auto *g    = new QGridLayout(gw_w);
            g->setHorizontalSpacing(4); g->setVerticalSpacing(4);
            g->setContentsMargins(0, 0, 0, 0);
            g->addWidget(lbl(gw_w, "H"), 0, 1, Qt::AlignCenter);
            g->addWidget(lbl(gw_w, "V"), 0, 2, Qt::AlignCenter);
            sw.start_h = le(gw_w); sw.start_v = le(gw_w);
            sw.end_h   = le(gw_w); sw.end_v   = le(gw_w);
            sw.step_h  = le(gw_w); sw.step_v  = le(gw_w);
            sw.frame_h = le(gw_w); sw.frame_v = le(gw_w);
            struct { const char *lbl; QLineEdit *h; QLineEdit *v; } rows[] = {
                { "Starting",  sw.start_h, sw.start_v },
                { "Ending",    sw.end_h,   sw.end_v   },
                { "Step",      sw.step_h,  sw.step_v  },
                { "Pos count", sw.frame_h, sw.frame_v },
            };
            for (int r = 0; r < 4; ++r) {
                g->addWidget(lbl(gw_w, rows[r].lbl), r + 1, 0);
                g->addWidget(rows[r].h,              r + 1, 1);
                g->addWidget(rows[r].v,              r + 1, 2);
            }
            for (int r = 0; r < 3; ++r)
                g->addWidget(lbl(gw_w, "\xc2\xb0"), r + 1, 3);
            hl->addWidget(gw_w);

            auto *rw  = new QWidget(row_w);
            auto *rvl = new QVBoxLayout(rw);
            rvl->setContentsMargins(0, 0, 0, 0); rvl->setSpacing(8);
            rvl->addWidget(lbl(rw, "Direction"));
            auto *dir_row = new QWidget(rw);
            auto *drl = new QHBoxLayout(dir_row);
            drl->setContentsMargins(0, 0, 0, 0); drl->setSpacing(6);
            sw.dir_n = new QRadioButton("N", dir_row);
            sw.dir_z = new QRadioButton("Z", dir_row);
            drl->addWidget(sw.dir_n);
            drl->addWidget(sw.dir_z);
            rvl->addWidget(dir_row);
            rvl->addWidget(lbl(rw, "Wait"));
            auto *wait_row = new QWidget(rw);
            auto *wrl = new QHBoxLayout(wait_row);
            wrl->setContentsMargins(0, 0, 0, 0); wrl->setSpacing(4);
            sw.ptz_wait = le(wait_row);
            wrl->addWidget(sw.ptz_wait);
            wrl->addWidget(lbl(wait_row, "s"));
            rvl->addWidget(wait_row);
            rvl->addStretch(1);
            hl->addWidget(rw);

            bl->addWidget(row_w);
            bl->addStretch(1);
        }

        // ── Capture / Record ───────────────────────────────────────────────
        {
            auto *body = make_card("CAPTURE / RECORD");
            auto *bl   = static_cast<QVBoxLayout*>(body->layout());

            auto *row_w = new QWidget(body);
            auto *hl    = new QHBoxLayout(row_w);
            hl->setContentsMargins(0, 0, 0, 0); hl->setSpacing(16);

            auto *lw  = new QWidget(row_w);
            auto *lvl = new QVBoxLayout(lw);
            lvl->setContentsMargins(0, 0, 0, 0); lvl->setSpacing(8);
            sw.capture_ori = new QCheckBox("original", lw);
            sw.capture_res = new QCheckBox("result",   lw);
            sw.record_ori  = new QCheckBox("original", lw);
            sw.record_res  = new QCheckBox("result",   lw);
            lvl->addWidget(lbl(lw, "CAPTURE"));
            lvl->addWidget(sw.capture_ori);
            lvl->addWidget(sw.capture_res);
            lvl->addWidget(lbl(lw, "RECORD"));
            lvl->addWidget(sw.record_ori);
            lvl->addWidget(sw.record_res);
            lvl->addStretch(1);
            hl->addWidget(lw);

            auto *rw  = new QWidget(row_w);
            auto *rvl = new QVBoxLayout(rw);
            rvl->setContentsMargins(0, 0, 0, 0); rvl->setSpacing(6);
            sw.frame_count    = le(rw);
            sw.num_single_pos = le(rw);

            // Each description is a self-sized 2-row container:
            //   [first part of label                ]
            //   [last part            ][input]
            // Container is sized to its content; both containers are then
            // equalized to the wider of the two so the two descriptions line
            // up consistently.
            auto desc_row = [&rw](const QString &first_part, const QString &last_part, QLineEdit *edit) -> QWidget* {
                auto *c = new QWidget(rw);
                c->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
                auto *vbox = new QVBoxLayout(c);
                vbox->setContentsMargins(0, 0, 0, 0);
                vbox->setSpacing(0);
                vbox->addWidget(new QLabel(first_part, c));
                auto *r2 = new QHBoxLayout;
                r2->setContentsMargins(0, 0, 0, 0);
                r2->setSpacing(4);
                r2->addWidget(new QLabel(last_part, c));
                r2->addStretch(1);
                r2->addWidget(edit);
                vbox->addLayout(r2);
                return c;
            };

            // More balanced splits (smaller first line, more on the second).
            auto *d1 = desc_row("# of TCU+PTZ", "param sets", sw.frame_count);
            auto *d2 = desc_row("# of imgs",    "per profile", sw.num_single_pos);
            const int unified_w = qMax(d1->sizeHint().width(), d2->sizeHint().width());
            d1->setFixedWidth(unified_w);
            d2->setFixedWidth(unified_w);

            rvl->addWidget(lbl(rw, "PROFILES"));
            rvl->addWidget(d1);
            rvl->addWidget(lbl(rw, "IMGS / PROFILE"));
            rvl->addWidget(d2);
            rvl->addStretch(1);
            hl->addWidget(rw);

            bl->addWidget(row_w);
            bl->addStretch(1);
        }

        // ── Action column (90 px) ──────────────────────────────────────────
        {
            auto *ac  = new QWidget(cw);
            ac->setObjectName("SCAN_ACTION_COL");
            ac->setFixedWidth(90);
            auto *vl  = new QVBoxLayout(ac);
            vl->setContentsMargins(4, 4, 4, 4); vl->setSpacing(6);
            ui->SCAN_BUTTON->setParent(ac); ui->SCAN_BUTTON->show();
            vl->addWidget(ui->SCAN_BUTTON);
            // Re-parent but don't force visible — ScanController owns visibility.
            ui->CONTINUE_SCAN_BUTTON->setParent(ac);
            vl->addWidget(ui->CONTINUE_SCAN_BUTTON);
            ui->RESTART_SCAN_BUTTON->setParent(ac);
            vl->addWidget(ui->RESTART_SCAN_BUTTON);
            vl->addStretch(1);
            cards->addWidget(ac);
        }

        cards->addStretch(1);

        // Bind ScanController so widget edits update ScanController::params().
        // Note: m_scan_ctrl is created further down in init(); bind is called
        // there instead. We capture sw for that later call.
        m_scan_overlay_widgets = sw;
    }
    m_scan_overlay->hide();  // shown only in SCAN mode

    // ----- Preview sidebars (Sprint 20.4) -----
    // 3 thumbnail QLabels per overlay, mirroring displays[0..2]. Each Display
    // already emits set_pixmap when the pipeline pushes a new frame; thumbnails
    // listen and scale-to-fit. No pipeline changes needed.
    for (int i = 0; i < 3; ++i) { m_setup_thumb[i] = nullptr; m_scan_thumb[i] = nullptr; }
    auto populate_thumbs = [this](QVBoxLayout *layout, QLabel *thumbs[3]) {
        if (!layout) return;
        // Clear placeholder + stretch from the constructor.
        while (auto *item = layout->takeAt(0)) {
            if (auto *w = item->widget()) w->deleteLater();
            delete item;
        }
        QWidget *parent_w = layout->parentWidget();
        static const char *names[3] = { "MAIN", "ALT 1", "ALT 2" };
        for (int i = 0; i < 3; ++i) {
            auto *lbl = new QLabel(names[i], parent_w);
            lbl->setObjectName("PREVIEW_THUMB_LBL");
            lbl->setStyleSheet("font-weight: 700; color: #E0924A; background: transparent;");
            layout->addWidget(lbl);
            thumbs[i] = new QLabel(parent_w);
            thumbs[i]->setObjectName("PREVIEW_THUMB");
            thumbs[i]->setFixedHeight(150);
            thumbs[i]->setMinimumWidth(180);
            thumbs[i]->setAlignment(Qt::AlignCenter);
            thumbs[i]->setStyleSheet("background-color: #040303; border: 1px solid #382D25;");
            layout->addWidget(thumbs[i]);
        }
        layout->addStretch(1);
    };
    if (m_setup_overlay) populate_thumbs(m_setup_overlay->previewSidebarLayout(), m_setup_thumb);
    if (m_scan_overlay)  populate_thumbs(m_scan_overlay ->previewLayout(),        m_scan_thumb);

    // Wire each thumbnail to its source Display's set_pixmap signal. The
    // signal fires queued from the GrabThread; we scale-to-fit on the GUI
    // thread. When the source Display is empty (no camera), thumbs stay dark.
    for (int i = 0; i < 3; ++i) {
        if (!m_router.display(static_cast<DisplayId>(i)).widget) continue;
        auto scaler = [](QLabel *thumb) {
            return [thumb](const QPixmap &pm) {
                if (!thumb || pm.isNull()) return;
                thumb->setPixmap(pm.scaled(thumb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            };
        };
        if (m_setup_thumb[i])
            connect(m_router.display(static_cast<DisplayId>(i)).widget, &Display::set_pixmap, m_setup_thumb[i], scaler(m_setup_thumb[i]), Qt::QueuedConnection);
        if (m_scan_thumb[i])
            connect(m_router.display(static_cast<DisplayId>(i)).widget, &Display::set_pixmap, m_scan_thumb[i], scaler(m_scan_thumb[i]), Qt::QueuedConnection);
    }

    // Mode-tab wiring: overlays are mutually exclusive; MONITOR hides both.
    connect(ui->TITLE, &TopBar::modeChanged, this, [this](TopBar::Mode mode) {
        const bool setup_mode   = (mode == TopBar::Mode::Setup);
        const bool scan_mode    = (mode == TopBar::Mode::Scan);
        const bool overlay_mode = setup_mode || scan_mode;

        // Leaving Setup: read pending UI edits into Config and persist to default.json
        // so values entered in the setup panel survive next launch (matches user
        // expectation that "modified values are updated to the default.json").
        const bool was_setup_mode = m_setup_overlay && m_setup_overlay->isVisible();
        if (was_setup_mode && !setup_mode) {
            syncPreferencesToConfig();
            if (config) config->auto_save();
        }

        if (m_setup_overlay) m_setup_overlay->setVisible(setup_mode);
        if (m_scan_overlay)  m_scan_overlay ->setVisible(scan_mode);
        if (m_right_stack)   m_right_stack  ->setVisible(!overlay_mode);
        if (m_pbar)          m_pbar         ->setVisible(!overlay_mode);
        position_display_overlays(DisplayId::Main);
        if (m_router.display(DisplayId::Main).fps_label) m_router.display(DisplayId::Main).fps_label->setVisible(!overlay_mode);
        // Source-death label tracks the actual queue state (DisplaySlot::source_dead),
        // but stays hidden whenever an overlay is up.
        if (QLabel* sd = m_router.display(DisplayId::Main).source_dead_label) {
            sd->setVisible(!overlay_mode && m_router.display(DisplayId::Main).source_dead);
            if (sd->isVisible()) sd->raise();
        }
        // Alt FPS labels (1/2) live inside FloatingWindows — visibility tracks parent.
        // Floating panels follow their pbar toggle state when not in an overlay mode.
        auto pbar_checked = [this](const char *name) -> bool {
            if (!m_pbar) return true;
            auto *b = m_pbar->findChild<QPushButton*>(name);
            return b ? b->isChecked() : true;
        };
        if (m_optics_panel)  m_optics_panel ->setVisible(!overlay_mode && pbar_checked("PBAR_OPTICS_BTN"));
        if (m_tcu_panel)     m_tcu_panel    ->setVisible(!overlay_mode && pbar_checked("PBAR_TCU_BTN"));
        if (m_console_panel) m_console_panel->setVisible(!overlay_mode && pbar_checked("PBAR_CONSOLE_BTN"));
        // Panels must sit above side-zone widgets (siblings in CENTRAL's z-order).
        if (!overlay_mode) {
            if (m_optics_panel)  m_optics_panel ->raise();
            if (m_tcu_panel)     m_tcu_panel    ->raise();
            if (m_console_panel) m_console_panel->raise();
        }
        if (setup_mode && m_setup_overlay) {
            m_setup_overlay->raise();
            const QRect c = ui->CENTRAL->rect();
            m_setup_overlay->setGeometry(c.x(), 40, c.width(), c.height() - 40);
        }
        if (scan_mode && m_scan_overlay) {
            m_scan_overlay->raise();
            const QRect c = ui->CENTRAL->rect();
            m_scan_overlay->setGeometry(c.x(), 40, c.width(), c.height() - 40);
        }
    });

    // ui->LEFT permanent hide: PARAMS_STATIC + IMG_GRAB_STATIC migrated above;
    // remaining LASER_STATIC widgets are already in TCUPanel (16.7) or unused.
    // Reparent DISTANCE label out of LASER_STATIC before LEFT goes hidden —
    // it lives at the top-right of SOURCE_DISPLAY (positioned in resizeEvent)
    // and reads TCUController::distance_text_updated (wired at line 1423).
    ui->DISTANCE->setParent(ui->CENTRAL);
    ui->DISTANCE->setGeometry(QRect());
    ui->DISTANCE->show();
    ui->DISTANCE->raise();
    ui->LEFT->hide();
    // SideDisplay + Mapbox can now occupy the freed left zone.
    if (m_side_display) m_side_display->show();
    if (m_mapbox)       m_mapbox      ->show();
    // All four side widgets are now CENTRAL children; raise floating panels so
    // they remain above the side-zone widgets in CENTRAL's z-order.
    if (m_optics_panel)  m_optics_panel ->raise();
    if (m_tcu_panel)     m_tcu_panel    ->raise();
    if (m_console_panel) m_console_panel->raise();

    ui->DATA_EXCHANGE->document()->setMaximumBlockCount(200);
    ui->DATA_EXCHANGE->setFont(temp_f);
    ui->FILE_PATH_EDIT->setFont(consolas);

    m_device_mgr = new DeviceManager(config, this);
    m_device_mgr->init();

    // Create TCUController
    m_tcu_ctrl = new TCUController(config, m_device_mgr, this);
    m_tcu_ctrl->init(aliasing);

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
    m_scan_ctrl->init(&save_location, &m_router.stream(StreamId::S0).meta.width, &m_router.stream(StreamId::S0).meta.height);

    // S22: bind the SCAN overlay widgets to ScanController now that the
    // controller exists. The widgets were created in the scan-overlay block
    // earlier in init() and cached on m_scan_overlay_widgets.
    if (m_scan_overlay) m_scan_overlay->bind(m_scan_ctrl, m_scan_overlay_widgets);

    // --- Wire controller UI signals ---
    // DeviceManager → UI
    connect(m_device_mgr, &DeviceManager::port_label_style_changed, this, [this](int idx, QString style) {
        QLabel *labels[] = { ui->TCU_COM, ui->LENS_COM, ui->LASER_COM, ui->PTZ_COM, ui->RANGE_COM };
        if (idx >= 0 && idx < 5) labels[idx]->setStyleSheet(style);
    });
    connect(m_device_mgr, &DeviceManager::angle_display_updated, this, [this](QString h, QString v) {
        // Skip when the field has focus — device status frames arrive every
        // few ms and would clobber the user's keystrokes mid-edit.
        if (!ui->ANGLE_H_EDIT->hasFocus()) ui->ANGLE_H_EDIT->setText(h);
        if (!ui->ANGLE_V_EDIT->hasFocus()) ui->ANGLE_V_EDIT->setText(v);
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
        // Restore the original "+" / "-" glyphs after a press shows "x". The
        // previous version cleared to "" which left the buttons blank for the
        // rest of the session.
        ui->VL_ZOOM_IN_BTN   ->setText("+"); ui->VL_ZOOM_OUT_BTN  ->setText("-");
        ui->VL_FOCUS_NEAR_BTN->setText("+"); ui->VL_FOCUS_FAR_BTN ->setText("-");
        ui->IR_ZOOM_IN_BTN   ->setText("+"); ui->IR_ZOOM_OUT_BTN  ->setText("-");
        ui->IR_FOCUS_NEAR_BTN->setText("+"); ui->IR_FOCUS_FAR_BTN ->setText("-");
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
        m_setup_overlay->set_ptz_type_enabled(enabled);
    });
    connect(m_device_mgr, &DeviceManager::baudrate_display_requested, this, [this](int idx, int baud) {
        m_setup_overlay->display_baudrate(idx, baud);
    });
    connect(m_device_mgr, &DeviceManager::tcp_server_chk_update, this, [this](bool enabled, bool checked) {
        m_setup_overlay->w()->TCP_SERVER_CHK->setEnabled(enabled);
        m_setup_overlay->w()->TCP_SERVER_CHK->setChecked(checked);
    });
    connect(m_device_mgr, &DeviceManager::tcp_server_connect_failed, this, [this]() {
        m_setup_overlay->w()->TCP_SERVER_CHK->setEnabled(true);
        m_setup_overlay->w()->TCP_SERVER_CHK->setChecked(false);
    });
    connect(m_device_mgr, &DeviceManager::get_lens_param_requested, m_lens_ctrl, &LensController::on_GET_LENS_PARAM_BTN_clicked);

    // Lens relay on/off (next to Iris in Classic optics box). Checked = relay
    // ON, label flips to 'x'; unchecked = OFF, label 'o'.
    connect(ui->LENS_ON_BTN, &QPushButton::toggled, this, [this](bool on) {
        ui->LENS_ON_BTN->setText(on ? "x" : "o");
        m_lens_ctrl->on_LENS_ON_BTN_toggled(on);
    });

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
        connect(ui->STOP_BTN, &QPushButton::clicked, m_device_mgr, &DeviceManager::ptz_stop_btn_clicked);

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
    connect(m_tcu_ctrl, &TCUController::tcu_type_layout_changed, this, [this](int idx, int diff, int laser_diff) {
        // Restored from pre-Sprint-9 set_tcu_type — type switch must reflow
        // U/N/P labels and shift edits, not just toggle _P visibility.
        if (laser_diff)
            update_tcu_param_pos(laser_diff, ui->LASER_WIDTH_UNIT_U, ui->LASER_WIDTH_EDIT_N, ui->LASER_WIDTH_UNIT_N, ui->LASER_WIDTH_EDIT_P);
        if (diff) {
            ui->DELAY_A->resize(ui->DELAY_A->width() + 12 * diff, ui->DELAY_A->height());
            ui->DELAY_B->resize(ui->DELAY_B->width() + 12 * diff, ui->DELAY_B->height());
            ui->GATE_WIDTH_A->resize(ui->GATE_WIDTH_A->width() + 12 * diff, ui->GATE_WIDTH_A->height());
            ui->GATE_WIDTH_B->resize(ui->GATE_WIDTH_B->width() + 12 * diff, ui->GATE_WIDTH_B->height());
            update_tcu_param_pos(diff, ui->DELAY_A_UNIT_U, ui->DELAY_A_EDIT_N, ui->DELAY_A_UNIT_N, ui->DELAY_A_EDIT_P);
            update_tcu_param_pos(diff, ui->DELAY_B_UNIT_U, ui->DELAY_B_EDIT_N, ui->DELAY_B_UNIT_N, ui->DELAY_B_EDIT_P);
            update_tcu_param_pos(diff, ui->GATE_WIDTH_A_UNIT_U, ui->GATE_WIDTH_A_EDIT_N, ui->GATE_WIDTH_A_UNIT_N, ui->GATE_WIDTH_A_EDIT_P);
            update_tcu_param_pos(diff, ui->GATE_WIDTH_B_UNIT_U, ui->GATE_WIDTH_B_EDIT_N, ui->GATE_WIDTH_B_UNIT_N, ui->GATE_WIDTH_B_EDIT_P);
        }
        ui->DELAY_GRP->move(idx ? -6 : 0, 100);
        ui->GATE_WIDTH_GRP->move(idx ? -6 : 0, 180);
    });
    connect(m_tcu_ctrl, &TCUController::tcu_diff_view_toggled, this, [this](bool show) {
        ui->DELAY_B_GRP->setVisible(!show);
        ui->GATE_WIDTH_B_GRP->setVisible(!show);
        ui->DELAY_DIFF_GRP->setVisible(show);
        ui->GATE_WIDTH_DIFF_GRP->setVisible(show);
    });
    connect(m_tcu_ctrl, &TCUController::ps_config_display_updated, this, [this](QString stepping, QString max_step) {
        m_setup_overlay->w()->PS_STEPPING_EDT->setText(stepping);
        m_setup_overlay->w()->MAX_PS_STEP_EDT->setText(max_step);
    });
    connect(m_tcu_ctrl, &TCUController::auto_mcp_chk_click_requested, this, [this]() {
        ui->AUTO_MCP_CHK->click();
    });
    connect(m_tcu_ctrl, &TCUController::fire_laser_btn_click_requested, this, [this]() {
        ui->FIRE_LASER_BTN->click();
    });
    connect(m_tcu_ctrl, &TCUController::laser_chk_click_requested, this, [this]() {
        m_setup_overlay->w()->LASER_ENABLE_CHK->click();
    });
    connect(m_tcu_ctrl, &TCUController::dist_ns_changed, this, [this](float dist_ns) {
        m_setup_overlay->set_dist_ns(dist_ns);
        if (m_scan_ctrl) m_scan_ctrl->params().dist_ns = dist_ns;
    });
    connect(m_tcu_ctrl, &TCUController::update_distance_display_requested, m_setup_overlay, &SetupOverlay::update_distance_display);

    // LensController → UI
    connect(m_lens_ctrl, &LensController::button_text_changed, this, [this](int id, QString text) {
        // IRIS_OPEN_BTN / IRIS_CLOSE_BTN (ids 6,7) removed in S16 — duplicate of radius
        QAbstractButton *btns[] = {
            ui->ZOOM_IN_BTN, ui->ZOOM_OUT_BTN, ui->FOCUS_FAR_BTN, ui->FOCUS_NEAR_BTN,
            ui->RADIUS_INC_BTN, ui->RADIUS_DEC_BTN
        };
        if (id >= 0 && id < 6) btns[id]->setText(text);
    });
    connect(m_lens_ctrl, &LensController::iris_text_updated, this, [this](QString text) {
        if (!ui->IRIS_EDIT->hasFocus()) ui->IRIS_EDIT->setText(text);
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
    // FIRE_LASER_BTN's gating (text + enabled) is wired up in
    // setup_floating_panels(), where the UnlockBar is available.
    connect(m_laser_ctrl, &LaserController::current_edit_enabled, ui->CURRENT_EDIT, &QWidget::setEnabled);
    connect(m_laser_ctrl, &LaserController::simple_laser_chk_update, this, [this](bool enabled, int state) {
        ui->SIMPLE_LASER_CHK->setEnabled(enabled);
        if (state == 0) ui->SIMPLE_LASER_CHK->setChecked(false);
        else if (state == 1) ui->SIMPLE_LASER_CHK->setChecked(true);
        else if (state == 2) ui->SIMPLE_LASER_CHK->click();
    });
    connect(m_laser_ctrl, &LaserController::laser_enable_requested, this, [this](bool enable) {
        // FIRE/STOP must drive both the master "enable laser" command
        // (LASER_ENABLE_CHK → 0x88 0x1F …) and the channel-1 select command
        // (LASER_CHK_1 → toggle_laser → TCU::LASER_ON). Both checkboxes' own
        // stateChanged / buttonToggled handlers send the wire commands; we
        // just set their state here. setChecked is a no-op when the box is
        // already in the requested state, so this is safe to call on every
        // FIRE/STOP transition.
        m_setup_overlay->w()->LASER_ENABLE_CHK->setChecked(enable);
        m_setup_overlay->w()->LASER_CHK_1->setChecked(enable);
    });
    connect(m_laser_ctrl, &LaserController::update_current_requested, this, [this]() {
        update_current();
    });

    // RFController → UI
    connect(m_rf_ctrl, &RFController::distance_text_updated, ui->DISTANCE, &QLabel::setText);

    // ScanController → UI. Side-effect: drive a `scanning` dynamic property on
    // SCAN_BUTTON so QSS can swap the visual state (positive-green idle ↔
    // warning-amber running). Qt requires unpolish + polish to re-apply QSS
    // when a property selector value changes.
    connect(m_scan_ctrl, &ScanController::scan_button_text_changed,
            this, [this](const QString &text) {
        ui->SCAN_BUTTON->setText(text);
        const bool scanning = (text != tr("Scan"));
        if (ui->SCAN_BUTTON->property("scanning").toBool() != scanning) {
            ui->SCAN_BUTTON->setProperty("scanning", scanning);
            ui->SCAN_BUTTON->style()->unpolish(ui->SCAN_BUTTON);
            ui->SCAN_BUTTON->style()->polish(ui->SCAN_BUTTON);
        }
    });
    connect(m_scan_ctrl, &ScanController::scan_options_changed, this, [this](bool cont, bool restart) {
        ui->CONTINUE_SCAN_BUTTON->setVisible(cont);
        ui->RESTART_SCAN_BUTTON->setVisible(restart);
    });

    // ----- Floating panel mode contents (Sprint 16.8) -----

    // OpticsPanel — Modern mode: JoystickWidget + ZOOM + FOCUS ArcSliders
    {
        auto *modern_w = new QWidget;
        auto *ml = new QVBoxLayout(modern_w);
        ml->setContentsMargins(4, 4, 4, 4);
        ml->setSpacing(8);

        // Joystick column (joystick + "PTZ" label below). Centered horizontally
        // when added to ml so it sits at the top of the modern page.
        auto *joy_col = new QWidget(modern_w);
        auto *joy_l = new QVBoxLayout(joy_col);
        joy_l->setContentsMargins(0, 0, 0, 0);
        joy_l->setSpacing(2);
        auto *joystick = new JoystickWidget(joy_col);
        joystick->setFixedSize(120, 120);
        auto *joy_lbl = new QLabel("PTZ", joy_col);
        joy_lbl->setObjectName("JoyLabel");
        joy_lbl->setAlignment(Qt::AlignCenter);
        joy_l->addWidget(joystick, 0, Qt::AlignHCenter);
        joy_l->addWidget(joy_lbl);

        auto *zoom_slider  = new ArcSliderWidget(modern_w);
        auto *focus_slider = new ArcSliderWidget(modern_w);
        auto *iris_slider  = new ArcSliderWidget(modern_w);
        zoom_slider ->setFixedSize(54, 120);
        focus_slider->setFixedSize(54, 120);
        iris_slider ->setFixedSize(54, 120);
        // Speed/joystick mode: drag from midpoint = signed speed (1..63),
        // quantized in steps of 4. Release snaps back to midpoint = STOP.
        zoom_slider ->setSpeedMode(63);
        focus_slider->setSpeedMode(63);
        iris_slider ->setSpeedMode(63);
        // Per-slider colours match prototype v3 lens tokens.
        zoom_slider ->setActiveColor(QColor(0xE6, 0x45, 0x5D)); // lens-zoom  (pink)
        focus_slider->setActiveColor(QColor(0xE0, 0x92, 0x4A)); // lens-focus (orange)
        iris_slider ->setActiveColor(QColor(0x4D, 0xA6, 0xE6)); // lens-iris  (blue)

        // Helper: wrap an ArcSliderWidget in a vertical sub-column
        // [slider][label][value]. Each sub-column sits side-by-side in slider_row.
        auto makeArcCol = [&](ArcSliderWidget *slider, const char *lbl) {
            auto *col = new QWidget(modern_w);
            auto *cl  = new QVBoxLayout(col);
            cl->setContentsMargins(0, 0, 0, 0);
            cl->setSpacing(2);

            auto *name = new QLabel(lbl, col);
            name->setObjectName("ArcLabel");
            name->setAlignment(Qt::AlignCenter);

            auto *val = new QLabel("00", col);
            val->setObjectName("ArcValue");
            val->setAlignment(Qt::AlignCenter);
            // In speed mode the value is signed speed; show absolute speed
            // (the up/down direction is already conveyed by the thumb).
            connect(slider, &ArcSliderWidget::valueChanged, val, [val](int v) {
                val->setText(QString::number(qAbs(v)).rightJustified(2, '0'));
            });

            cl->addWidget(slider, 0, Qt::AlignHCenter);
            cl->addWidget(name);
            cl->addWidget(val);
            cl->addStretch(1);          // keep slider+labels packed at the top
            return col;
        };

        auto *slider_row = new QWidget(modern_w);
        auto *sr_l = new QHBoxLayout(slider_row);
        sr_l->setContentsMargins(0, 0, 0, 0);
        sr_l->setSpacing(5);
        sr_l->addWidget(makeArcCol(zoom_slider,  "ZOOM"));
        sr_l->addWidget(makeArcCol(focus_slider, "FOCUS"));
        sr_l->addWidget(makeArcCol(iris_slider,  "IRIS"));

        ml->addWidget(joy_col,    0, Qt::AlignHCenter);
        ml->addWidget(slider_row, 0, Qt::AlignHCenter);
        ml->addStretch(1);

        // Joystick → PTZ with independent pan/tilt speeds. Each axis's speed
        // is |dx| / |dy| scaled to PTZ range; direction byte is composed
        // from the sign of each axis (no 8-sector snap), so moving into a
        // corner no longer causes a 0→full jump on either speed byte.
        // dy > 0 is DOWN in screen coords.
        // id grid: {0:UL,1:U,2:UR, 3:L,5:R, 6:DL,7:D,8:DR} → DeviceManager
        // adds 1 to map to PTZ::PARAMS enum.
        connect(joystick, &JoystickWidget::directionChanged,
                this, [this](double dx, double dy) {
            static int last_id = -1;
            static int last_h = -1, last_v = -1;

            if (dx == 0.0 && dy == 0.0) {
                if (last_id >= 0) m_device_mgr->ptz_button_released(last_id);
                last_id = -1;
                last_h = last_v = -1;
                return;
            }

            const int h_speed = qBound(0, static_cast<int>(std::round(qMin(std::abs(dx), 1.0) * 64.0)), 64);
            const int v_speed = qBound(0, static_cast<int>(std::round(qMin(std::abs(dy), 1.0) * 64.0)), 64);

            // Per-axis deadzone in speed space — below this the axis is
            // inactive and the direction byte stays cardinal. The widget's
            // own deadzone gates total magnitude; this only governs how
            // close to cardinal the sign-based picker latches.
            const int kAxisDz = 3;   // 3/64 ≈ 4.7%
            const bool h_active = h_speed >= kAxisDz;
            const bool v_active = v_speed >= kAxisDz;
            if (!h_active && !v_active) {
                if (last_id >= 0) m_device_mgr->ptz_button_released(last_id);
                last_id = -1;
                last_h = last_v = -1;
                return;
            }

            int id;
            if (h_active && v_active) {
                if      (dy < 0 && dx < 0) id = 0;  // UP_LEFT
                else if (dy < 0 && dx > 0) id = 2;  // UP_RIGHT
                else if (dy > 0 && dx < 0) id = 6;  // DOWN_LEFT
                else                       id = 8;  // DOWN_RIGHT
            } else if (v_active) {
                id = (dy < 0) ? 1 : 7;              // UP : DOWN
            } else {
                id = (dx < 0) ? 3 : 5;              // LEFT : RIGHT
            }

            if (h_speed != last_h || v_speed != last_v || id != last_id) {
                if (id != last_id && last_id >= 0) {
                    m_device_mgr->ptz_button_released(last_id);
                }
                m_device_mgr->ptz_move_hv(id, h_speed, v_speed);
                last_id = id;
                last_h = h_speed;
                last_v = v_speed;
            }
        });

        connect(zoom_slider,  &ArcSliderWidget::speedChanged, this, [this](int s) {
            m_lens_ctrl->set_modern_speed(LensController::Zoom, s);
        });
        connect(focus_slider, &ArcSliderWidget::speedChanged, this, [this](int s) {
            m_lens_ctrl->set_modern_speed(LensController::Focus, s);
        });
        connect(iris_slider,  &ArcSliderWidget::speedChanged, this, [this](int s) {
            m_lens_ctrl->set_modern_speed(LensController::Iris, s);
        });

        // Mode switch: when leaving modern -> classic, force-release all arcs
        // (covers the edge case where the mode toggle steals focus mid-drag)
        // and re-send classic's FOCUS_SPEED_SLIDER value so the device's
        // STEPPING register matches what classic shows.
        connect(m_optics_panel, &OpticsPanel::modeChanged, this,
                [this, zoom_slider, focus_slider, iris_slider](OpticsPanel::Mode mode) {
            if (mode == OpticsPanel::Mode::Classic) {
                zoom_slider ->setValue(0);
                focus_slider->setValue(0);
                iris_slider ->setValue(0);
                m_lens_ctrl->set_modern_speed(LensController::Zoom,  0);
                m_lens_ctrl->set_modern_speed(LensController::Focus, 0);
                m_lens_ctrl->set_modern_speed(LensController::Iris,  0);
                m_lens_ctrl->restore_classic_stepping(ui->FOCUS_SPEED_SLIDER->value());
            }
        });

        m_optics_panel->set_modern_page(modern_w);
    }

    // TCUPanel — Normal mode: DIST readout + UnlockBar + TimingBarWidget
    {
        auto *normal_w = new QWidget;
        auto *nl = new QVBoxLayout(normal_w);
        nl->setContentsMargins(4, 4, 4, 4);
        nl->setSpacing(6);

        // Top row: DIST readout (left) + UnlockBar (right, fixed width).
        auto *top_row   = new QWidget(normal_w);
        auto *tr_l      = new QHBoxLayout(top_row);
        tr_l->setContentsMargins(0, 0, 0, 0);
        tr_l->setSpacing(7);

        auto *dist_lbl  = new QLabel("DIST: <b>--</b>", top_row);
        dist_lbl->setObjectName("TcuDistLabel");
        dist_lbl->setTextFormat(Qt::RichText);

        auto *unlock = new UnlockBar(top_row);
        unlock->setFixedSize(140, 24);

        tr_l->addWidget(dist_lbl, 1);
        tr_l->addWidget(unlock);

        connect(m_tcu_ctrl, &TCUController::distance_text_updated,
                this, [dist_lbl](QString t) {
            dist_lbl->setText("DIST: <b>" + t + "</b>");
        });

        // Reparent the global FIRE_LASER_BTN into Normal page so it appears
        // between the DIST row and the timing bars (matches prototype v3).
        // The button keeps its existing click signal / controller wiring;
        // only its parent/position changes.
        //
        // FIRE must stay disabled until the user drags the UnlockBar — the
        // bar only emits locked() on transitions, so the initial state must
        // be set explicitly here (the .ui default leaves it enabled).
        ui->FIRE_LASER_BTN->setParent(normal_w);
        ui->FIRE_LASER_BTN->setVisible(true);
        ui->FIRE_LASER_BTN->setEnabled(false);
        ui->FIRE_LASER_BTN->setObjectName("FireLaserBtn"); // QSS hook

        auto *timing = new TimingBarWidget(normal_w);
        timing->setBarLabel(TimingBarWidget::LaserWidth, "LASER");
        timing->setBarUnit (TimingBarWidget::LaserWidth, "ns");
        timing->setBarScale(TimingBarWidget::LaserWidth,
                            (quint64)qMax(1, (int)config->get_data().tcu.max_laser_width));
        timing->setBarColor(TimingBarWidget::LaserWidth, QColor(0x3d, 0x8b, 0xff));

        // Distance and Dov bars use the same units as DELAY_SLIDER /
        // GW_SLIDER — meters, with the scale (slider max) driven by
        // max_dist / max_dov from config. Their initial scale comes from
        // the same delay_slider_max_changed / gw_slider_max_changed signals
        // the legacy sliders listen to.
        timing->setBarLabel(TimingBarWidget::Distance, "DISTANCE");
        timing->setBarUnit (TimingBarWidget::Distance, "m");
        timing->setBarScale(TimingBarWidget::Distance,
                            (quint64)qMax(1, (int)config->get_data().tcu.max_dist));
        timing->setBarColor(TimingBarWidget::Distance, QColor(0x4c, 0xc4, 0x77));

        timing->setBarLabel(TimingBarWidget::Dov, "DEPTH");
        timing->setBarUnit (TimingBarWidget::Dov, "m");
        timing->setBarScale(TimingBarWidget::Dov,
                            (quint64)qMax(1, (int)config->get_data().tcu.max_dov));
        timing->setBarColor(TimingBarWidget::Dov, QColor(0xff, 0xb0, 0x3a));

        timing->setBarLabel(TimingBarWidget::Mcp, "MCP");
        timing->setBarUnit (TimingBarWidget::Mcp, "");
        timing->setBarScale(TimingBarWidget::Mcp,
                            (quint64)qMax(1, m_tcu_ctrl->get_mcp_max()));
        timing->setBarColor(TimingBarWidget::Mcp, QColor(0xee, 0x44, 0x44));

        nl->addWidget(top_row);
        nl->addWidget(ui->FIRE_LASER_BTN);
        nl->addWidget(timing, 1);

        connect(m_tcu_ctrl, &TCUController::laser_width_display_updated,
                this, [timing](QString u, QString n, QString) {
            timing->setBarValue(TimingBarWidget::LaserWidth,
                                (quint64)u.toLongLong() * 1000 + (quint64)n.toLongLong());
        });
        // Distance / Dov bars are driven by the same meters-domain signals
        // as the legacy DELAY_SLIDER / GW_SLIDER. Using the ns-split display
        // signals here mixed units: change_delay() interprets its arg as
        // meters and clamps to max_dist, but delay_display_updated returns
        // nanoseconds — the round-trip pinned the bar to its scale.
        connect(m_tcu_ctrl, &TCUController::delay_slider_value_changed,
                this, [timing](int val) {
            timing->setBarValue(TimingBarWidget::Distance, (quint64)qMax(0, val));
        });
        connect(m_tcu_ctrl, &TCUController::gw_slider_value_changed,
                this, [timing](int val) {
            timing->setBarValue(TimingBarWidget::Dov, (quint64)qMax(0, val));
        });
        // Track max-dist / max-dov / max-laser / mcp-max changes (Setup
        // overlay edits and TCU-type switches) so each bar's scale stays in
        // sync with the corresponding legacy slider's maximum.
        connect(m_tcu_ctrl, &TCUController::delay_slider_max_changed,
                this, [timing](int max) {
            timing->setBarScale(TimingBarWidget::Distance, (quint64)qMax(1, max));
        });
        connect(m_tcu_ctrl, &TCUController::gw_slider_max_changed,
                this, [timing](int max) {
            timing->setBarScale(TimingBarWidget::Dov, (quint64)qMax(1, max));
        });
        connect(m_tcu_ctrl, &TCUController::mcp_slider_max_changed,
                this, [timing](int max) {
            timing->setBarScale(TimingBarWidget::Mcp, (quint64)qMax(1, max));
        });
        connect(m_setup_overlay, &SetupOverlay::max_laser_changed,
                this, [timing](float max_ns) {
            timing->setBarScale(TimingBarWidget::LaserWidth,
                                (quint64)qMax(1, (int)max_ns));
        });
        connect(m_tcu_ctrl, &TCUController::mcp_display_updated,
                this, [timing](int val, QString) {
            timing->setBarValue(TimingBarWidget::Mcp, (quint64)qMax(0, val));
        });

        // Bar → controller: user drag updates TCU params. All four bars
        // carry nanoseconds (LaserWidth / Distance=Delay / Dov=GW) except
        // Mcp which is the raw slider unit. setBarValue no longer re-emits,
        // so the controller→bar path above is loop-safe.
        connect(timing, &TimingBarWidget::barValueChanged, this,
                [this](TimingBarWidget::BarId id, quint64 value) {
            switch (id) {
            case TimingBarWidget::LaserWidth:
                m_tcu_ctrl->set_laser_width(static_cast<float>(value));
                m_tcu_ctrl->update_laser_width();
                break;
            case TimingBarWidget::Distance:
                m_tcu_ctrl->change_delay(static_cast<int>(value));
                break;
            case TimingBarWidget::Dov:
                m_tcu_ctrl->change_gatewidth(static_cast<int>(value));
                break;
            case TimingBarWidget::Mcp:
                m_tcu_ctrl->change_mcp(static_cast<int>(value));
                break;
            default: break;
            }
        });

        // FIRE_LASER_BTN gating:
        //   text="FIRE" → only clickable while the UnlockBar is unlocked
        //   text="STOP" → always clickable (so the user can halt firing
        //                 even after the bar auto-relocks)
        QPushButton *fire_btn = ui->FIRE_LASER_BTN;
        auto refreshFireEnabled = [fire_btn, unlock]() {
            const bool is_stop = fire_btn->text() == "STOP";
            fire_btn->setEnabled(is_stop || unlock->isUnlocked());
        };
        connect(unlock, &UnlockBar::unlocked, this, refreshFireEnabled);
        connect(unlock, &UnlockBar::locked,   this, refreshFireEnabled);
        // Controller updates text on every FIRE↔STOP transition; recompute
        // enabled after applying the new text so the unlock gate is honored.
        connect(m_laser_ctrl, &LaserController::fire_btn_update,
                this, [fire_btn, refreshFireEnabled](bool, QString text) {
            if (!text.isEmpty()) fire_btn->setText(text);
            refreshFireEnabled();
        });
        // Relock when laser is turned off (button text reverts to "ON")
        connect(m_laser_ctrl, &LaserController::laser_btn_update,
                this, [unlock](bool, QString text) {
            if (text == "ON") unlock->relock();
        });

        m_tcu_panel->set_normal_page(normal_w);
    }

    // PipelineProcessor — single shared instance. Per-stream/display state is
    // reached through DisplayRouter; only true globals go into PipelineGlobals.
    {
        m_globals.params_mutex      = &m_params_mutex;
        m_globals.processing_params = &m_processing_params;
        m_globals.roi_mutex         = &m_roi_mutex;
        m_globals.list_roi          = &list_roi;
        m_globals.yolo_init_mutex   = &m_yolo_init_mutex;
        m_globals.tcu_ctrl          = m_tcu_ctrl;
        m_globals.scan_ctrl         = m_scan_ctrl;
        m_globals.device_mgr        = m_device_mgr;
        m_globals.aux_panel         = m_aux_panel;
        m_globals.secondary_display = secondary_display;
        m_globals.scan_params       = &m_scan_ctrl->params();
        m_globals.save_location     = &save_location;
        m_globals.tp                = &tp;
        m_pipeline = new PipelineProcessor(m_globals, this);
    }

    // One FfmpegSource per stream slot. Lifecycle signals run on the GUI
    // thread; frame_decoded runs direct so frames hit BoundedQueue without an
    // unbounded GUI-event backlog.
    for (std::size_t i = 0; i < kStreamCount; ++i) {
        m_ffmpeg[i] = new FfmpegSource(this);
        connect(m_ffmpeg[i], &FfmpegSource::opened,        this, &UserPanel::on_ffmpeg_opened,   Qt::QueuedConnection);
        connect(m_ffmpeg[i], &FfmpegSource::frame_decoded, this, &UserPanel::on_ffmpeg_frame,    Qt::DirectConnection);
        connect(m_ffmpeg[i], &FfmpegSource::failed,        this, &UserPanel::on_ffmpeg_failed,   Qt::QueuedConnection);
        connect(m_ffmpeg[i], &FfmpegSource::finished,      this, &UserPanel::on_ffmpeg_finished, Qt::QueuedConnection);
    }

    // Connect PipelineProcessor signals
    // packet_lost_updated: discarded in UI redesign (Sprint 16)
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
    connect(m_pipeline, &PipelineProcessor::yolo_status_changed, this, [this](int display_idx, int state) {
        if (m_img_proc) m_img_proc->set_model_status(display_idx, state);
    }, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::recorder_finalized, this,
            [this](int display_idx, int kind, QString temp_path) {
        if (display_idx < 0 || display_idx >= kDisplayCount) return;
        DisplaySlot& d = m_router.display(static_cast<DisplayId>(display_idx));
        std::atomic<bool>& flag = (kind == 0) ? d.record_raw : d.record_proc;
        if (!flag) return;   // user-initiated STOP already finalized — nothing to do
        flag = false;
        if (!temp_path.isEmpty()) {
            QString dest = save_location + "/" + temp_path.section('/', -1, -1);
            std::thread t(UserPanel::move_to_dest, temp_path, dest);
            t.detach();
        }
        // Reset the button text matching the just-finalized recorder.
        if (display_idx == 0) {
            if (kind == 0) ui->SAVE_AVI_BUTTON  ->setText(tr("ORI"));
            else           ui->SAVE_FINAL_BUTTON->setText(tr("RES"));
        } else {
            // Alt rec button id is 4 (alt1) or 5 (alt2); see alt_ctrl_grp setup.
            const int btn_id = 4 + (display_idx - 1);
            if (auto *btn = alt_ctrl_grp->button(btn_id)) btn->setText("REC");
        }
        if (kind == 0) d.raw_path.clear();
        else           d.proc_path.clear();
    }, Qt::QueuedConnection);
    connect(m_pipeline, &PipelineProcessor::source_state_changed, this, [this](int display_idx, bool dead) {
        if (display_idx < 0 || display_idx >= 3) return;
        set_source_dead_overlay(static_cast<DisplayId>(display_idx), dead);
    }, Qt::QueuedConnection);
#ifdef LVTONG
    connect(m_pipeline, &PipelineProcessor::set_model_list_enabled, this, [this](bool enabled) {
        m_setup_overlay->w()->MODEL_LIST->setEnabled(enabled);
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
    connect(this, SIGNAL(update_device_list(int, QStringList)), m_setup_overlay, SLOT(update_device_list(int, QStringList)));
    connect(this, SIGNAL(device_connection_status_changed(int, QStringList)), m_setup_overlay, SLOT(update_device_list(int, QStringList)));

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

    connect(m_setup_overlay, &SetupOverlay::set_auto_rep_freq, m_device_mgr->tcu(), [this](bool arg1){ emit send_uint_tcu_msg(TCU::AUTO_PRF, arg1); });
    connect(m_setup_overlay, &SetupOverlay::set_ab_lock,        m_device_mgr->tcu(), [this](bool arg1){ emit send_uint_tcu_msg(TCU::AB_LOCK, arg1); });

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
    // Stop every producer before tearing down consumers and queues —
    // otherwise producer threads keep pushing into structures that are
    // about to disappear. cv:// threads are detached; we can't join, but
    // flipping keep_alive lets them exit on their own iteration. The cam
    // SDK callback also needs to deregister before the StreamSlot queues
    // (held by m_router, a UserPanel member) are destroyed.
    for (std::size_t i = 0; i < kStreamCount; ++i) {
        if (m_ffmpeg[i]) m_ffmpeg[i]->stop();
        if (m_cv_run[i]) m_cv_run[i]->store(false);
    }
    if (curr_cam && start_grabbing) curr_cam->stop_grabbing();

    for (int i = 0; i < 3; i++) {
        if (m_router.display(static_cast<DisplayId>(i)).fps_calc) {
            if (m_router.display(static_cast<DisplayId>(i)).fps_calc->isRunning()) m_router.display(static_cast<DisplayId>(i)).fps_calc->stop();
            delete m_router.display(static_cast<DisplayId>(i)).fps_calc;
            m_router.display(static_cast<DisplayId>(i)).fps_calc = nullptr;
        }
    }

    for (int i = 0; i < 3; i++) {
        m_router.display(static_cast<DisplayId>(i)).consumer_run = false;
        if (m_router.display(static_cast<DisplayId>(i)).consumer) {
            m_router.display(static_cast<DisplayId>(i)).consumer->quit();
            m_router.display(static_cast<DisplayId>(i)).consumer->wait();
            delete m_router.display(static_cast<DisplayId>(i)).consumer;
            m_router.display(static_cast<DisplayId>(i)).consumer = NULL;
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
    // auto-load default config if it exists. Path matches Config::auto_save
    // (config.cpp:627) so save+load round-trip across runs regardless of CWD.
    QString default_config_path = QCoreApplication::applicationDirPath() + "/default.json";
    if (QFile::exists(default_config_path)) {
        if (config->load_from_file(default_config_path)) {
            syncConfigToPreferences();
        }
    }

    // Set default PTZ type to udp-scw
    m_setup_overlay->w()->PTZ_TYPE_LIST->setCurrentIndex(2);

    // YOLO model selection is per-display state (lives on DisplaySlot::pcfg).
    // Per feedback_per_display_toggles_not_persisted: not saved to config file,
    // defaults to 0 (none) at startup.
    ui->MAIN_MODEL_LIST->setCurrentIndex(0);
    ui->ALT1_MODEL_LIST->setCurrentIndex(0);
    ui->ALT2_MODEL_LIST->setCurrentIndex(0);

    // Combobox changes write directly into the affected DisplaySlot's pcfg
    // under m_params_mutex (the per-display equivalent of update_processing_params).
    connect(ui->MAIN_MODEL_LIST, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QMutexLocker lk(&m_params_mutex);
        m_router.display(DisplayId::Main).pcfg.model_idx = index;
    });
    connect(ui->ALT1_MODEL_LIST, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QMutexLocker lk(&m_params_mutex);
        m_router.display(DisplayId::Alt1).pcfg.model_idx = index;
    });
    connect(ui->ALT2_MODEL_LIST, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QMutexLocker lk(&m_params_mutex);
        m_router.display(DisplayId::Alt2).pcfg.model_idx = index;
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
    m_setup_overlay->w()->AUTO_MCP_CHK->click();
    m_setup_overlay->w()->UNIT_LIST->setCurrentIndex(2);
    m_setup_overlay->w()->MAX_DIST_EDT->setText("1800.00");
    m_setup_overlay->w()->MAX_DOV_EDT->setText("150.00");
    m_setup_overlay->data_exchange(false);
    m_setup_overlay->w()->MAX_DIST_EDT->emit editingFinished();
    m_setup_overlay->w()->MAX_DOV_EDT->emit editingFinished();

    ui->FRAME_AVG_CHECK->setText("Filter");
#else //SIMPLE_UI
    ui->AUTO_MCP_CHK->hide();
    ui->SIMPLE_LASER_CHK->hide();
    ui->GW_SLIDER->hide();

    ui->FOCUS_SPEED_LABEL->hide();

    // Iris (RADIUS_INC/DEC) is now part of the unified Lens column layout —
    // the new IRIS_LABEL in the .ui serves as its column header. Keep the
    // legacy LASER_RADIUS_LABEL hidden (overlaps the new speed row).
    ui->LASER_RADIUS_LABEL->hide();

    ui->FRAME_AVG_CHECK->setText("Avg");
#endif //SIMPLE_UI

    ui->BRIGHTNESS_LABEL->hide();
    ui->BRIGHTNESS_SLIDER->hide();
    ui->PSEUDOCOLOR_CHK->show();

#ifdef LVTONG
    // Conditional click: idempotent against a persisted-true config (so we
    // don't toggle it back off), and immune to future .ui default flips
    // (setChecked(true) would silently no-op if the default were already true).
    if (!m_setup_overlay->w()->UNDERWATER_CHK->isChecked())
        m_setup_overlay->w()->UNDERWATER_CHK->click();

    ui->DISTANCE->hide();
    ui->DIST_BTN->hide();
    ui->CURRENT_EDIT->setText("18");
    ui->SIMPLE_LASER_CHK->setEnabled(false);

    if (!m_setup_overlay->w()->SHARE_CHK->isChecked())
        m_setup_overlay->w()->SHARE_CHK->click();
    m_setup_overlay->w()->IMG_FORMAT_LST->setCurrentIndex(1);

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
    m_setup_overlay->w()->UNIT_LIST->setCurrentIndex(2);
    m_setup_overlay->w()->AUTO_MCP_CHK->click();
    m_setup_overlay->data_exchange(false);
#else //LVTONG
    // FIRE_LASER_BTN now lives inside TCUPanel's Normal page (reparented in
    // setup_floating_panels) — visible by default; the laser controller
    // continues to drive its enabled/text state.
    // LASER_BTN is hidden because FIRE_LASER_BTN already sends laser ON/OFF
    // commands on every fire/stop transition (see LaserController::on_FIRE_
    // LASER_BTN_clicked), so the separate power toggle is redundant.
    ui->LASER_BTN->hide();
    ui->FISHNET_RESULT->hide();
#endif //LVTONG

    ui->LOGO->hide();
//    ui->ANALYSIS_RADIO->hide();
//    ui->PLUGIN_DISPLAY_1->hide();

#ifdef USING_CAMERALINK
    m_setup_overlay->w()->CAMERALINK_CHK->click();
#else //USING_CAMERALINK
    m_setup_overlay->w()->CAMERALINK_CHK->hide();
#endif //USING_CAMERALINK
    ui->SENSOR_TAPS_BTN->hide();

    // COM ports will be loaded from JSON config during auto-load
    QLineEdit *com_edits[] = { ui->TCU_COM_EDIT, ui->LENS_COM_EDIT, ui->LASER_COM_EDIT, ui->PTZ_COM_EDIT, ui->RANGE_COM_EDIT };
    for (int i = 0; i < 5; i++) com_edits[i]->emit returnPressed();

    if (m_device_mgr->serial(0)->isOpen() && m_device_mgr->serial(3)->isOpen()) on_LASER_BTN_clicked();

    // Start maximized so the SETUP overlay shows full content (the FlowLayout
    // in IMG_PROC wraps narrowly without enough viewport width).
    ui->TITLE->process_maximize();
}

void UserPanel::data_exchange(bool read){
    if (read) {
//        device_idx = ui->DEVICE_SELECTION->currentIndex();
        calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 4 + 4;

        trigger_by_software = ui->SOFTWARE_CHECK->isChecked();
        m_router.display(m_picked_display).pcfg.image_3d = ui->IMG_3D_CHECK->isChecked();
        m_router.display(m_picked_display).pcfg.enable_3d = ui->IMG_3D_CHECK->isChecked();

        gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
        time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
        m_router.stream(StreamId::S0).meta.fps = ui->CCD_FREQ_EDIT->text().toFloat();
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
        ui->IMG_3D_CHECK->setChecked(m_router.display(m_picked_display).pcfg.image_3d);

        ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
        ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
        ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", m_router.stream(StreamId::S0).meta.fps));
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
        ui->IRIS_EDIT->setText(QString::asprintf("%d", m_device_mgr->lens()->get(Lens::LASER_RADIUS)));

        ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", m_device_mgr->ptz()->get(PTZ::ANGLE_H)));
        ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", m_device_mgr->ptz()->get(PTZ::ANGLE_V)));
    }
}

// Pipeline methods moved to src/pipeline/pipelineprocessor.cpp

void UserPanel::swap_grab_thread_display(int display_idx1, int display_idx2)
{
    DisplayId d1 = static_cast<DisplayId>(display_idx1);
    DisplayId d2 = static_cast<DisplayId>(display_idx2);
    // Each display reads from its OWN consumer thread. If either side has no
    // consumer running (because that slot has never had a stream loaded into
    // it), the post-swap route lands on a display that can't render. Spin one
    // up before the swap so frames flow as soon as the route flips.
    auto ensure_consumer = [this](DisplayId did) {
        DisplaySlot& d = m_router.display(did);
        if (d.consumer && d.consumer->isRunning()) return;
        if (d.consumer) {
            d.consumer_run = false;
            d.consumer->quit();
            d.consumer->wait();
            delete d.consumer;
            d.consumer = NULL;
        }
        d.consumer_run = true;
        d.consumer = new ConsumerThread(m_pipeline, &m_router, &d);
        if (d.consumer) d.consumer->start();
        // Alt FloatingWindow visibility is the user's call (Alt+1 / Alt+2);
        // swap does not auto-show.
    };
    ensure_consumer(d1);
    ensure_consumer(d2);
    // Router::swap atomically exchanges which streams the two displays show
    // and bumps the route generation; each consumer thread observes the bump
    // on its next snapshot() and resets per-route caches (frame-avg / ECC /
    // recorders / modified_result) before continuing. Auto-finalize on
    // recorders is emitted via recorder_finalized so the GUI side moves the
    // temp file + flips the rec flag + resets the button text.
    m_router.swap(d1, d2);
    // Clear any stale pixmap that landed in-flight before the swap took
    // effect. Queued so it lands after any in-flight set_pixmap event.
    Display* w1 = m_router.display(d1).widget;
    Display* w2 = m_router.display(d2).widget;
    if (w1) QMetaObject::invokeMethod(w1, "clear", Qt::QueuedConnection);
    if (w2) QMetaObject::invokeMethod(w2, "clear", Qt::QueuedConnection);
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
    m_setup_overlay->w()->REFRESH_AVAILABLE_PORTS_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    ui->SWITCH_TCU_UI_BTN->setIcon(QIcon(":/tools/" + QString(app_theme ? "light" : "dark") + "/switch"));
    if (m_side_display) m_side_display->apply_theme(app_theme);

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
    if (m_router.display(DisplayId::Main).save_raw) on_SAVE_BMP_BUTTON_clicked();
    if (m_router.display(DisplayId::Main).save_proc) on_SAVE_RESULT_BUTTON_clicked();
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
    m_setup_overlay->switch_language(lang == 0, &trans);
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
    laser_settings->reject();
    for (FloatingWindow* &w: fw_display) w->close();
    secondary_display->close();
    if (QFile::exists("HQVSDK.xml")) QFile::remove("HQVSDK.xml");
    aliasing->close();
    preset->close();
}

int UserPanel::shut_down() {
    if (m_router.display(DisplayId::Main).record_raw) on_SAVE_AVI_BUTTON_clicked();
    if (m_router.display(DisplayId::Main).record_proc) on_SAVE_FINAL_BUTTON_clicked();

    m_router.display(DisplayId::Main).consumer_run = false;
    if (m_router.display(DisplayId::Main).consumer) {
        m_router.display(DisplayId::Main).consumer->quit();
        m_router.display(DisplayId::Main).consumer->wait();
        delete m_router.display(DisplayId::Main).consumer;
        m_router.display(DisplayId::Main).consumer = NULL;
    }

    if (!m_router.stream(StreamId::S0).frames.empty()) m_router.stream(StreamId::S0).frames.clear();

    if (curr_cam) {
        // Skip the SDK shut_down() call when the device was never initialized
        // (device_on=false). The adapter's stop_grabbing/shut_down path
        // returns INT_MIN (-2147483648) for "device not initialized", which
        // is harmless but noisy. Still tear down the C++ object.
        if (device_on) {
            int shutdown_result = curr_cam->shut_down();
            if (shutdown_result != 0) {
                qWarning("Camera shutdown failed with error code: %d", shutdown_result);
                // Try to force cleanup even if shutdown failed
            }
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
    if (m_setup_overlay->cameralink())    curr_cam = new EuresysCam;
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
    enable_controls(device_type = (curr_cam->search_for_devices() ?  (m_setup_overlay->cameralink() ? 3 : (config->get_data().device.ebus ? 2 : 1)) : 0));
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
    curr_cam->frame_rate(true, &m_router.stream(StreamId::S0).meta.fps);
    ui->GAIN_SLIDER->setValue((int)gain_analog_edit);
    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", m_router.stream(StreamId::S0).meta.fps));

#ifdef LVTONG
    int new_w = 1024;
    int new_h = 1024;
    int new_x = 296;
    int new_y = 32;
    curr_cam->frame_size(false, &new_w, &new_h);
    curr_cam->frame_offset(false, &new_x, &new_y);
#endif

//    ui->TITLE->prog_settings->set_pixel_format(0);
    m_setup_overlay->set_pixel_format(0);
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

    // If the camera/SDK stopped underneath us, the UI/router can still think
    // S0 is active. Force the old acquisition path to quiesce before calling
    // start_grabbing() again; camera SDKs often crash on start-while-half-live.
    const DisplayId old_cam_display = m_router.display_for(StreamId::S0);
    if (start_grabbing || m_router.stream(StreamId::S0).producer_run.load() || old_cam_display != DisplayId::_Count) {
        curr_cam->stop_grabbing();
        m_router.stream(StreamId::S0).frames.close();
        m_router.stream(StreamId::S0).frames.clear();
        m_router.stream(StreamId::S0).frame_info.close();
        m_router.stream(StreamId::S0).frame_info.clear();
        m_router.stream(StreamId::S0).producer_run = false;
        if (old_cam_display != DisplayId::_Count) {
            DisplaySlot& d = m_router.display(old_cam_display);
            d.consumer_run = false;
            if (d.consumer) {
                d.consumer->quit();
                d.consumer->wait();
                delete d.consumer;
                d.consumer = NULL;
            }
            if (d.widget) d.widget->is_grabbing = false;
        }
        m_router.unroute(StreamId::S0);
        start_grabbing = false;
    }

    // Cam binds to StreamId::S0. Stop any other source already producing
    // there so the cam SDK doesn't push into a queue another producer is
    // also feeding.
    stop_all_sources_on_slot(0);

    // Cam SDK pushes frames through StreamSlot::S0's sink. m_cam_sink stores
    // it at a stable address so the SDK's user_data pointer stays valid.
    // Reopen producer queues in case a previous stop closed them.
    m_router.stream(StreamId::S0).frames.reopen();
    m_router.stream(StreamId::S0).frame_info.reopen();
    m_router.stream(StreamId::S0).meta.source_type = SourceType::IndustrialCam;
    m_cam_sink = m_router.stream(StreamId::S0).sink();
    curr_cam->set_user_pointer(&m_cam_sink);

    // TODO complete BayerGB process & display
    curr_cam->pixel_type(true, &m_router.stream(StreamId::S0).meta.pixel_format);
    switch (m_router.stream(StreamId::S0).meta.pixel_format) {
    case PixelType_Gvsp_Mono8:       m_router.stream(StreamId::S0).meta.is_color = false; update_pixel_depth( 8); break;
    case PixelType_Gvsp_Mono10:      m_router.stream(StreamId::S0).meta.is_color = false; update_pixel_depth(10); break;
    case PixelType_Gvsp_Mono12:      m_router.stream(StreamId::S0).meta.is_color = false; update_pixel_depth(12); break;
    case PixelType_Gvsp_BayerGB8:    m_router.stream(StreamId::S0).meta.is_color =  true; update_pixel_depth( 8); break;
    case PixelType_Gvsp_RGB8_Packed: m_router.stream(StreamId::S0).meta.is_color =  true; update_pixel_depth( 8); break;
    default:                         m_router.stream(StreamId::S0).meta.is_color = false; update_pixel_depth( 8); break;
    }

    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
//    ptr_tcu->duty = time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
//    ptr_tcu->ccd_freq = frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();
    emit send_double_tcu_msg(TCU::DUTY, time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000);
    emit send_double_tcu_msg(TCU::CCD_FREQ, m_router.stream(StreamId::S0).meta.fps = ui->CCD_FREQ_EDIT->text().toFloat());

    // Tear down any existing main-display consumer cleanly.
    if (m_router.display(DisplayId::Main).consumer_run || m_router.display(DisplayId::Main).consumer) {
        m_router.display(DisplayId::Main).consumer_run = false;
        if (m_router.display(DisplayId::Main).consumer) {
            m_router.display(DisplayId::Main).consumer->quit();
            m_router.display(DisplayId::Main).consumer->wait();
            delete m_router.display(DisplayId::Main).consumer;
            m_router.display(DisplayId::Main).consumer = NULL;
        }
        m_router.unroute(StreamId::S0);
    }

    // adjust display size according to frame size
    curr_cam->frame_size(true, &m_router.stream(StreamId::S0).meta.width, &m_router.stream(StreamId::S0).meta.height);
    ui->TITLE->set_resolution(m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height);
    qInfo("frame w: %d, h: %d", m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height);
    //    QRect region = ui->SOURCE_DISPLAY->geometry();
    //    region.setHeight(ui->SOURCE_DISPLAY->width() * h / w);
    //    ui->SOURCE_DISPLAY->setGeometry(region);
    //    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
    //    ui->SOURCE_DISPLAY->update_roi(QPoint());
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
    secondary_display->resize_display(m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height);

    m_router.display(DisplayId::Main).consumer_run = true;

    m_router.display(DisplayId::Main).consumer = new ConsumerThread(m_pipeline, &m_router, &m_router.display(DisplayId::Main));
    if (m_router.display(DisplayId::Main).consumer == NULL) {
        m_router.display(DisplayId::Main).consumer_run = false;
        QMessageBox::warning(this, "PROMPT", tr("Create thread fail"));
        return;
    }
    m_router.route(DisplayId::Main, StreamId::S0);
    set_source_dead_overlay(DisplayId::Main, false);
    m_router.stream(StreamId::S0).producer_run = true;
    m_router.display(DisplayId::Main).consumer->start();

    if (curr_cam->start_grabbing()) {
        m_router.display(DisplayId::Main).consumer_run = false;
        if (m_router.display(DisplayId::Main).consumer) {
            m_router.display(DisplayId::Main).consumer->quit();
            m_router.display(DisplayId::Main).consumer->wait();
            delete m_router.display(DisplayId::Main).consumer;
            m_router.display(DisplayId::Main).consumer = NULL;
        }
        m_router.unroute(StreamId::S0);
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

    const DisplayId cam_display = m_router.display_for(StreamId::S0);
    if (cam_display == DisplayId::Main) {
        if (m_router.display(DisplayId::Main).record_raw) on_SAVE_AVI_BUTTON_clicked();
        if (m_router.display(DisplayId::Main).record_proc) on_SAVE_FINAL_BUTTON_clicked();
    }

    // Stop the cam SDK first so callbacks deregister before we close the
    // queue. This is the cam stop+restart crash fix path: previously the SDK
    // could fire one final frame into a freed queue. Now we close+drain after
    // stop_grabbing returns.
    curr_cam->stop_grabbing();

    // Close the producer queue to wake the consumer on closed-empty.
    m_router.stream(StreamId::S0).frames.close();
    m_router.stream(StreamId::S0).frames.clear();
    m_router.stream(StreamId::S0).frame_info.close();
    m_router.stream(StreamId::S0).frame_info.clear();
    m_router.stream(StreamId::S0).producer_run = false;

    if (cam_display != DisplayId::_Count) {
        set_source_dead_overlay(cam_display, true);
        DisplaySlot& d = m_router.display(cam_display);
        d.consumer_run = false;
        if (d.consumer) {
            d.consumer->quit();
            d.consumer->wait();
            delete d.consumer;
            d.consumer = NULL;
        }
    }
    m_router.unroute(StreamId::S0);

    start_grabbing = false;
    ui->SOURCE_DISPLAY->is_grabbing = false;
    QTimer::singleShot(100, this, SLOT(clean()));
    enable_controls(device_type);
    if (device_type == -1) ui->ENUM_BUTTON->click();
    if (device_type == -2) ui->ENUM_BUTTON->click();
}

void UserPanel::on_SAVE_BMP_BUTTON_clicked()
{
    if (!QDir(save_location + "/ori_bmp").exists()) QDir().mkdir(save_location + "/ori_bmp");
    if (device_type == -1 || !config->get_data().save.consecutive_capture) m_router.display(DisplayId::Main).save_raw = 1;
    else{
        m_router.display(DisplayId::Main).save_raw = !m_router.display(DisplayId::Main).save_raw;
        ui->SAVE_BMP_BUTTON->setText(m_router.display(DisplayId::Main).save_raw ? tr("STOP") : tr("ORI"));
        m_setup_overlay->w()->CONSECUTIVE_CAPTURE_CHK->setEnabled(!m_router.display(DisplayId::Main).save_raw);
    }
}

void UserPanel::on_SAVE_FINAL_BUTTON_clicked()
{
    DisplaySlot& d = m_router.display(DisplayId::Main);
    const StreamId sid = m_router.snapshot(DisplayId::Main).stream;
    const StreamMeta meta = (sid == StreamId::None) ? StreamMeta{} : m_router.stream(sid).meta;
//    cv::imwrite(QString(save_location + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".jpg").toLatin1().data(), modified_result);
    if (d.record_proc) {
//        curr_cam->stop_recording(0);
        QString res_avi;
        {
            QMutexLocker locker(&d.paint_mutex);
            res_avi = d.proc_path;
            d.proc_path.clear();
            d.proc_writer.release();
        }
        if (!res_avi.isEmpty()) {
            QString dest = save_location + "/" + res_avi.section('/', -1, -1);
            std::thread t(UserPanel::move_to_dest, QString(res_avi), QString(dest));
            t.detach();
        }
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        QString res_avi;
        {
            QMutexLocker locker(&d.paint_mutex);
            res_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_main_res.mp4");
            d.proc_path = res_avi;
//            m_router.display(DisplayId::Main).proc_writer.open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(m_router.display(m_picked_display).pcfg.image_3d ? m_router.stream(StreamId::S0).meta.width + 104 : m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height), m_router.stream(StreamId::S0).meta.is_color || m_router.display(m_picked_display).pcfg.image_3d);
            {
                // SAVE_FINAL records Main display; size/color from Main's pcfg.
                const bool main_3d = m_router.display(DisplayId::Main).pcfg.image_3d;
                const bool main_pc = m_router.display(DisplayId::Main).pcfg.pseudocolor;
                const int vw_w = main_3d ? meta.width + 104 : meta.width;
                const int vw_h = meta.height;
                const bool vw_color = meta.is_color || main_pc || main_3d;
                // Real-time fps: prefer the TimedQueue measured arrival rate
                // (count over the 5 s window). Fall back to frame_rate_edit
                // only if the window is too sparse to be meaningful (e.g.
                // record was triggered immediately after stream connect).
                const double measured_fps = m_router.display(DisplayId::Main).fps_calc ? m_router.display(DisplayId::Main).fps_calc->count() / 5.0 : 0.0;
                const double vw_fps = (measured_fps >= 1.0) ? measured_fps : meta.fps;
                qDebug() << "[vid_out RES] open" << res_avi
                         << "size=" << vw_w << "x" << vw_h
                         << "fps=" << vw_fps
                         << "(measured=" << measured_fps
                         << "commanded=" << meta.fps << ")"
                         << "color=" << vw_color;
                m_router.display(DisplayId::Main).proc_writer.open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), vw_fps, cv::Size(vw_w, vw_h), vw_color);
                qDebug() << "[vid_out RES] isOpened=" << m_router.display(DisplayId::Main).proc_writer.isOpened();
            }
        }
    }
    m_router.display(DisplayId::Main).record_proc = !m_router.display(DisplayId::Main).record_proc;
    ui->SAVE_FINAL_BUTTON->setText(m_router.display(DisplayId::Main).record_proc ? tr("STOP") : tr("RES"));
}

void UserPanel::on_SET_PARAMS_BUTTON_clicked()
{
    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
    time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
    m_router.stream(StreamId::S0).meta.fps = ui->CCD_FREQ_EDIT->text().toFloat();

    // CCD FREQUENCY
//    ptr_tcu->communicate_display(convert_to_send_tcu(0x06, 1.25e8 / fps), 7, 1, false);
//    ptr_tcu->set_user_param(TCUThread::CCD_FREQ, frame_rate_edit);
    emit send_double_tcu_msg(TCU::CCD_FREQ, m_router.stream(StreamId::S0).meta.fps = ui->CCD_FREQ_EDIT->text().toFloat());

    // DUTY RATIO -> EXPO. TIME
//    ptr_tcu->communicate_display(convert_to_send_tcu(0x07, duty * 1.25e2), 7, 1, false);
//    ptr_tcu->set_user_param(TCUThread::DUTY, time_exposure_edit);
    emit send_double_tcu_msg(TCU::DUTY, time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000);

//    time_exposure_edit = ptr_tcu->duty;
//    frame_rate_edit = ptr_tcu->fps;
    if (device_on) {
        curr_cam->time_exposure(false, &time_exposure_edit);
        curr_cam->frame_rate(false, &m_router.stream(StreamId::S0).meta.fps);
//        curr_cam->gain_analog(false, &gain_analog_edit);
        change_gain(gain_analog_edit);
    }
    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", m_router.stream(StreamId::S0).meta.fps));

    for (int i = 0; i < 3; i++) m_router.display(static_cast<DisplayId>(i)).fps_calc->empty_q();
}

void UserPanel::on_GET_PARAMS_BUTTON_clicked()
{
    curr_cam->trigger_mode(true, &trigger_mode_on);
    trigger_mode_on ? on_TRIGGER_RADIO_clicked() : on_CONTINUOUS_RADIO_clicked();
    curr_cam->trigger_source(true, &trigger_by_software);
    trigger_source = trigger_by_software ? 1 : 2;

    curr_cam->time_exposure(true, &time_exposure_edit);
    curr_cam->gain_analog(true, &gain_analog_edit);
    curr_cam->frame_rate(true, &m_router.stream(StreamId::S0).meta.fps);

    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)std::round(gain_analog_edit)));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", m_router.stream(StreamId::S0).meta.fps));
}

void UserPanel::on_FILE_PATH_BROWSE_clicked()
{
    QString temp = QFileDialog::getExistingDirectory(this, tr("Select folder"), save_location);
    if (!temp.isEmpty()) save_location = temp;
    if (m_router.display(DisplayId::Main).save_raw && !QDir(temp + "/ori_bmp").exists()) QDir().mkdir(temp + "/ori_bmp");
    if (m_router.display(DisplayId::Main).save_proc && !QDir(temp + "/res_bmp").exists()) QDir().mkdir(temp + "/res_bmp");

    ui->FILE_PATH_EDIT->setText(save_location);
}

void UserPanel::clean()
{
    ui->SOURCE_DISPLAY->clear();
    ui->DATA_EXCHANGE->clear();
    if (m_router.display(DisplayId::Main).consumer_run) return;
    m_router.stream(StreamId::S0).frames.clear();
    m_router.stream(StreamId::S0).latest_raw.release();
    m_router.display(DisplayId::Main).modified_result.release();
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
    m_setup_overlay->enable_ip_editing(device_type > 0 && config->get_data().tcu.type < curr_cam->gige_device_num());
    if (device_type > 0) {
        int ip, gateway, nic_address;
        curr_cam->ip_address(true, config->get_data().tcu.type, &ip, &gateway, &nic_address);
//        ui->TITLE->prog_settings->config_ip(true, ip, gateway);
        m_setup_overlay->config_ip(true, ip, gateway, nic_address);
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

    this->m_router.stream(StreamId::S0).meta.pixel_format = pixel_format;
    int ret = curr_cam->pixel_type(false, &pixel_format);
    switch (pixel_format) {
    case PixelType_Gvsp_Mono8:       m_router.stream(static_cast<StreamId>(display_idx)).meta.is_color = false; update_pixel_depth( 8); break;
    case PixelType_Gvsp_Mono10:      m_router.stream(static_cast<StreamId>(display_idx)).meta.is_color = false; update_pixel_depth(10); break;
    case PixelType_Gvsp_Mono12:      m_router.stream(static_cast<StreamId>(display_idx)).meta.is_color = false; update_pixel_depth(12); break;
    case PixelType_Gvsp_BayerGB8:    m_router.stream(static_cast<StreamId>(display_idx)).meta.is_color =  true; update_pixel_depth( 8); break;
    case PixelType_Gvsp_RGB8_Packed: m_router.stream(static_cast<StreamId>(display_idx)).meta.is_color =  true; update_pixel_depth( 8); break;
    default:                         m_router.stream(static_cast<StreamId>(display_idx)).meta.is_color = false; update_pixel_depth( 8); break;
    }

    if (was_playing) ui->START_GRABBING_BUTTON->click();
}

void UserPanel::update_lower_3d_thresh()
{
    // SETUP overlay just changed the canonical (Config) threshold. Push it
    // into Main's pcfg directly — 3D only runs on Main, so this is where the
    // value actually has effect. If the GENERAL picker is currently on Main,
    // also refresh RANGE_THRESH_EDIT; otherwise leave it alone (it's showing
    // an Alt's independent value).
    const double normalized = config->get_data().image_proc.lower_3d_thresh;
    m_router.display(DisplayId::Main).pcfg.lower_3d_thresh = normalized;
    if (m_picked_display == DisplayId::Main) {
        QSignalBlocker b(ui->RANGE_THRESH_EDIT);
        ui->RANGE_THRESH_EDIT->setText(QString::number(
            static_cast<int>(normalized * (1 << m_router.stream(StreamId::S0).meta.pixel_depth))));
    }
}

void UserPanel::reset_custom_3d_params()
{
//    m_setup_overlay->w()->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(ptr_tcu->delay_a)));
//    m_setup_overlay->w()->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(ptr_tcu->gate_width_a)));
    m_setup_overlay->w()->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(m_device_mgr->tcu()->get(TCU::DELAY_A))));
    m_setup_overlay->w()->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A))));
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
    // Match the temp extension to the user-picked extension so VideoWriter
    // writes the right container — otherwise the later rename to .avi leaves
    // an MP4-container file with an .avi suffix.
    QString ext = QFileInfo(output_filename).suffix();
    if (ext.isEmpty()) ext = "mp4";
    temp_output_filename = TEMP_SAVE_LOCATION + "/" + QString::number(rand(), 16) + "." + ext;
    qDebug() << "[vid_out PROC] open" << temp_output_filename
             << "size=" << m_router.stream(StreamId::S0).meta.width << "x" << m_router.stream(StreamId::S0).meta.height
             << "fps=" << m_router.stream(StreamId::S0).meta.fps
             << "color=" << true;
    vid_out_proc.open(temp_output_filename.toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), m_router.stream(StreamId::S0).meta.fps, cv::Size(m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height), true);
    qDebug() << "[vid_out PROC] isOpened=" << vid_out_proc.isOpened();
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
    m_setup_overlay->data_exchange(false);
    update_delay();
    update_gate_width();
    update_laser_width();
    ui->MCP_SLIDER->setValue(std::round(m_device_mgr->tcu()->get(TCU::MCP)));
    on_SET_PARAMS_BUTTON_clicked();
}


void UserPanel::update_processing_params()
{
    QMutexLocker lk(&m_params_mutex);
    m_processing_params.brightness           = ui->BRIGHTNESS_SLIDER->value();
    m_processing_params.contrast             = ui->CONTRAST_SLIDER->value();
    m_processing_params.gamma_slider         = ui->GAMMA_SLIDER->value();
    m_processing_params.show_info            = ui->INFO_CHECK->isChecked();
    m_processing_params.show_center          = ui->CENTER_CHECK->isChecked();
    m_processing_params.select_tool          = ui->SELECT_TOOL->isChecked();
    m_processing_params.dual_display         = ui->DUAL_DISPLAY_CHK->isChecked();
    m_processing_params.mcp_slider_focused   = ui->MCP_SLIDER->hasFocus();
    m_processing_params.hist_display_size    = ui->HIST_DISPLAY->size();
    m_processing_params.split                = m_setup_overlay->split();
    m_processing_params.custom_info_text     = m_setup_overlay->w()->CUSTOM_INFO_EDT->text();
    m_processing_params.selection_v1         = m_router.display(DisplayId::Main).widget->selection_v1;
    m_processing_params.selection_v2         = m_router.display(DisplayId::Main).widget->selection_v2;
    m_processing_params.display_region       = m_router.display(DisplayId::Main).widget->display_region;
    m_processing_params.display_width        = m_router.display(DisplayId::Main).widget->width();
    m_processing_params.display_height       = m_router.display(DisplayId::Main).widget->height();

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
    m_pipeline_config.fishnet_model_idx   = data.image_proc.fishnet_model_idx;
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
    // YOLO model_idx is per-display (lives on each DisplaySlot's PipelineConfig);
    // populated from the model-list comboboxes, not the config file.
    m_pipeline_config.visible_model_path  = data.yolo.visible_model_path;
    m_pipeline_config.visible_classes_file = data.yolo.visible_classes_file;
    m_pipeline_config.thermal_model_path  = data.yolo.thermal_model_path;
    m_pipeline_config.thermal_classes_file = data.yolo.thermal_classes_file;
    m_pipeline_config.gated_model_path    = data.yolo.gated_model_path;
    m_pipeline_config.gated_classes_file  = data.yolo.gated_classes_file;

    // Push the Config-sourced fields into each DisplaySlot's per-display
    // pcfg, but preserve fields that are session-only / driven per-display
    // by the GENERAL-tab picker: YOLO model_idx and the picker-controlled
    // GENERAL toggles (frame-avg / 3D / enhance / pseudocolor + options).
    //
    // lower_3d_thresh is special: only Main's value round-trips to Config
    // (3D only runs on Main, so Alt1/Alt2's threshold is functionally
    // unused). Main takes the freshly Config-derived value; Alt1/Alt2
    // preserve their session-local copy.
    for (std::size_t i = 0; i < kDisplayCount; ++i) {
        DisplayId   did = static_cast<DisplayId>(i);
        DisplaySlot& d  = m_router.display(did);
        int    saved_model     = d.pcfg.model_idx;
        bool   saved_fa_on     = d.pcfg.frame_avg_enabled;
        int    saved_fa_mode   = d.pcfg.frame_avg_mode;
        bool   saved_3d_on     = d.pcfg.enable_3d;
        bool   saved_3d_layout = d.pcfg.image_3d;
        bool   saved_enh_on    = d.pcfg.enhance_enabled;
        int    saved_enh_opt   = d.pcfg.enhance_option;
        bool   saved_pc_on     = d.pcfg.pseudocolor_enabled;
        bool   saved_pc_layout = d.pcfg.pseudocolor;
        double saved_thresh    = d.pcfg.lower_3d_thresh;
        d.pcfg = m_pipeline_config;
        d.pcfg.model_idx           = saved_model;
        d.pcfg.frame_avg_enabled   = saved_fa_on;
        d.pcfg.frame_avg_mode      = saved_fa_mode;
        d.pcfg.enable_3d           = saved_3d_on;
        d.pcfg.image_3d            = saved_3d_layout;
        d.pcfg.enhance_enabled     = saved_enh_on;
        d.pcfg.enhance_option      = saved_enh_opt;
        d.pcfg.pseudocolor_enabled = saved_pc_on;
        d.pcfg.pseudocolor         = saved_pc_layout;
        if (did != DisplayId::Main) d.pcfg.lower_3d_thresh = saved_thresh;
        // Main keeps the Config-derived value (already copied via d.pcfg = m_pipeline_config).
    }
}

void UserPanel::apply_picked_pcfg_from_widgets()
{
    QMutexLocker lk(&m_params_mutex);
    PipelineConfig& pcfg = m_router.display(m_picked_display).pcfg;
    pcfg.frame_avg_enabled   = ui->FRAME_AVG_CHECK->isChecked();
    pcfg.frame_avg_mode      = ui->FRAME_AVG_OPTIONS->currentIndex();
    pcfg.enable_3d           = ui->IMG_3D_CHECK->isChecked();
    pcfg.image_3d            = ui->IMG_3D_CHECK->isChecked();
    pcfg.enhance_enabled     = ui->IMG_ENHANCE_CHECK->isChecked();
    pcfg.enhance_option      = ui->ENHANCE_OPTIONS->currentIndex();
    pcfg.pseudocolor_enabled = ui->PSEUDOCOLOR_CHK->isChecked();
    pcfg.pseudocolor         = ui->PSEUDOCOLOR_CHK->isChecked();
}

void UserPanel::repopulate_general_widgets_from_picked()
{
    PipelineConfig& pcfg = m_router.display(m_picked_display).pcfg;
    // Block widget signals while we mirror the picked display's state into
    // the UI — otherwise the stateChanged / currentIndexChanged handlers
    // would fire and write the OLD widget values back into the freshly
    // picked slot, corrupting it on every switch.
    QSignalBlocker b1(ui->FRAME_AVG_CHECK);
    QSignalBlocker b2(ui->FRAME_AVG_OPTIONS);
    QSignalBlocker b3(ui->IMG_3D_CHECK);
    QSignalBlocker b4(ui->IMG_ENHANCE_CHECK);
    QSignalBlocker b5(ui->ENHANCE_OPTIONS);
    QSignalBlocker b6(ui->PSEUDOCOLOR_CHK);
    QSignalBlocker b7(ui->RANGE_THRESH_EDIT);
    ui->FRAME_AVG_CHECK->setChecked(pcfg.frame_avg_enabled);
    ui->FRAME_AVG_OPTIONS->setCurrentIndex(pcfg.frame_avg_mode);
    ui->IMG_3D_CHECK->setChecked(pcfg.enable_3d);
    ui->IMG_ENHANCE_CHECK->setChecked(pcfg.enhance_enabled);
    ui->ENHANCE_OPTIONS->setCurrentIndex(pcfg.enhance_option);
    ui->PSEUDOCOLOR_CHK->setChecked(pcfg.pseudocolor_enabled);
    const int pd = m_router.stream(StreamId::S0).meta.pixel_depth;
    if (pd > 0) ui->RANGE_THRESH_EDIT->setText(QString::number(static_cast<int>(pcfg.lower_3d_thresh * (1 << pd))));
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
    if (m_setup_overlay->w()->TCP_SERVER_IP_EDIT) {
        data.network.tcp_server_address = m_setup_overlay->w()->TCP_SERVER_IP_EDIT->text();
    }

    // Sync UI settings
    data.ui.simplified = simple_ui;
    data.ui.dark_theme = !app_theme; // app_theme seems to be inverted
    data.ui.english = (lang == 0);

    // Sync camera settings
    data.camera.continuous_mode = !trigger_mode_on;
    data.camera.frequency = static_cast<int>(m_router.stream(StreamId::S0).meta.fps);
    data.camera.time_exposure = time_exposure_edit;
    data.camera.gain = gain_analog_edit;

    // Read pending Preferences UI changes (text fields) into Config
    m_setup_overlay->data_exchange(true);
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
    if (m_setup_overlay->w()->TCP_SERVER_IP_EDIT) {
        m_setup_overlay->w()->TCP_SERVER_IP_EDIT->setText(data.network.tcp_server_address);
    }
    m_device_mgr->udpptz()->set_target(QHostAddress(data.network.udp_target_ip), data.network.udp_target_port);
    m_device_mgr->udpptz()->set_listen(QHostAddress(data.network.udp_listen_ip), data.network.udp_listen_port);
    m_device_mgr->udpptz()->set_protocol(static_cast<UDPPTZ::Protocol>(data.network.udpptz_protocol));

    // Sync UI settings
    bool simplify_ui = data.ui.simplified;
    if (simple_ui != simplify_ui) switch_ui();
    bool new_theme = data.ui.dark_theme ? 0 : 1;
    if (new_theme != app_theme) set_theme();
    lang = data.ui.english ? 0 : 1;
    if (!data.ui.english) switch_language();

    // Sync camera settings
    trigger_mode_on = !data.camera.continuous_mode;
    m_router.stream(StreamId::S0).meta.fps = static_cast<float>(data.camera.frequency);
    time_exposure_edit = data.camera.time_exposure;
    gain_analog_edit = data.camera.gain;

    // TCU, device, save, and image_proc widgets are now populated by m_setup_overlay->data_exchange(false)
    data_exchange(false);
    m_setup_overlay->data_exchange(false);
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
                        [](cv::Mat &frame, void* ptr){ ((BoundedQueue<cv::Mat>*)ptr)->push(frame.clone()); },
            &m_router.stream(static_cast<StreamId>(display_list->currentIndex())).frames, display_list->currentIndex());
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
    static cv::Mat filter = m_router.stream(StreamId::S0).latest_raw.clone();
    filter = m_router.stream(StreamId::S0).latest_raw / 64;
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
        // Read from ScanController::params() — the single source of truth.
        // Widget edits propagate into m_params via ScanOverlay's slots.
        const ScanParams& sp = m_scan_ctrl ? m_scan_ctrl->params() : ScanParams{};
        out << sp.starting_delay << sp.ending_delay << sp.starting_gw << sp.ending_gw
            << sp.frame_count << sp.step_size_delay << sp.step_size_gw << sp.rep_freq
            << (int)(sp.capture_scan_ori) << (int)(sp.capture_scan_res)
            << (int)(sp.record_scan_ori) << (int)(sp.record_scan_res);
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
        // Deserialize into ScanController::params() (canonical store);
        // ScanOverlay::loadFromParams() below pushes the values to widgets.
        ScanParams& sp = m_scan_ctrl->params();
        out >> sp.starting_delay >> sp.ending_delay >> sp.starting_gw >> sp.ending_gw
            >> sp.frame_count >> sp.step_size_delay >> sp.step_size_gw >> sp.rep_freq;
        out >> temp_bool; sp.capture_scan_ori = temp_bool;
        out >> temp_bool; sp.capture_scan_res = temp_bool;
        out >> temp_bool; sp.record_scan_ori  = temp_bool;
        out >> temp_bool; sp.record_scan_res  = temp_bool;
        // Push the freshly-deserialized params into the SCAN overlay widgets.
        if (m_scan_overlay) m_scan_overlay->loadFromParams();
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
    // m_router.display(m_picked_display).pcfg.image_3d = ui->IMG_3D_CHECK->isChecked();

    {
        QMutexLocker pl(&m_router.display(m_picked_display).paint_mutex);
        m_router.display(m_picked_display).pcfg.image_3d  = arg1;
        m_router.display(m_picked_display).pcfg.enable_3d = arg1;
    }
    if (!arg1) m_tcu_ctrl->set_frame_a_3d(false);
    update_processing_params();

    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);

    if (simple_ui) {
        emit send_double_tcu_msg(TCU::DELAY_N, arg1 ? m_tcu_ctrl->get_depth_of_view() / m_tcu_ctrl->get_dist_ns() : 0);
        if (m_setup_overlay->w()->AUTO_MCP_CHK->isChecked()) m_setup_overlay->w()->AUTO_MCP_CHK->click();
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
    if (!curr_cam) return;
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
    // Deliberately NOT reading back here. The MV SDK / vendor serial paths
    // sometimes return 0 on read (uninitialized MVCC_FLOATVALUE on failure,
    // unsupported feature, timing). Reading into gain_analog_edit then
    // calling setValue(0) re-fires valueChanged → re-enters change_gain(0)
    // and the slider stuck at 0 forever. The user-requested value is already
    // clamped to the device's range above, so trust it for the UI.
    ui->GAIN_EDIT->setText(QString::number(val));
    // slider is already at `val` (it triggered this slot via valueChanged),
    // so no setValue needed — and skipping it removes the recursion risk.
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
            else if (edit == ui->IRIS_EDIT) {
                set_iris();
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
            // SETUP overlay shown via TopBar mode switch
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
                m_device_mgr->disconnect_from_serial_server_tcp() : m_device_mgr->connect_to_serial_server_tcp(m_setup_overlay->w()->TCP_SERVER_IP_EDIT->text());
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
    switch (m_processing_params.image_rotate) {
    case 1:
    case 3:
        ww = m_router.stream(StreamId::S0).meta.height; hh = m_router.stream(StreamId::S0).meta.width; break;
    case 0:
    case 2:
    default:
        ww = m_router.stream(StreamId::S0).meta.width; hh = m_router.stream(StreamId::S0).meta.height; break;
    }
    // Before any source loads, S0 meta width/height are 0. Fall back to a
    // 16:9 placeholder so the resize math (width*hh/ww) doesn't divide by
    // zero. As soon as a source's meta is populated the next resizeEvent
    // uses the real aspect.
    if (ww <= 0 || hh <= 0) { ww = 16; hh = 9; }
    // resizeEvent sizes the Main display widget; read Main's 3D state, not
    // the picker's (which may currently show Alt1/Alt2).
    ww = m_router.display(DisplayId::Main).pcfg.image_3d ? ww + 104 : ww;
    QRect window = this->geometry();
//    qDebug() << window;
    // Sprint 18: ui->LEFT is permanently hidden; left zone now belongs to
    // SideDisplayWidget (top) + MapboxWidget (bottom). Right zone is the
    // RightStackContainer.
    const int left_zone_w  = 220;
    const int right_zone_w = 220;
    const int title_h      = 40;
    const int side_top     = title_h + 4;
    const int pbar_h       = 26;
    const int right_top    = side_top + pbar_h + 4;     // pbar sits above right-stack
    const int side_bottom  = window.height() - 4;
    const int side_h       = side_bottom - side_top;
    const int right_h      = side_bottom - right_top;
    if (m_side_display) m_side_display->setGeometry(6,                                     side_top,                       left_zone_w,  side_h / 2 - 4);
    if (m_mapbox)       m_mapbox      ->setGeometry(6,                                     side_top + side_h / 2 + 4,      left_zone_w,  side_h / 2 - 8);
    if (m_right_stack)  m_right_stack ->setGeometry(window.width() - right_zone_w - 6,     right_top,                      right_zone_w, right_h);
    if (m_setup_overlay && m_setup_overlay->isVisible()) {
        const QRect c = ui->CENTRAL->rect();
        m_setup_overlay->setGeometry(c.x(), 40, c.width(), c.height() - 40);
    }
    if (m_scan_overlay && m_scan_overlay->isVisible()) {
        const QRect c = ui->CENTRAL->rect();
        m_scan_overlay->setGeometry(c.x(), 40, c.width(), c.height() - 40);
    }

    QRect region = ui->SOURCE_DISPLAY->geometry();
    QPoint center = ui->SOURCE_DISPLAY->center;

    int mid_width = window.width() - (right_zone_w + 12) - (left_zone_w + 12);
    ui->MID->setGeometry(left_zone_w + 12, title_h, mid_width, window.height() - title_h);
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
    // center toolbar: top-center of display region
    m_center_toolbar->adjustSize();
    m_center_toolbar->move(region.x() + (region.width() - m_center_toolbar->width()) / 2, 4);
    m_center_toolbar->raise();
    m_center_toolbar->show();
//    ui->CURR_COORD->move(region.x() + 80, 5);
    ui->START_COORD->move(region.x() + 80, 5);
    ui->SHAPE_INFO->move(ui->START_COORD->geometry().right() + 20, 5);
    ui->SHIFT_INFO->move(ui->SHAPE_INFO->geometry().right() + 20, 5);

    // INFO_CHECK is now inside m_center_toolbar — no separate move needed
    ui->CENTER_CHECK->move(region.right() - 60, 4);

    // DISTANCE: top-right of the source-display region (TCU-driven value via
    // TCUController::distance_text_updated wired at line 1423).
    ui->DISTANCE->adjustSize();
    ui->DISTANCE->move(region.right() - ui->DISTANCE->width() - 10, 5);

    // panel toggle bar: tab-row, aligned with right-stack column at side_top.
    // Width matches right-stack so the 3 toggle buttons spread evenly across
    // the same column instead of floating with content-only width.
    if (m_pbar) {
        m_pbar->setGeometry(window.width() - right_zone_w - 6,
                            side_top,
                            right_zone_w,
                            pbar_h);
        m_pbar->raise();
        m_pbar->show();
    }

    // FPS overlay (main): bottom-left of display, in CENTRAL coords — hidden in overlay modes
    const bool any_overlay = (m_setup_overlay && m_setup_overlay->isVisible())
                          || (m_scan_overlay  && m_scan_overlay ->isVisible());
    // Alt FPS labels are children of displays[1/2]; their visibility follows
    // the parent FloatingWindow visibility automatically.
    // Floating panels live in CENTRAL; raise above the fps label to keep
    // their headers clickable. Anchor them per prototype v3:
    //   Optics + TCU → bottom-left  (Optics at x=10, TCU just to its right)
    //   Console      → bottom-right (right-aligned in CENTRAL)
    if (!any_overlay) {
        const int central_w = ui->CENTRAL->width();
        const int central_h = ui->CENTRAL->height();
        const int bottom_pad = 10;
        const int left_pad   = 10;
        const int right_pad  = 10;
        const int panel_gap  = 12;  // gap between Optics and TCU
        if (m_optics_panel) {
            const int y = central_h - m_optics_panel->height() - bottom_pad;
            m_optics_panel->move(left_pad, y);
            m_optics_panel->raise();
        }
        if (m_tcu_panel) {
            const int x = (m_optics_panel ? m_optics_panel->x() + m_optics_panel->width() + panel_gap
                                          : left_pad);
            const int y = central_h - m_tcu_panel->height() - bottom_pad;
            m_tcu_panel->move(x, y);
            m_tcu_panel->raise();
        }
        if (m_console_panel) {
            const int x = central_w - m_console_panel->width() - right_pad;
            const int y = central_h - m_console_panel->height() - bottom_pad;
            m_console_panel->move(x, y);
            m_console_panel->raise();
        }
    }
    ui->HIDE_BTN->move(ui->MID->geometry().left() - 8, this->geometry().height() / 2 - 10);
//    ui->HIDE_BTN->move(ui->LEFT->geometry().right() - 10 + (ui->SOURCE_DISPLAY->geometry().left() + ui->MID->geometry().left() - ui->LEFT->geometry().right() + 10) / 2 + 2, this->geometry().height() / 2 - 10);
    ui->RULER_H->setGeometry(region.left(), region.bottom() - 10, region.width(), 32);
    ui->RULER_V->setGeometry(region.right() - 10, region.top(), 32, region.height());
#ifdef LVTONG
    ui->FISHNET_RESULT->move(region.right() - 170, 5);
#endif

    m_router.display(DisplayId::Main).paint_mutex.lock();
    ui->SOURCE_DISPLAY->setGeometry(region);
    ui->SOURCE_DISPLAY->update_roi(center);
    position_display_overlays(DisplayId::Main);
    if (QLabel* fps = m_router.display(DisplayId::Main).fps_label) fps->setVisible(!any_overlay);
    if (QLabel* sd = m_router.display(DisplayId::Main).source_dead_label)
        sd->setVisible(!any_overlay && m_router.display(DisplayId::Main).source_dead);
    qInfo("display region x: %d, y: %d, w: %d, h: %d", region.x(), region.y(), region.width(), region.height());
    m_router.display(DisplayId::Main).paint_mutex.unlock();

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
            ui->TITLE->cancel_drag();
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
            ui->TITLE->cancel_drag();
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
            load_video_file(current_video_filename = file_name, false, [](cv::Mat &frame, void* ptr){ ((BoundedQueue<cv::Mat>*)ptr)->push(frame.clone()); }, &(this->m_router.stream(StreamId::S0).frames), 0);
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
    // First file in a multi-drop sweep tears down any active producer on
    // Main so the static image isn't interleaved with leftover stream
    // frames. Subsequent files in the same drop (init=false) just append.
    if (init) stop_all_sources_on_slot(0);
    if (init) m_router.display(DisplayId::Main).consumer_run = false;
    if (!m_router.display(DisplayId::Main).setup_mutex.tryLock(1e3)) return false;

    QImage qimage_temp;
    cv::Mat mat_temp;
    bool tiff = false;

    if (filename.endsWith(".tif") || filename.endsWith(".tiff")) tiff = true;

    if (tiff) {
        if (ImageIO::load_image_tif(mat_temp, filename)) { m_router.display(DisplayId::Main).setup_mutex.unlock(); return false; }
    }
    else {
        if (!qimage_temp.load(filename)) { m_router.display(DisplayId::Main).setup_mutex.unlock(); return false; }
    }

    if (device_on) {
        QMessageBox::warning(this, "PROMPT",
            tr("Shut down the camera or swap it to an alt display before loading a new source on Main."));
        m_router.display(DisplayId::Main).setup_mutex.unlock();
        return true;
    }

    if (init) {
        if (tiff) start_static_display(mat_temp.cols, mat_temp.rows, false, 0, 8 * (mat_temp.depth() / 2 + 1));
        else      start_static_display(qimage_temp.width(), qimage_temp.height(), qimage_temp.format() != QImage::Format_Indexed8);
    }
    qimage_temp = qimage_temp.convertToFormat(m_router.stream(StreamId::S0).meta.is_color ? QImage::Format_RGB888 : QImage::Format_Indexed8);

    if (tiff) m_router.stream(StreamId::S0).frames.push(mat_temp.clone());
    else      m_router.stream(StreamId::S0).frames.push(cv::Mat(m_router.stream(StreamId::S0).meta.height, m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.is_color ? CV_8UC3 : CV_8UC1, (uchar*)qimage_temp.bits(), qimage_temp.bytesPerLine()).clone());

    m_router.display(DisplayId::Main).setup_mutex.unlock();
    return true;
}

void init_filter_graph(AVFormatContext *format_context, int video_stream_idx, AVFilterGraph *filter_graph, AVCodecContext *codec_context,
                       AVFilterContext **buffersink_ctx, AVFilterContext **buffersrc_ctx) {
    char args[512];
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    // Guards cover every early-return path below. avfilter_inout_free is a no-op on NULL,
    // so it's safe if avfilter_graph_parse_ptr already consumed the lists.
    std::shared_ptr<AVFilterInOut*> deleter_outputs(&outputs, avfilter_inout_free);
    std::shared_ptr<AVFilterInOut*> deleter_inputs(&inputs, avfilter_inout_free);

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
}

static int ffmpeg_interrupt_callback(void *p) {
    time_t *t0 = (time_t *)p;
    if (*t0 > 0 && time(NULL) - *t0 > 30) return 1;
    return 0;
}

// TODO: release ptrs after exited half-way
int UserPanel::load_video_file(QString filename, bool format_gray, void (*process_frame)(cv::Mat &, void *), void *ptr, int display_idx, bool display)
{
    // Industrial cam on Main is sticky — user must either shut it down or
    // swap it to an Alt display before loading a different source onto
    // Main. Guard on `device_on` (cam initialized) to match load_image_file.
    if (display && display_idx == 0 && device_on) {
        QMessageBox::warning(this, "PROMPT",
            tr("Shut down the camera or swap it to an alt display before loading a new source on Main."));
        return -1;
    }
    // Stop any in-flight stream source on this slot before dispatching. All
    // stream source types push to the same StreamSlot::frames queue; without
    // an explicit tear-down, the prior source's producer keeps pushing
    // while the new one also pushes and the consumer pops them interleaved.
    if (display) stop_all_sources_on_slot(display_idx);

    if (filename.startsWith("cv:") || filename.startsWith("CV:")) {
        // Capture a fresh keep-alive flag for this cv:// thread. The flag
        // outlives the parent slot variable: load_video_file may reassign
        // m_cv_run[display_idx] to a new shared_ptr (next load), but this
        // thread keeps reading its own captured copy until the parent flips
        // it to false from outside (via the top-of-load_video_file tear-down).
        std::shared_ptr<std::atomic<bool>> keep_alive;
        if (display && display_idx >= 0 && display_idx < kStreamCount) {
            keep_alive = std::make_shared<std::atomic<bool>>(true);
            m_cv_run[display_idx] = keep_alive;
        }
        std::thread t([this, filename, process_frame, ptr, display_idx, display, keep_alive](){
            if (current_video_filename != filename) current_video_filename = "";
            if (display) {
                m_router.display(static_cast<DisplayId>(display_idx)).consumer_run = false;
                if (!m_router.display(static_cast<DisplayId>(display_idx)).setup_mutex.tryLock(1e3)) return -1;
            }

            cv::VideoCapture vin;
            bool convert_to_number = false;
            int device_idx = filename.right(filename.length() - 3).toUInt(&convert_to_number);
            if (convert_to_number) vin.open(device_idx, cv::CAP_DSHOW);
            else                   vin.open(filename.right(filename.length() - 3).toLatin1().constData());
            if (!vin.isOpened()) { if (display) m_router.display(static_cast<DisplayId>(display_idx)).setup_mutex.unlock(); return -2; }

            if (convert_to_number) {
                vin.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
                vin.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
//                vin.set(cv::CAP_PROP_FPS, 10);
            }
            cv::Mat frame;
            vin >> frame;
            if (display) start_static_display(frame.cols, frame.rows, frame.channels() == 3, display_idx, 8, -2);
//            if (display) start_static_display(1920, 1080, frame.channels() == 3, display_idx, 8, -2);

            // Display-bound capture pushes through the routed StreamSlot's
            // sink so the consumer thread picks frames up like any other
            // source. Display-less (export) capture keeps the legacy
            // process_frame callback for the offline conversion pipeline.
            StreamFrameSink sink = display ? m_router.stream(static_cast<StreamId>(display_idx)).sink()
                                           : StreamFrameSink{};

            // Display-bound loop checks its own keep_alive flag (so a new
            // load can reliably kill this thread). Display-less (export)
            // loop has no keep_alive; falls back to consumer_run for
            // backward compatibility with the offline conversion pipeline.
            const bool has_flag = (bool)keep_alive;
            while (has_flag ? keep_alive->load() : m_router.display(static_cast<DisplayId>(display_idx)).consumer_run.load()) {
                vin >> frame;
                if (frame.empty()) { QThread::msleep(5); continue; }
                cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
                if (display && sink.frames) sink.frames->push(frame.clone());
                else if (process_frame)     (*process_frame)(frame, ptr);
            }

            if (display) m_router.display(static_cast<DisplayId>(display_idx)).setup_mutex.unlock();

            if (!display) this->video_stopped();
        });
        t.detach();
    }
    else if (display) {
        // Live FFmpeg playback — delegated to FfmpegSource. start() stops any
        // previous run on the same StreamId, opens the source on a worker
        // thread, and emits opened/frame_decoded/failed/finished signals
        // back on the GUI thread.
        if (display_idx >= 0 && display_idx < kStreamCount && m_ffmpeg[display_idx]) {
            m_ffmpeg[display_idx]->start(static_cast<StreamId>(display_idx), filename, format_gray);
        }
        return 0;
    }
    else {
        // Export path: legacy lambda still in use. Carries process_frame for
        // the offline enhancement → vid_out_proc write workflow.
        std::thread t([this, filename, format_gray, process_frame, ptr, display_idx, display](){
            if (current_video_filename != filename) current_video_filename = "";

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
            // Address-of pattern: deleter reads the current value at scope exit.
            // The value form (buffer, av_free) would capture the NULL we have now and leak the later av_malloc.
            std::shared_ptr<uint8_t*> deleter_buffer(&buffer, [](uint8_t **p){ av_free(*p); });
            SwsContext *sws_context = NULL;
            std::shared_ptr<SwsContext*> deleter_sws_context(&sws_context, [](SwsContext **p){ sws_freeContext(*p); });
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
                if (display) m_router.display(static_cast<DisplayId>(display_idx)).setup_mutex.unlock();
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
                // Latency tuning for live RTSP. Defaults are tuned for VOD
                // (analyzeduration 5 s, probesize 5 MB) and produce ~2 s of
                // startup lag plus a deep reorder buffer that makes bursty
                // playback. These shrink the input pipeline so frames hit
                // the decoder as fast as the network delivers them.
                av_dict_set(&format_opts, "buffer_size",      "262144",  0);  // 256 KB
                av_dict_set(&format_opts, "fflags",           "nobuffer", 0);
                av_dict_set(&format_opts, "max_delay",        "200000",  0);  // 200 ms
                av_dict_set(&format_opts, "analyzeduration",  "500000",  0);  // 500 ms
                av_dict_set(&format_opts, "probesize",        "100000",  0);  // 100 KB
            }
            else if (is_http_stream) {
                // HLS / DASH / progressive HTTP. Without these the demuxer
                // serially fetches each .ts segment after the previous one
                // is fully consumed, producing a visible pause at every
                // segment boundary. PotPlayer overlaps fetch with playback;
                // these options make FFmpeg do the same.
                //   http_persistent = reuse TCP/TLS connection across segments
                //     (avoids per-segment TLS handshake on HTTPS).
                //   http_multiple   = allow next segment to download in
                //     parallel with current segment's playback.
                //   seg_max_retry   = transient 5xx / TCP resets shouldn't
                //     abort the stream.
                av_dict_set(&format_opts, "http_persistent", "1", 0);
                av_dict_set(&format_opts, "http_multiple",   "1", 0);
                av_dict_set(&format_opts, "seg_max_retry",   "3", 0);
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

            if (display) {
                // avg_frame_rate is the actual playback rate (computed by
                // FFmpeg from observed packet timing during the probe window);
                // r_frame_rate is the base/raw rate derived from container
                // hints. Prefer avg, fall back to r only if avg is empty.
                //
                // Failure case this guard exists for: RTSP streams under the
                // short probe configured above (analyzeduration=500ms,
                // probesize=100KB). 500 ms isn't long enough for FFmpeg to
                // populate avg_frame_rate, so avg stays 0/0 → falls back to
                // r_frame_rate, which for any RTP stream defaults to 90000/1
                // (the RTP 90 kHz clock per RFC 3551, used as a sentinel when
                // SDP carries no a=framerate line). Writing 90000 into
                // frame_rate_edit poisons the next VideoWriter::open() for
                // this display — H.264 rejects fps=90000 and the recording
                // produces a 0-byte file. Reject implausibly-high values and
                // keep the previously-set fps for this display (which on the
                // main display is driven by CCD_FREQ_EDIT — the user-entered
                // camera rate, which is the preferred source for real-time
                // streaming).
                //
                // TODO: short probe + late fps refresh. Keep the fast 500ms
                // probe for low startup lag, then re-read avg_frame_rate from
                // the stream after ~50 decoded packets — by that point FFmpeg
                // will have populated it. Would give automatic fps detection
                // (matching `ffmpeg -i` CLI behavior) without sacrificing the
                // sub-second connect time. Not implemented; falling back to
                // the user-entered CCD_FREQ_EDIT value is the deliberate
                // choice for now.
                AVRational fr = format_context->streams[video_stream_idx]->avg_frame_rate;
                if (fr.num == 0 || fr.den == 0)
                    fr = format_context->streams[video_stream_idx]->r_frame_rate;
                const double fps = av_q2d(fr);
                if (fps > 0.0 && fps <= 1000.0) {
                    m_router.stream(static_cast<StreamId>(display_idx)).meta.fps = std::round(fps);
                    // Only the main display drives the camera-sync TCU register.
                    if (display_idx == 0)
                        emit send_double_tcu_msg(TCU::CCD_FREQ, m_router.stream(StreamId::S0).meta.fps);
                }
                else {
                    qDebug() << "[playback] rejecting bogus fps" << fps
                             << "(avg=" << format_context->streams[video_stream_idx]->avg_frame_rate.num
                             << "/"     << format_context->streams[video_stream_idx]->avg_frame_rate.den
                             << "r="    << format_context->streams[video_stream_idx]->r_frame_rate.num
                             << "/"     << format_context->streams[video_stream_idx]->r_frame_rate.den
                             << ") — keeping previous fps=" << m_router.stream(static_cast<StreamId>(display_idx)).meta.fps;
                }
            }

            codec = avcodec_find_decoder(codec_param->codec_id);
            if (codec == NULL) return cleanup_and_return(-3);

            if (avcodec_parameters_to_context(codec_context, codec_param) < 0) return cleanup_and_return(-3);

            // try HW-accelerated decoding (D3D11VA), fall back to multi-threaded SW.
            // Streams (RTSP/HTTP) deliberately skip HW: on some machines the
            // GPU→CPU readback via av_hwframe_transfer_data is slower than SW
            // decode, causing the HW surface pool to back up and the playback
            // to burst-then-stall ("PowerPoint" effect). Local files keep HW
            // because the PTS throttle paces the loop and the pool drains.
            AVBufferRef *hw_device_ctx = nullptr;
            bool hw_decoding = false;
            if (is_local_file &&
                av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) == 0) {
                codec_context->hw_device_ctx = av_buffer_ref(hw_device_ctx);
                hw_decoding = true;
                qDebug() << "FFmpeg: using D3D11VA hardware decoding (local file)";
            } else {
                qDebug() << "FFmpeg: using SW decoding ("
                         << (is_local_file ? "HW unavailable" : "stream") << ")";
            }
            if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);

            codec_context->thread_count = 0;
            codec_context->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
            // For network streams, tell the decoder not to wait for B-frame
            // reorder (output in decode order) and enable fast decode paths.
            // Without LOW_DELAY the decoder buffers a full reorder window
            // before emitting the first frame, which compounds the input-side
            // analyzeduration delay.
            if (is_rtsp_stream || is_http_stream) {
                codec_context->flags  |= AV_CODEC_FLAG_LOW_DELAY;
                codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
            }

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
                    if (is_rtsp_stream || is_http_stream) {
                        codec_context->flags  |= AV_CODEC_FLAG_LOW_DELAY;
                        codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
                    }
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
            while (m_router.display(static_cast<DisplayId>(display_idx)).consumer_run) {
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
                    // Drain every frame the decoder produced for this packet.
                    // B-frames + threaded decode buffer multiple outputs per send;
                    // a single receive leaves frames queued and accumulates latency.
                    bool stream_done = false;
                    while (avcodec_receive_frame(codec_context, frame) == 0) {
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
                            if (av_buffersrc_add_frame_flags(buffersrc_ctx, sw_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) { stream_done = true; break; }

                            // pull filtered frames from the filtergraph
                            av_frame_unref(frame_filter);
                            int ret = av_buffersink_get_frame(buffersink_ctx, frame_filter);
                            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { stream_done = true; break; }
                        }
                        else {
                            // Use frame->pts (decoder output) not sw_frame->pts:
                            // av_hwframe_transfer_data does not copy metadata,
                            // so under HW decoding sw_frame->pts is unset (0),
                            // making the throttle a no-op and the video play at
                            // decode rate (~2× source).
                            // av_rescale_q returns µs (target = {1,AV_TIME_BASE});
                            // nsecsElapsed/1000 = µs to match.
                            if (display && frame->pts != AV_NOPTS_VALUE) {
                                if (last_pts != AV_NOPTS_VALUE) {
                                    delay = av_rescale_q(frame->pts - last_pts, time_base, time_base_q) - elapsed_timer.nsecsElapsed() / 1000;
                                    if (delay > 0 && delay < 1000000) av_usleep(delay);
                                    elapsed_timer.restart();
                                }
                                last_pts = frame->pts;
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
                    if (stream_done) break;
                }
            }

            if (display) m_router.display(static_cast<DisplayId>(display_idx)).setup_mutex.unlock();

            if (!display) this->video_stopped();
            return 0;
        });
        t.detach();
    }

    return 0;
}

void UserPanel::update_pixel_depth(int depth, int display_idx)
{
    m_router.stream(static_cast<StreamId>(display_idx)).meta.pixel_depth = depth;
    ui->TITLE->set_pixel_depth(depth);
}

// Guard: drop events whose session != source's current session. This shuts
// down the "stale events still queued in the GUI loop after stop()" path
// that lets a freshly-loaded source race with the leftovers of the previous
// one.
static inline bool ffmpeg_session_stale(FfmpegSource* src, int session) {
    return !src || src->current_session() != session;
}

void UserPanel::on_ffmpeg_opened(StreamId target, int session, FfmpegSource::VideoInfo info)
{
    if (target == StreamId::None) return;
    const int display_idx = static_cast<int>(target);
    if (ffmpeg_session_stale(m_ffmpeg[display_idx], session)) {
        qDebug() << "[FfmpegSource] dropping stale opened session=" << session
                 << "current=" << (m_ffmpeg[display_idx] ? m_ffmpeg[display_idx]->current_session() : -1);
        return;
    }
    // start_static_display tears down any previous consumer for this slot,
    // sets stream meta, creates a new consumer thread, and routes the slot.
    // device_type = -2 = NetworkStream (used for both file + network here).
    start_static_display(info.width, info.height, info.is_color, display_idx, 8, -2);
    if (info.fps > 0.0 && info.fps <= 1000.0) {
        m_router.stream(target).meta.fps = static_cast<float>(std::round(info.fps));
        if (display_idx == 0)
            emit send_double_tcu_msg(TCU::CCD_FREQ, m_router.stream(StreamId::S0).meta.fps);
    }
    m_ffmpeg[display_idx]->mark_display_ready(session);
}

void UserPanel::on_ffmpeg_frame(StreamId target, int session, cv::Mat frame)
{
    if (target == StreamId::None) return;
    const int display_idx = static_cast<int>(target);
    if (ffmpeg_session_stale(m_ffmpeg[display_idx], session)) return;
    m_router.stream(target).frames.push(frame);
}

void UserPanel::on_ffmpeg_failed(StreamId target, int session, QString reason)
{
    qDebug() << "[FfmpegSource] failed:" << static_cast<int>(target) << "session=" << session << reason;
    if (target == StreamId::None) return;
    const int display_idx = static_cast<int>(target);
    if (ffmpeg_session_stale(m_ffmpeg[display_idx], session)) return;
    set_source_dead_overlay(m_router.display_for(target), true);
    m_router.stream(target).frames.close();
    m_router.stream(target).frame_info.close();
}

void UserPanel::on_ffmpeg_finished(StreamId target, int session)
{
    qDebug() << "[FfmpegSource] finished:" << static_cast<int>(target) << "session=" << session;
    if (target == StreamId::None) return;
    const int display_idx = static_cast<int>(target);
    if (ffmpeg_session_stale(m_ffmpeg[display_idx], session)) return;
    set_source_dead_overlay(m_router.display_for(target), true);
    m_router.stream(target).frames.close();
    m_router.stream(target).frame_info.close();
}

void UserPanel::stop_all_sources_on_slot(int display_idx)
{
    if (display_idx < 0 || display_idx >= kStreamCount) return;
    if (m_ffmpeg[display_idx]) m_ffmpeg[display_idx]->stop();
    if (m_cv_run[display_idx]) m_cv_run[display_idx]->store(false);
    StreamId sid = static_cast<StreamId>(display_idx);
    if (m_router.stream(sid).meta.source_type != SourceType::IndustrialCam) {
        DisplayId did = m_router.display_for(sid);
        if (did != DisplayId::_Count) set_source_dead_overlay(did, true);
        m_router.stream(sid).frames.close();
        m_router.stream(sid).frame_info.close();
    }
    // Industrial cam is intentionally NOT auto-stopped from this helper.
    // Cam is the primary source on Main with its own UI surface and
    // recording state; silently tearing it down on a video load would be
    // surprising. Callers that load onto slot 0 should check
    // `start_grabbing` upfront and refuse with a warning so the user
    // explicitly stops the cam (or swaps it to Alt1/Alt2) before loading.
}


void UserPanel::rotate(int angle)
{
    m_router.display(DisplayId::Main).paint_mutex.lock();
    m_processing_params.image_rotate = angle;
    m_router.display(DisplayId::Main).paint_mutex.unlock();
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
    DisplayId did = static_cast<DisplayId>(display_idx);
    StreamId  sid = static_cast<StreamId>(display_idx);

    // Tear down any existing consumer + stream for this display slot.
    if (m_router.display(static_cast<DisplayId>(display_idx)).consumer) {
        m_router.display(static_cast<DisplayId>(display_idx)).consumer_run = false;
        m_router.display(static_cast<DisplayId>(display_idx)).consumer->quit();
        m_router.display(static_cast<DisplayId>(display_idx)).consumer->wait();
        delete m_router.display(static_cast<DisplayId>(display_idx)).consumer;
        m_router.display(static_cast<DisplayId>(display_idx)).consumer = NULL;
    }
    while (m_router.display(static_cast<DisplayId>(display_idx)).consumer_state.load()) QThread::msleep(1);
    m_router.stream(sid).frames.clear();
    m_router.stream(sid).frames.reopen();
    m_router.stream(sid).frame_info.clear();
    m_router.stream(sid).frame_info.reopen();
    m_router.unroute(sid);

    {
        QMutexLocker lk(&m_router.display(static_cast<DisplayId>(display_idx)).paint_mutex);
        m_router.stream(static_cast<StreamId>(display_idx)).meta.width = width;
        m_router.stream(static_cast<StreamId>(display_idx)).meta.height = height;
        if (display_idx == 0) ui->TITLE->set_resolution(width, height);
        this->m_router.stream(static_cast<StreamId>(display_idx)).meta.is_color = is_color;
        update_pixel_depth(pixel_depth, display_idx);
        if (display_idx == 0) this->device_type = device_type;
        // Map legacy device_type to per-stream SourceType.
        m_router.stream(sid).meta.source_type =
            device_type == 1  ? SourceType::IndustrialCam :
            device_type == -1 ? SourceType::LocalFile     :
            device_type == -2 ? SourceType::NetworkStream :
                                SourceType::None;
    }

    if (display_idx) fw_display[display_idx - 1]->resize_display(width, height);
    else {
        QResizeEvent e(this->size(), this->size());
        resizeEvent(&e);
    }
    set_source_dead_overlay(did, false);
    position_display_overlays(did);

    m_router.display(static_cast<DisplayId>(display_idx)).consumer_run = true;
    m_router.display(static_cast<DisplayId>(display_idx)).consumer = new ConsumerThread(m_pipeline, &m_router, &m_router.display(did));
    if (m_router.display(static_cast<DisplayId>(display_idx)).consumer == NULL) {
        m_router.display(static_cast<DisplayId>(display_idx)).consumer_run = false;
        QMessageBox::warning(this, "PROMPT", tr("Cannot display image"));
        return;
    }
    m_router.route(did, sid);
    m_router.stream(sid).producer_run = true;
    m_router.display(static_cast<DisplayId>(display_idx)).consumer->start();
    m_router.display(static_cast<DisplayId>(display_idx)).widget->is_grabbing = true;
    if (display_idx == 0) {
        start_grabbing = true;
        enable_controls(true);
        ui->ENUM_BUTTON->setEnabled(false);
        ui->START_BUTTON->setEnabled(false);
        ui->STOP_GRABBING_BUTTON->setEnabled(true);
    }
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
    DisplaySlot& d = m_router.display(DisplayId::Main);
    const StreamId sid = m_router.snapshot(DisplayId::Main).stream;
    const StreamMeta meta = (sid == StreamId::None) ? StreamMeta{} : m_router.stream(sid).meta;
    if (d.record_raw) {
//        curr_cam->stop_recording(0);
        QString raw_avi;
        {
            QMutexLocker lk(&d.paint_mutex);
            raw_avi = d.raw_path;
            d.raw_path.clear();
            d.raw_writer.release();
        }
        if (!raw_avi.isEmpty()) {
            QString dest = save_location + "/" + raw_avi.section('/', -1, -1);
            std::thread t(UserPanel::move_to_dest, QString(raw_avi), QString(dest));
            t.detach();
        }
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        d.paint_mutex.lock();
        QString raw_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_main_raw.mp4");
        d.raw_path = raw_avi;
//        m_router.display(DisplayId::Main).raw_writer.open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height), m_router.stream(StreamId::S0).meta.is_color);
        // Real-time fps: use the TimedQueue measured arrival rate over the 5 s
        // window; fall back to the commanded rate when the window is too
        // sparse to trust.
        const double measured_fps_ori = m_router.display(DisplayId::Main).fps_calc ? m_router.display(DisplayId::Main).fps_calc->count() / 5.0 : 0.0;
        const double vw_fps_ori = (measured_fps_ori >= 1.0) ? measured_fps_ori : meta.fps;
        qDebug() << "[vid_out ORI] open" << raw_avi
                 << "size=" << meta.width << "x" << meta.height
                 << "fps=" << vw_fps_ori
                 << "(measured=" << measured_fps_ori
                 << "commanded=" << meta.fps << ")"
                 << "color=" << meta.is_color;
        m_router.display(DisplayId::Main).raw_writer.open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), vw_fps_ori, cv::Size(meta.width, meta.height), meta.is_color);
        qDebug() << "[vid_out ORI] isOpened=" << m_router.display(DisplayId::Main).raw_writer.isOpened();
        m_router.display(DisplayId::Main).paint_mutex.unlock();
    }
    m_router.display(DisplayId::Main).record_raw = !m_router.display(DisplayId::Main).record_raw;
    ui->SAVE_AVI_BUTTON->setText(m_router.display(DisplayId::Main).record_raw ? tr("STOP") : tr("ORI"));
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
    if (device_type == -1 || !config->get_data().save.consecutive_capture) m_router.display(DisplayId::Main).save_proc = 1;
    else{
        m_router.display(DisplayId::Main).save_proc = !m_router.display(DisplayId::Main).save_proc;
        ui->SAVE_RESULT_BUTTON->setText(m_router.display(DisplayId::Main).save_proc ? tr("STOP") : tr("RES"));
        m_setup_overlay->w()->CONSECUTIVE_CAPTURE_CHK->setEnabled(!m_router.display(DisplayId::Main).save_proc);
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

void UserPanel::set_iris()
{
    m_lens_ctrl->set_iris_value(ui->IRIS_EDIT->text().toUInt());
}

void UserPanel::update_current()
{
    m_tcu_ctrl->update_current(ui->CURRENT_EDIT->text().toFloat());
}
void UserPanel::screenshot()
{
//    m_router.display(DisplayId::Main).paint_mutex.lock();
    QPixmap screenshot = window()->grab();
    screenshot.save(save_location + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    QApplication::clipboard()->setPixmap(screenshot);
//    m_router.display(DisplayId::Main).paint_mutex.unlock();
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
    temp_mvcam->frame_size(true, &m_router.stream(StreamId::S0).meta.width, &m_router.stream(StreamId::S0).meta.height);
    qInfo("frame w: %d, h: %d", m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height);
    ui->TITLE->set_resolution(m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height);
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
    if (!m_router.display(static_cast<DisplayId>(display_idx)).consumer_state) return;
    switch (cmd / 2) {
    case 0: {
//        if (m_router.display(static_cast<DisplayId>(display_idx)).record_raw) on_SAVE_AVI_BUTTON_clicked();
//        if (m_router.display(static_cast<DisplayId>(display_idx)).record_proc) on_SAVE_FINAL_BUTTON_clicked();

        m_router.display(static_cast<DisplayId>(display_idx)).consumer_run = false;
        if (m_router.display(static_cast<DisplayId>(display_idx)).consumer) {
            m_router.display(static_cast<DisplayId>(display_idx)).consumer->quit();
            m_router.display(static_cast<DisplayId>(display_idx)).consumer->wait();
            delete m_router.display(static_cast<DisplayId>(display_idx)).consumer;
            m_router.display(static_cast<DisplayId>(display_idx)).consumer = NULL;
        }
        QTimer::singleShot(100, m_router.display(static_cast<DisplayId>(display_idx)).widget, SLOT(clear()));
        set_source_dead_overlay(static_cast<DisplayId>(display_idx), true);

        break;
    }
    case 1: m_router.display(static_cast<DisplayId>(display_idx)).save_proc = 1; break;
    case 2: {
        DisplaySlot& d = m_router.display(static_cast<DisplayId>(display_idx));
        // Read metadata from the stream actually routed to this display, not
        // from StreamId(display_idx) — they diverge after a Router::swap.
        const StreamId sid = m_router.snapshot(static_cast<DisplayId>(display_idx)).stream;
        const StreamMeta meta = (sid == StreamId::None) ? StreamMeta{} : m_router.stream(sid).meta;
        if (d.record_proc) {
            QString res_avi;
            {
                QMutexLocker lk(&d.paint_mutex);
                res_avi = d.proc_path;
                d.proc_path.clear();
                d.proc_writer.release();
            }
            if (!res_avi.isEmpty()) {
                QString dest = save_location + "/" + res_avi.section('/', -1, -1);
                std::thread t(UserPanel::move_to_dest, QString(res_avi), QString(dest));
                t.detach();
            }
        }
        else {
            QString res_avi;
            {
                QMutexLocker lk(&d.paint_mutex);
                res_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_alt" + QString::number(display_idx) + "_res.mp4");
                d.proc_path = res_avi;
                const double measured_fps_alt = d.fps_calc ? d.fps_calc->count() / 5.0 : 0.0;
                const double vw_fps_alt = (measured_fps_alt >= 1.0) ? measured_fps_alt : meta.fps;
                qDebug() << "[vid_out ALT" << display_idx << "] open" << res_avi
                         << "size=" << meta.width << "x" << meta.height
                         << "fps=" << vw_fps_alt
                         << "(measured=" << measured_fps_alt
                         << "commanded=" << meta.fps << ")"
                         << "color=" << meta.is_color;
                d.proc_writer.open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), vw_fps_alt, cv::Size(meta.width, meta.height), meta.is_color);
                qDebug() << "[vid_out ALT" << display_idx << "] isOpened=" << d.proc_writer.isOpened();
            }
        }
        d.record_proc = !d.record_proc;
        alt_ctrl_grp->button(cmd)->setText(d.record_proc ? "STOP" : "REC");
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
        if (device_on && curr_cam) curr_cam->frame_size(true, &m_router.stream(StreamId::S0).meta.width, &m_router.stream(StreamId::S0).meta.height);
        ui->TITLE->set_resolution(m_router.stream(StreamId::S0).meta.width, m_router.stream(StreamId::S0).meta.height);
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
    {
        QMutexLocker pl(&m_router.display(m_picked_display).paint_mutex);
        m_router.display(m_picked_display).pcfg.pseudocolor         = arg1;
        m_router.display(m_picked_display).pcfg.pseudocolor_enabled = arg1;
    }
    update_processing_params();
}

bool UserPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        for (int i = 1; i < 3; i++) {
            if (obj == m_router.display(static_cast<DisplayId>(i)).widget) {
                position_display_overlays(static_cast<DisplayId>(i));
                if (QLabel* fps = m_router.display(static_cast<DisplayId>(i)).fps_label) fps->show();
                if (QLabel* sd = m_router.display(static_cast<DisplayId>(i)).source_dead_label)
                    sd->setVisible(m_router.display(static_cast<DisplayId>(i)).source_dead);
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void UserPanel::update_tcu_param_pos(int dir, QLabel *u_unit, QLineEdit *n_input,
                                     QLabel *n_unit, QLineEdit *p_input)
{
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
    }
}

