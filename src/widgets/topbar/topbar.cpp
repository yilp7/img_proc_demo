#include "topbar.h"

// ----- TopButton -----

TopButton::TopButton(QString icon, QWidget *parent)
    : QPushButton(QIcon(icon), "", parent) {}

void TopButton::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    setCursor(cursor_curr_pointer);
}

// ----- TopBar -----

TopBar::TopBar(QWidget *parent)
    : QFrame(parent),
      url(nullptr), settings(nullptr), capture(nullptr),
      cls(nullptr), lang(nullptr), theme(nullptr),
      min(nullptr), max(nullptr), exit(nullptr),
      is_maximized(false),
      m_left_slot(nullptr), m_center_slot(nullptr), m_right_slot(nullptr),
      m_mode_group(nullptr), m_system_selector(nullptr),
      m_clock_timer(nullptr), m_clock_label(nullptr),
      m_current_mode(Mode::Monitor),
      m_cam_dot(nullptr), m_tcu_dot(nullptr), m_lens_dot(nullptr),
      m_laser_dot(nullptr), m_ptz_dot(nullptr),
      m_pixel_depth_label(nullptr), m_resolution_label(nullptr),
      m_pressed(false),
      m_signal_receiver(nullptr)
{
    setObjectName("TOP_BAR");

    auto *root = new QHBoxLayout(this);
    // Right margin = 0 so EXIT_BTN touches the window's right edge — Fitts'
    // law: when the window is maximized, the user can slam the mouse into
    // the screen corner and click without needing to pixel-aim.
    root->setContentsMargins(8, 0, 0, 0);
    root->setSpacing(8);

    m_left_slot   = new QFrame(this);
    m_center_slot = new QFrame(this);
    m_right_slot  = new QFrame(this);
    m_left_slot  ->setObjectName("TOP_BAR_LEFT");
    m_center_slot->setObjectName("TOP_BAR_CENTER");
    m_right_slot ->setObjectName("TOP_BAR_RIGHT");

    // left slot — 5 DotIndicators (CAM/TCU/LENS/LASER/PTZ) + pixel-depth + resolution readouts
    auto *ll = new QHBoxLayout(m_left_slot);   ll->setContentsMargins(0, 0, 0, 0); ll->setSpacing(6);

    auto makeDot = [&](const char *name) -> DotIndicator* {
        auto *d = new DotIndicator(m_left_slot);
        d->setObjectName(name);
        ll->addWidget(d);
        return d;
    };
    m_cam_dot   = makeDot("CAM_DOT");
    m_tcu_dot   = makeDot("TCU_DOT");
    m_lens_dot  = makeDot("LENS_DOT");
    m_laser_dot = makeDot("LASER_DOT");
    m_ptz_dot   = makeDot("PTZ_DOT");

    ll->addSpacing(8);

    m_pixel_depth_label = new QLabel("--bit", m_left_slot);
    m_pixel_depth_label->setObjectName("PIXEL_DEPTH_LABEL");
    ll->addWidget(m_pixel_depth_label);

    m_resolution_label = new QLabel("-- x --", m_left_slot);
    m_resolution_label->setObjectName("RESOLUTION_LABEL");
    ll->addWidget(m_resolution_label);

    ll->addStretch(1);

    // center slot — system selector | mode tabs | clock
    auto *cl = new QHBoxLayout(m_center_slot); cl->setContentsMargins(0, 0, 0, 0); cl->setSpacing(8);

    m_system_selector = new QComboBox(m_center_slot);
    m_system_selector->setObjectName("SYSTEM_SELECTOR");
    m_system_selector->addItem("System 1");
    m_system_selector->setFixedWidth(90);
    connect(m_system_selector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TopBar::systemChanged);
    cl->addWidget(m_system_selector);

    cl->addStretch(1);

    m_mode_group = new QButtonGroup(this);
    auto makeTab = [&](const char *text, const char *name, Mode mode, bool checked) {
        auto *btn = new QPushButton(text, m_center_slot);
        btn->setObjectName(name);
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setFixedHeight(22);
        m_mode_group->addButton(btn, static_cast<int>(mode));
        cl->addWidget(btn);
    };
    makeTab("SETUP",   "MODE_SETUP",   Mode::Setup,   false);
    makeTab("MONITOR", "MODE_MONITOR", Mode::Monitor, true);
    makeTab("SCAN",    "MODE_SCAN",    Mode::Scan,    false);
    m_mode_group->setExclusive(true);

    connect(m_mode_group, QOverload<int>::of(&QButtonGroup::buttonClicked), this,
            [this](int id) {
                m_current_mode = static_cast<Mode>(id);
                emit modeChanged(m_current_mode);
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
    min  = new TopButton(":/tools/min",  m_right_slot);
    max  = new TopButton(":/tools/max",  m_right_slot);
    exit = new TopButton(":/tools/exit", m_right_slot);
    min ->setObjectName("MIN_BTN");
    max ->setObjectName("MAX_BTN");
    exit->setObjectName("EXIT_BTN");
    min ->setFixedSize(40, 30);
    max ->setFixedSize(40, 30);
    exit->setFixedSize(40, 30);
    min ->setMouseTracking(true);
    max ->setMouseTracking(true);
    exit->setMouseTracking(true);

    // action cluster pushed in setup(); for now reserve their layout slots so spacing matches once populated
    rl->addStretch(1);
    rl->addWidget(min);
    rl->addWidget(max);
    rl->addWidget(exit);

    root->addWidget(m_left_slot,   1);
    root->addWidget(m_center_slot, 1);
    root->addWidget(m_right_slot,  1);

    connect(min,  &QPushButton::clicked, this, [this]{ window()->showMinimized(); });
    connect(max,  &QPushButton::clicked, this, &TopBar::process_maximize);
    connect(exit, &QPushButton::clicked, this, [this]{ window()->close(); });
}

void TopBar::setup(QObject *ptr)
{
    m_signal_receiver = ptr;

    auto *rl = qobject_cast<QHBoxLayout*>(m_right_slot->layout());

    // build the action cluster (URL / SETTINGS / CAP / CLS / LANG / THEME) and insert before min/max/exit
    auto append_action = [&](TopButton *&btn, const char *obj_name) {
        btn = new TopButton("", m_right_slot);
        btn->setObjectName(obj_name);
        btn->setFixedSize(20, 20);
        btn->setMouseTracking(true);
        // insert ahead of min (which is at index = stretch + 0 = 1; but we keep prepending so order is left→right: url, settings, cap, cls, lang, theme, min, max, exit)
        rl->insertWidget(rl->count() - 3, btn);
    };

    append_action(url,      "URL_BTN");
    append_action(settings, "SETTINGS_BTN");
    append_action(capture,  "CAPTURE_BTN");
    append_action(cls,      "CLS_BTN");
    append_action(lang,     "LANGUAGE_BTN");
    append_action(theme,    "THEME_BTN");

    connect(url,     SIGNAL(clicked()), m_signal_receiver, SLOT(prompt_for_input_file()));
    connect(capture, SIGNAL(clicked()), m_signal_receiver, SLOT(screenshot()));
    connect(cls,     SIGNAL(clicked()), m_signal_receiver, SLOT(clean()));
    connect(lang,    SIGNAL(clicked()), m_signal_receiver, SLOT(switch_language()));
    connect(theme,   SIGNAL(clicked()), m_signal_receiver, SLOT(set_theme()));

    // settings menu — port from TitleBar::setup verbatim
    QMenu *settings_menu = new QMenu(settings);
    settings_menu->setFont(monaco);

    QMenu *pref_export = settings_menu->addMenu(">> export pref.");
    pref_export->addAction("to file",   m_signal_receiver, SLOT(save_config_to_file()));
    pref_export->addAction("to preset", m_signal_receiver, SLOT(generate_preset_data()));
    settings_menu->addAction("<< load pref.",   m_signal_receiver, SLOT(prompt_for_config_file()));
    settings_menu->addAction("## config s.n.",  m_signal_receiver, SLOT(prompt_for_serial_file()));
    settings_menu->addAction("=> export video", m_signal_receiver, SLOT(export_current_video()), QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_S));

    settings->setMenu(settings_menu);
}

void TopBar::process_maximize()
{
    if (!is_maximized) m_normal_geom = window()->geometry();
    max->setIcon(is_maximized ? QIcon(":/tools/max") : QIcon(":/tools/restore"));
    if (is_maximized) window()->setGeometry(m_normal_geom);
    else              window()->setGeometry(QApplication::desktop()->availableGeometry());
    is_maximized ^= 1;
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
    if (is_maximized || !m_pressed) return;
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
