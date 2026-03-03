#include "titlebar.h"
#include "visual/preferences.h"
#include "visual/scanconfig.h"

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
    connect(preferences, SIGNAL(search_for_devices()),               signal_receiver, SLOT(search_for_devices()));
    connect(preferences, SIGNAL(query_dev_ip()),                     signal_receiver, SLOT(update_dev_ip()));
    connect(preferences, SIGNAL(set_dev_ip(int, int)),               signal_receiver, SLOT(set_dev_ip(int, int)));
    connect(preferences, SIGNAL(change_pixel_format(int)),           signal_receiver, SLOT(change_pixel_format(int)));
    connect(preferences, SIGNAL(rotate_image(int)),                  signal_receiver, SLOT(rotate(int)));
    connect(preferences, SIGNAL(device_underwater(bool)),            signal_receiver, SLOT(update_light_speed(bool)));
    connect(preferences, SIGNAL(get_port_info(int)),                 signal_receiver, SLOT(display_port_info(int)));
    connect(preferences, SIGNAL(change_baudrate(int, int)),          signal_receiver, SLOT(set_baudrate(int, int)));
    connect(preferences, SIGNAL(set_tcp_status(int, bool)),          signal_receiver, SLOT(set_tcp_status_on_port(int, bool)));
    connect(preferences, SIGNAL(share_tcu_port(bool)),               signal_receiver, SLOT(set_tcu_as_shared_port(bool)));
    connect(preferences, SIGNAL(com_write(int, QByteArray)),         signal_receiver, SLOT(com_write_data(int, QByteArray)));
    connect(preferences, SIGNAL(tcu_type_changed(int)),              signal_receiver, SLOT(set_tcu_type(int)));
    connect(preferences, SIGNAL(set_auto_mcp(bool)),                 signal_receiver, SLOT(set_auto_mcp(bool)));
    connect(preferences, SIGNAL(rep_freq_unit_changed(int)),         signal_receiver, SLOT(setup_hz(int)));
    connect(preferences, SIGNAL(base_unit_changed(int)),             signal_receiver, SLOT(setup_stepping(int)));
    connect(preferences, SIGNAL(max_dist_changed(float)),            signal_receiver, SLOT(setup_max_dist(float)));
    connect(preferences, SIGNAL(max_dov_changed(float)),             signal_receiver, SLOT(setup_max_dov(float)));
    connect(preferences, SIGNAL(delay_offset_changed(float)),        signal_receiver, SLOT(update_delay_offset(float)));
    connect(preferences, SIGNAL(gate_width_offset_changed(float)),   signal_receiver, SLOT(update_gate_width_offset(float)));
    connect(preferences, SIGNAL(laser_offset_changed(float)),        signal_receiver, SLOT(update_laser_offset(float)));
    connect(preferences, SIGNAL(ps_config_updated(bool, int, uint)), signal_receiver, SLOT(update_ps_config(bool, int, uint)));
    connect(preferences, SIGNAL(laser_toggled(int)),                 signal_receiver, SLOT(setup_laser(int)));
    connect(preferences, SIGNAL(lower_3d_thresh_updated()),          signal_receiver, SLOT(update_lower_3d_thresh()));
    connect(preferences, SIGNAL(query_tcu_param()),                  signal_receiver, SLOT(reset_custom_3d_params()));

    QMenu *pref_export = settings_menu->addMenu(">> export pref.");
    pref_export->addAction("to file", signal_receiver, SLOT(save_config_to_file())/*, QKeySequence(Qt::ALT + Qt::Key_E)*/);
    pref_export->addAction("to preset", signal_receiver, SLOT(generate_preset_data())/*, QKeySequence(Qt::ALT + Qt::Key_P)*/);
//    settings_menu->addAction(">> export pref.", signal_receiver, SLOT(export_config())/*, QKeySequence(Qt::ALT + Qt::Key_E)*/);
    settings_menu->addAction("<< load pref.",   signal_receiver, SLOT(prompt_for_config_file())/*, QKeySequence(Qt::ALT + Qt::Key_R)*/);
    // TODO congigure serial number should be exclusive to ICMOS only
    settings_menu->addAction("## config s.n.",  signal_receiver, SLOT(prompt_for_serial_file())/*, QKeySequence(Qt::ALT + Qt::Key_C)*/);
    settings_menu->addAction("=> export video", signal_receiver, SLOT(export_current_video()), QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_S));
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
