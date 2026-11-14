#include "topbar.h"

#include <QStyle>

namespace {
void repolish(QWidget *w)
{
    if (!w || !w->style()) return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}
}

TopButton::TopButton(QString icon, QWidget *parent)
    : QPushButton(QIcon(icon), "", parent) {}

void TopButton::mouseMoveEvent(QMouseEvent *event)
{
    setCursor(cursor_curr_pointer);
    QPushButton::mouseMoveEvent(event);
}

TopBar::TopBar(QWidget *parent) :
    QFrame(parent),
    m_addon(nullptr),
    m_url(nullptr),
    m_settings(nullptr),
    m_capture(nullptr),
    m_cls(nullptr),
    m_lang(nullptr),
    m_color_mode(nullptr),
    m_min(nullptr),
    m_max(nullptr),
    m_exit(nullptr),
    m_is_maximized(false),
    m_left_slot(nullptr),
    m_center_slot(nullptr),
    m_right_slot(nullptr),
    m_window_controls(nullptr),
    m_mode_group(nullptr),
    m_system_selector(nullptr),
    m_clock_timer(nullptr),
    m_clock_label(nullptr),
    m_current_mode(Mode::MONITOR),
    m_cam_dot(nullptr),
    m_tcu_dot(nullptr),
    m_lens_dot(nullptr),
    m_laser_dot(nullptr),
    m_ptz_dot(nullptr),
    m_pixel_depth_label(nullptr),
    m_resolution_label(nullptr),
    m_pressed(false),
    m_light_mode(false),
    m_signal_receiver(nullptr)
{
    setObjectName("TOP_BAR");

    auto *root = new QHBoxLayout(this);
    // Right margin 0 so EXIT_BTN touches the window's right edge: maximized, the
    // corner of the screen is then the button.
    root->setContentsMargins(8, 0, 0, 0);
    root->setSpacing(8);

    m_left_slot   = new QFrame(this);
    m_center_slot = new QFrame(this);
    m_right_slot  = new QFrame(this);
    m_left_slot  ->setObjectName("TOP_BAR_LEFT");
    m_center_slot->setObjectName("TOP_BAR_CENTER");
    m_right_slot ->setObjectName("TOP_BAR_RIGHT");

    // left slot: five status dots (CAM / TCU / LENS / LASER / PTZ) and the pixel-depth and resolution readouts
    auto *ll = new QHBoxLayout(m_left_slot);   ll->setContentsMargins(0, 0, 0, 0); ll->setSpacing(6);

    auto make_dot = [&](const char *name) -> DotIndicator* {
        auto *d = new DotIndicator(m_left_slot);
        d->setObjectName(name);
        ll->addWidget(d);
        return d;
    };
    m_cam_dot   = make_dot("CAM_DOT");
    m_tcu_dot   = make_dot("TCU_DOT");
    m_lens_dot  = make_dot("LENS_DOT");
    m_laser_dot = make_dot("LASER_DOT");
    m_ptz_dot   = make_dot("PTZ_DOT");

    ll->addSpacing(8);

    m_pixel_depth_label = new QLabel("--bit", m_left_slot);
    m_pixel_depth_label->setObjectName("PIXEL_DEPTH_LABEL");
    ll->addWidget(m_pixel_depth_label);

    m_resolution_label = new QLabel("-- x --", m_left_slot);
    m_resolution_label->setObjectName("RESOLUTION_LABEL");
    ll->addWidget(m_resolution_label);

    ll->addStretch(1);

    // center slot: system selector, mode tabs, clock
    auto *cl = new QHBoxLayout(m_center_slot); cl->setContentsMargins(0, 0, 0, 0); cl->setSpacing(8);

    m_system_selector = new QComboBox(m_center_slot);
    m_system_selector->setObjectName("SYSTEM_SELECTOR");
    m_system_selector->addItem("System 1");
    m_system_selector->setFixedWidth(90);
    connect(m_system_selector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TopBar::system_changed);
    cl->addWidget(m_system_selector);

    cl->addStretch(1);

    m_mode_group = new QButtonGroup(this);
    auto make_tab = [&](const char *text, const char *name, Mode mode, bool checked) {
        auto *btn = new QPushButton(text, m_center_slot);
        btn->setObjectName(name);
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setFixedHeight(22);
        m_mode_group->addButton(btn, static_cast<int>(mode));
        cl->addWidget(btn);
    };
    make_tab("SETUP",   "MODE_SETUP",   Mode::SETUP,   false);
    make_tab("MONITOR", "MODE_MONITOR", Mode::MONITOR, true);
    make_tab("SCAN",    "MODE_SCAN",    Mode::SCAN,    false);
    m_mode_group->setExclusive(true);

    connect(m_mode_group, QOverload<int>::of(&QButtonGroup::buttonClicked), this,
            [this](int id) {
                m_current_mode = static_cast<Mode>(id);
                emit mode_changed(m_current_mode);
            });

    cl->addStretch(1);

    m_clock_label = new QLabel(QTime::currentTime().toString("HH:mm:ss"), m_center_slot);
    m_clock_label->setObjectName("TOP_BAR_CLOCK");
    cl->addWidget(m_clock_label);

    m_clock_timer = new QTimer(this);
    m_clock_timer->setInterval(1000);
    connect(m_clock_timer, &QTimer::timeout, this,
            [this]{ m_clock_label->setText(QTime::currentTime().toString("HH:mm:ss")); });
    m_clock_timer->start();
    auto *rl = new QHBoxLayout(m_right_slot);  rl->setContentsMargins(0, 0, 0, 0); rl->setSpacing(4);

    // window-control trio is created in the constructor (no signal_receiver dependency)
    m_window_controls = new QFrame(m_right_slot);
    m_window_controls->setObjectName("TOP_WINDOW_CONTROLS");
    m_window_controls->setFixedWidth(120);
    m_window_controls->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_window_controls->setMouseTracking(true);
    auto *wl = new QHBoxLayout(m_window_controls);
    wl->setContentsMargins(0, 0, 0, 0);
    wl->setSpacing(0);

    m_min  = new TopButton("", m_window_controls);
    m_max  = new TopButton("", m_window_controls);
    m_exit = new TopButton("", m_window_controls);
    m_min ->setObjectName("MIN_BTN");
    m_max ->setObjectName("MAX_BTN");
    m_exit->setObjectName("EXIT_BTN");
    m_max ->setProperty("windowState", "normal");
    m_min ->setFixedWidth(40);
    m_max ->setFixedWidth(40);
    m_exit->setFixedWidth(40);
    m_min ->setMinimumHeight(30);
    m_max ->setMinimumHeight(30);
    m_exit->setMinimumHeight(30);
    m_min ->setIconSize(QSize(16, 16));
    m_max ->setIconSize(QSize(16, 16));
    m_exit->setIconSize(QSize(16, 16));
    m_min ->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_max ->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_exit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_min ->setMouseTracking(true);
    m_max ->setMouseTracking(true);
    m_exit->setMouseTracking(true);
    // action cluster pushed in setup(); for now reserve their layout slots so spacing matches once populated
    rl->addStretch(1);
    wl->addWidget(m_min);
    wl->addWidget(m_max);
    wl->addWidget(m_exit);
    rl->addWidget(m_window_controls);

    root->addWidget(m_left_slot,   1);
    root->addWidget(m_center_slot, 1);
    root->addWidget(m_right_slot,  1);

    connect(m_min,  &QPushButton::clicked, this, [this]{ window()->showMinimized(); });
    connect(m_max,  &QPushButton::clicked, this, &TopBar::process_maximize);
    connect(m_exit, &QPushButton::clicked, this, [this]{ window()->close(); });

    apply_color_mode(false);
}

void TopBar::setup(QObject *ptr)
{
    m_signal_receiver = ptr;

    auto *rl = qobject_cast<QHBoxLayout*>(m_right_slot->layout());

    // build the action cluster (ADDON / URL / SETTINGS / CAP / CLS / LANG / COLOR MODE) and insert before min/max/exit
    auto append_action = [&](TopButton *&btn, const char *obj_name) {
        btn = new TopButton("", m_right_slot);
        btn->setObjectName(obj_name);
        btn->setFixedSize(20, 20);
        btn->setMouseTracking(true);
        // Insert ahead of the zero-spacing window-controls group so the
        // action icons keep normal spacing, while MIN/MAX/EXIT stay flush.
        rl->insertWidget(rl->count() - 1, btn);
    };

    append_action(m_addon,    "ADDON_BTN");
    append_action(m_url,      "URL_BTN");
    append_action(m_settings, "SETTINGS_BTN");
    append_action(m_capture,  "CAPTURE_BTN");
    append_action(m_cls,      "CLS_BTN");
    append_action(m_lang,     "LANGUAGE_BTN");
    append_action(m_color_mode, "COLOR_MODE_BTN");

    connect(m_addon,   SIGNAL(clicked()), m_signal_receiver, SLOT(on_DUAL_LIGHT_BTN_clicked()));
    connect(m_url,     SIGNAL(clicked()), m_signal_receiver, SLOT(prompt_for_input_file()));
    connect(m_capture, SIGNAL(clicked()), m_signal_receiver, SLOT(screenshot()));
    connect(m_cls,     SIGNAL(clicked()), m_signal_receiver, SLOT(clean()));
    connect(m_lang,    SIGNAL(clicked()), m_signal_receiver, SLOT(switch_language()));
    connect(m_color_mode, SIGNAL(clicked()), m_signal_receiver, SLOT(toggle_color_mode()));

    QMenu *settings_menu = new QMenu(m_settings);
    settings_menu->setFont(monaco);

    QMenu *pref_export = settings_menu->addMenu(">> export pref.");
    pref_export->addAction("to file",   m_signal_receiver, SLOT(save_config_to_file()));
    pref_export->addAction("to preset", m_signal_receiver, SLOT(generate_preset_data()));
    settings_menu->addAction("<< load pref.",   m_signal_receiver, SLOT(prompt_for_config_file()));
    settings_menu->addAction("## config s.n.",  m_signal_receiver, SLOT(prompt_for_serial_file()));
    settings_menu->addAction("=> export video", m_signal_receiver, SLOT(export_current_video()), QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_S));

    m_settings->setMenu(settings_menu);
}

void TopBar::process_maximize()
{
    QWidget *w = window();
    if (!w) return;

    if (!m_is_maximized) {
        m_normal_geom = w->geometry();
        w->setWindowState(w->windowState() | Qt::WindowMaximized);
        m_is_maximized = true;
        sync_window_control_state();
        return;
    }

    w->setWindowState(w->windowState() & ~Qt::WindowMaximized);
    w->setGeometry(m_normal_geom);
    m_is_maximized = false;
    sync_window_control_state();
}

void TopBar::apply_color_mode(bool light_mode)
{
    m_light_mode = light_mode;
    const QString mode_dir = m_light_mode ? "light" : "dark";
    if (m_min)  m_min ->setIcon(QIcon(":/tools/" + mode_dir + "/min"));
    if (m_exit) m_exit->setIcon(QIcon(":/tools/" + mode_dir + "/exit"));
    sync_window_control_state();
}

void TopBar::sync_window_control_state()
{
    if (!m_max) return;
    const QString mode_dir = m_light_mode ? "light" : "dark";
    m_max->setIcon(QIcon(":/tools/" + mode_dir + "/" + QString(m_is_maximized ? "restore" : "max")));
    m_max->setProperty("windowState", m_is_maximized ? "maximized" : "normal");
    repolish(m_max);
}

void TopBar::mousePressEvent(QMouseEvent *event)
{
    QFrame::mousePressEvent(event);
    if (event->button() != Qt::LeftButton) return;
    m_pressed  = true;
    m_prev_pos = event->globalPos();
}

void TopBar::mouseMoveEvent(QMouseEvent *event)
{
    QFrame::mouseMoveEvent(event);
    if (m_is_maximized || !m_pressed) return;
    window()->move(window()->pos() + event->globalPos() - m_prev_pos);
    m_prev_pos = event->globalPos();
}

void TopBar::mouseReleaseEvent(QMouseEvent *event)
{
    QFrame::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton) return;
    m_pressed = false;
}

void TopBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);
    if (event->button() != Qt::LeftButton) return;
    process_maximize();
}

void TopBar::set_pixel_depth(int depth)
{
    m_pixel_depth_label->setText(QString::number(depth) + "-bit");
}

void TopBar::set_resolution(int w, int h)
{
    m_resolution_label->setText(QString::asprintf("%d x %d", w, h));
}
