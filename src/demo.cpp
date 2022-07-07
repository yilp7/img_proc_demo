#include "demo.h"
#include "./ui_demo_dev.h"

GrabThread::GrabThread(void *info)
{
    p_info = info;
}

void GrabThread::run()
{
    ((Demo*)p_info)->grab_thread_process();
}

Demo* wnd;

Demo::Demo(QWidget *parent)
    : QMainWindow(parent),
    mouse_pressed(false),
    ui(new Ui::Demo),
    calc_avg_option(5),
    range_threshold(0),
    trigger_by_software(false),
    curr_cam(NULL),
    time_exposure_edit(5000),
    gain_analog_edit(0),
    frame_rate_edit(10),
    serial_port{NULL},
    tcp_port{NULL},
    use_tcp(false),
    share_serial_port(false),
    rep_freq(30),
    delay_n_n(0),
    stepping(10),
    hz_unit(0),
    base_unit(0),
    fps(10),
    duty(5000),
    mcp(5),
    laser_on(0),
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
    h_grab_thread(NULL),
    grab_thread_state(false),
    h_joystick_thread(NULL),
    seq_idx(0),
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
    trans.load("project_zh.qm");
    ui->CENTRAL->setMouseTracking(true);
    ui->LEFT->setMouseTracking(true);
    ui->MID->setMouseTracking(true);
    ui->RIGHT->setMouseTracking(true);
    ui->TITLE->setMouseTracking(true);
    setMouseTracking(true);
    setCursor(QCursor(QPixmap(":/cursor/cursor").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation), 0, 0));

    tp.start();

    dist_ns = c * 1e-9 / 2;

    // initialization
    // - default save path
    save_location += QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
//    qDebug() << QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section('/', 0, -1);
    TEMP_SAVE_LOCATION = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s);", hide_left ? "right" : "left"));

    connect(this, SIGNAL(set_pixmap(QPixmap)), ui->SOURCE_DISPLAY, SLOT(setPixmap(QPixmap)), Qt::QueuedConnection);

    // - image operations
    QComboBox *enhance_options = ui->ENHANCE_OPTIONS;
    QComboBox *calc_avg_options = ui->FRAME_AVG_OPTIONS;
    enhance_options->addItem(tr("None"));
    enhance_options->addItem(tr("Histogram"));
    enhance_options->addItem(tr("Laplace"));
    enhance_options->addItem(tr("Log-based"));
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
    com_label[1] = ui->RANGE_COM;
    com_label[2] = ui->LENS_COM;
    com_label[3] = ui->LASER_COM;
    com_label[4] = ui->PTZ_COM;

    com_edit[0] = ui->TCU_COM_EDIT;
    com_edit[1] = ui->RANGE_COM_EDIT;
    com_edit[2] = ui->LENS_COM_EDIT;
    com_edit[3] = ui->LASER_COM_EDIT;
    com_edit[4] = ui->PTZ_COM_EDIT;

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
    connect(ui->FOCUS_SPEED_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_focus_speed(int)), Qt::QueuedConnection);

    ui->CONTINUE_SCAN_BUTTON->hide();
    ui->RESTART_SCAN_BUTTON->hide();

    // connect QLineEdit, QButton signals used in thread
    connect(this, SIGNAL(append_text(QString)), SLOT(append_data(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(update_scan(bool)), SLOT(enable_scan_options(bool)), Qt::QueuedConnection);
//    connect(this, &Demo::appendText, &Demo::append_data);
    connect(this, SIGNAL(update_delay_in_thread()), SLOT(update_delay()), Qt::QueuedConnection);
    connect(this, SIGNAL(update_mcp_in_thread(int)), SLOT(change_mcp(int)), Qt::QueuedConnection);

    // register signal when thread pool full
    connect(this, SIGNAL(task_queue_full()), SLOT(stop_image_writing()), Qt::UniqueConnection);

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
    ui->DRAG_TOOL->click();
    ui->CURR_COORD->setup("current");
    connect(ui->SOURCE_DISPLAY, SIGNAL(curr_pos(QPoint)), ui->CURR_COORD, SLOT(display_pos(QPoint)));
    ui->START_COORD->setup("start");
    connect(ui->SOURCE_DISPLAY, SIGNAL(start_pos(QPoint)), ui->START_COORD, SLOT(display_pos(QPoint)));
    ui->SHAPE_INFO->setup("size");
    connect(ui->SOURCE_DISPLAY, SIGNAL(shape_size(QPoint)), ui->SHAPE_INFO, SLOT(display_pos(QPoint)));

    ptz_grp = new QButtonGroup(this);
    ui->UP_LEFT_BTN->setIcon(QIcon(":/directions/up_left"));
    ptz_grp->addButton(ui->UP_LEFT_BTN, 0);
    ui->UP_BTN->setIcon(QIcon(":/directions/up"));
    ptz_grp->addButton(ui->UP_BTN, 1);
    ui->UP_RIGHT_BTN->setIcon(QIcon(":/directions/up_right"));
    ptz_grp->addButton(ui->UP_RIGHT_BTN, 2);
    ui->LEFT_BTN->setIcon(QIcon(":/directions/left"));
    ptz_grp->addButton(ui->LEFT_BTN, 3);
    ui->SELF_TEST_BTN->setIcon(QIcon(":/directions/self_test"));
    ptz_grp->addButton(ui->SELF_TEST_BTN, 4);
    ui->RIGHT_BTN->setIcon(QIcon(":/directions/right"));
    ptz_grp->addButton(ui->RIGHT_BTN, 5);
    ui->DOWN_LEFT_BTN->setIcon(QIcon(":/directions/down_left"));
    ptz_grp->addButton(ui->DOWN_LEFT_BTN, 6);
    ui->DOWN_BTN->setIcon(QIcon(":/directions/down"));
    ptz_grp->addButton(ui->DOWN_BTN, 7);
    ui->DOWN_RIGHT_BTN->setIcon(QIcon(":/directions/down_right"));
    ptz_grp->addButton(ui->DOWN_RIGHT_BTN, 8);
    connect(ptz_grp, SIGNAL(buttonPressed(int)), this, SLOT(ptz_button_pressed(int)));
    connect(ptz_grp, SIGNAL(buttonReleased(int)), this, SLOT(ptz_button_released(int)));

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

    // connect title bar to the main window (Demo*)
    ui->TITLE->setup(this);

    // connect to joystick (windows xbox only)
    h_joystick_thread = new JoystickThread(this);
    h_joystick_thread->start();

    connect(h_joystick_thread, SIGNAL(button_pressed(int)), this, SLOT(joystick_button_pressed(int)));
    connect(h_joystick_thread, SIGNAL(button_released(int)), this, SLOT(joystick_button_released(int)));
    connect(h_joystick_thread, SIGNAL(direction_changed(int)), this, SLOT(joystick_direction_changed(int)));

    connect(this, SIGNAL(update_fishnet_result(int)), SLOT(display_fishnet_result(int)));

    // right before gui display (init state)
    for (int i = 0; i < 5; i++) serial_port[i] = new QSerialPort(this), setup_serial_port(serial_port + i, i, com_edit[i]->text(), 9600);
    for (int i = 0; i < 5; i++) tcp_port[i] = new QTcpSocket(this);
    on_ENUM_BUTTON_clicked();
    if (serial_port[0]->isOpen() && serial_port[3]->isOpen()) on_LASER_BTN_clicked();

    // - set startup focus
    (ui->START_BUTTON->isEnabled() ? ui->START_BUTTON : ui->ENUM_BUTTON)->setFocus();

//    Preferences *preferences = new Preferences;
//    preferences->show();

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
    ui->TITLE->prog_settings->max_dist = 6000;
    ui->TITLE->prog_settings->data_exchange(false);
    ui->TITLE->prog_settings->max_dist_changed(6000);

#else
    ui->LOGO->hide();
#endif
}

Demo::~Demo()
{
    tp.stop();

    delete ui;
}

void Demo::data_exchange(bool read){
    if (read) {
//        device_idx = ui->DEVICE_SELECTION->currentIndex();
        calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 5 + 5;

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
        ui->FRAME_AVG_OPTIONS->setCurrentIndex(calc_avg_option / 5 - 1);

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

int Demo::grab_thread_process() {
    Display *disp = ui->SOURCE_DISPLAY;
    ProgSettings *settings = ui->TITLE->prog_settings;
    QImage stream;
    int ww, hh;
    float weight = h / 1024.0; // font scale & thickness
    prev_3d = cv::Mat(h, w, CV_8UC3);
    prev_img = cv::Mat(h, w, CV_8UC1);
    double *range = (double*)calloc(w * h, sizeof(double));
    cv::Mat sobel, fishnet_res;
    cv::dnn::Net net = cv::dnn::readNet("model/resnet18.onnx");
    double threshold = 0.99;
    while (grab_thread_state) {
        while (img_q.size() > 3) img_q.pop();

        if (img_q.empty()) {
//            QThread::msleep(10);
            img_mem = prev_img.clone();
            updated = false;
        }
        else {
            img_mem = img_q.front();
            img_q.pop();
            updated = true;
            if (device_type < 0) img_q.push(img_mem.clone());
        }

        image_mutex.lock();
//        qDebug () << QDateTime::currentDateTime().toString() << updated << img_mem.data;

        if (updated) {
            // calc histogram (grayscale)
            memset(hist, 0, 256 * sizeof(uint));
            for (int i = 0; i < h; i++) for (int j = 0; j < w; j++) hist[(img_mem.data + i * img_mem.cols)[j]]++;

            // if the image needs flipping
            if (settings->symmetry) cv::flip(img_mem, img_mem, settings->symmetry - 2);

            // mcp self-adaptive
            if (auto_mcp && !ui->MCP_SLIDER->hasFocus()) {
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
                // TODO not yet well implemented
                if (abs(focus_direction) > 7) lens_stop(), focus_direction = 0;
                else if (focus_direction > 0) lens_stop(), change_focus_speed(8), focus_far();
                else if (focus_direction < 0) lens_stop(), change_focus_speed(16), focus_near();
            }
        }
*/
        if (ui->TITLE->prog_settings->fishnet_recog) {
            cv::cvtColor(img_mem, fishnet_res, cv::COLOR_GRAY2RGB);
            fishnet_res.convertTo(fishnet_res, CV_32FC3, 1.0 / 255);
            cv::resize(fishnet_res, fishnet_res, cv::Size(224, 224));

            cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(224, 224));
            net.setInput(blob);
            cv::Mat prob = net.forward("195");
//            std::cout << cv::format(prob, cv::Formatter::FMT_C) << std::endl;

            double min, max;
            cv::minMaxLoc(prob, &min, &max);

    //        prob -=max;
            double is_net = exp(prob.at<float>(1)) / (exp(prob.at<float>(0)) + exp(prob.at<float>(1)));
//            double is_net = exp(prob.at<float>(1)) / exp(prob.at<float>(0) + prob.at<float>(1));

            emit update_fishnet_result(is_net > ui->TITLE->prog_settings->fishnet_thresh);
        }
        else emit update_fishnet_result(-1);

        // process frame average
        if (seq_sum.empty()) seq_sum = cv::Mat::zeros(h, w, CV_16U);
        if (ui->FRAME_AVG_CHECK->isChecked()) {
            if (updated) {
                calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 5 + 5;
                if (seq[9].empty()) for (auto& m: seq) m = cv::Mat::zeros(h, w, CV_16U);

                seq_sum -= seq[seq_idx];
                img_mem.convertTo(seq[seq_idx], CV_16U);
                seq_sum += seq[seq_idx];
//                for(int i = 0; i < calc_avg_option; i++) seq_sum += seq[i];

                seq_idx = (seq_idx + 1) % calc_avg_option;
            }
            seq_sum.convertTo(modified_result, CV_8U, 1.0 / (calc_avg_option * (1 << (pixel_depth - 8))));
        }
        else {
            img_mem.convertTo(modified_result, CV_8U, 1.0 / (1 << (pixel_depth - 8)));
        }

        // process 3d image construction from ABN frames
        if (ui->IMG_3D_CHECK->isChecked()) {
            ww = w + 104;
            hh = h;
            range_threshold = ui->RANGE_THRESH_EDIT->text().toFloat();
//            modified_result = frame_a_3d ? prev_3d : ImageProc::gated3D(prev_img, img_mem, delay_dist / dist_ns, depth_of_view / dist_ns, range_threshold);
//            if (prev_3d.empty()) cv::cvtColor(img_mem, prev_3d, cv::COLOR_GRAY2RGB);
            if (updated) {
                if (frame_a_3d) ImageProc::gated3D(prev_img, img_mem, modified_result, delay_dist / dist_ns, depth_of_view / dist_ns, range, range_threshold);
                else            ImageProc::gated3D(img_mem, prev_img, modified_result, delay_dist / dist_ns, depth_of_view / dist_ns, range, range_threshold);
                prev_3d = modified_result;
                frame_a_3d ^= 1;
//                if (device_type < -1) device_type++;
            }
            else modified_result = prev_3d;
//            if (!frame_a_3d) prev_3d = modified_result.clone();

            cv::resize(modified_result, img_display, cv::Size(ui->SOURCE_DISPLAY->width(), ui->SOURCE_DISPLAY->height()), 0, 0, cv::INTER_AREA);
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
                // log
                case 3: {
                    cv::Mat img_log;
                    modified_result.convertTo(img_log, CV_32F);
                    modified_result += 1.0;
                    cv::log(img_log, img_log);
                    img_log *= settings->log;
                    cv::normalize(img_log, img_log, 0, 255, cv::NORM_MINMAX);
                    cv::convertScaleAbs(img_log, modified_result);
                    break;
                }
                // SP
                case 4: {
                    ImageProc::plateau_equl_hist(&modified_result, &modified_result, 4);
                    break;
                }
                // accumulative
                case 5: {
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
                    ImageProc::accumulative_enhance(modified_result, modified_result, settings->accu_base);
                    break;
                }
                // sigmoid (nonlinear) (mergw log w/ 1/(1+exp))
                case 6: {
                    uchar *img = modified_result.data;
                    cv::Mat img_log, img_nonLT = cv::Mat(h, w, CV_8U);
                    modified_result.convertTo(img_log, CV_32F);
                    modified_result += 1.0;
                    cv::log(img_log, img_log);
                    img_log *= settings->log;
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
                case 7: {
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
                    ImageProc::adaptive_enhance(modified_result, modified_result, settings->low_in, settings->high_in, settings->low_out, settings->high_out, settings->gamma);
                    break;
                }
                // enhance_dehaze
                case 8: {
                    modified_result = ~modified_result;
                    ImageProc::haze_removal(modified_result, modified_result, 7, settings->dehaze_pct, 0.1, 60, 0.01);
                    modified_result = ~modified_result;
                    break;
                }
                // dcp
                case 9: {
                    ImageProc::haze_removal(modified_result, modified_result, 7, settings->dehaze_pct, 0.1, 60, 0.01);
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
            if (ui->HISTOGRAM_RADIO->isChecked()) {
                if (modified_result.channels() != 1) continue;
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
                ui->HIST_DISPLAY->setPixmap(QPixmap::fromImage(QImage(hist_image.data, hist_image.cols, hist_image.rows, hist_image.step, QImage::Format_RGB888).scaled(ui->HIST_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            }
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
            }

            // resize to display size
            cv::resize(modified_result, img_display, cv::Size(ui->SOURCE_DISPLAY->width(), ui->SOURCE_DISPLAY->height()), 0, 0, cv::INTER_AREA);
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
        emit set_pixmap(QPixmap::fromImage(stream.scaled(ui->SOURCE_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        // process scan
        if (scan && updated) {
            save_scan_img();
            delay_dist += scan_step;
//            filter_scan();

            emit update_delay_in_thread();
//            update_delay();
        }
        if (scan && std::round(delay_dist / dist_ns) > scan_stopping_delay) {on_SCAN_BUTTON_clicked();}

        // image write / video record
        if (updated && save_original) save_to_file(false);
        if (updated && save_modified) save_to_file(true);
        if (updated && record_original) {
            cv::Mat temp = img_mem.clone();
            if (base_unit == 2) cv::putText(temp, QString::asprintf("DIST %05d m DOV %04d m", (int)delay_dist, (int)depth_of_view).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            else cv::putText(temp, QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(delay_dist / dist_ns), (int)std::round(depth_of_view / dist_ns)).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            cv::putText(temp, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(ww - 240, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), weight * 2);
            vid_out[0].write(temp);
        }
        if (updated && record_modified) {
            if (!image_3d && !ui->INFO_CHECK->isChecked()) {
                if (base_unit == 2) cv::putText(modified_result, QString::asprintf("DIST %05d m DOV %04d m", (int)delay_dist, (int)depth_of_view).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                else cv::putText(modified_result, QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(delay_dist / dist_ns), (int)std::round(depth_of_view / dist_ns)).toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                cv::putText(modified_result, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            }
            vid_out[1].write(modified_result);
        }

        if (updated) prev_img = img_mem.clone();
        image_mutex.unlock();
    }
    free(range);
    return 0;
}

inline bool Demo::is_maximized()
{
    return ui->TITLE->is_maximized;
}

// used when moving temp recorded vid to destination
void Demo::move_to_dest(QString src, QString dst)
{
    QFile::rename(src, dst);
}

void Demo::save_image_bmp(cv::Mat img, QString filename)
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

void Demo::save_image_tif(cv::Mat img, QString filename)
{
    uint w = img.cols, h = img.rows, channel = img.channels();
    uint depth = 8 * (img.depth() / 2 + 1);
    FILE *f = fopen(filename.toLocal8Bit().constData(), "wb");
    static ushort type_short = 3, type_long = 4, type_fraction = 5;
    static uint count_1 = 1, count_3 = 3;
    static uint TWO = 2;
    static uint next_ifd_offset = 0;
    ulong block_size = w * channel, sum = h * block_size;
    uint i;

    // Offset=w*h*d + 8(eg:Img[1000][1000][3] --> 3000008)
    // RGB  Color:W H BitsPerSample Compression Photometric StripOffset Orientation SamplePerPixle RowsPerStrip StripByteCounts PlanarConfiguration
    // Gray Color:W H BitsPerSample Compression Photometric StripOffset Orientation SamplePerPixle RowsPerStrip StripByteCounts XResolution YResoulution PlanarConfiguration ResolutionUnit
    // DE_N ID:   0 1 2             3           4           5           6           7              8            9               10          11           12                  13

    fseek(f, 0, SEEK_SET);
    static ushort endian = 'II';
    fwrite(&endian, 2, 1, f);
    static ushort tiff_magic_number = 42;
    fwrite(&tiff_magic_number, 2, 1, f);
    uint ifd_offset = sum + 8; // 8 (IFH size)
    fwrite(&ifd_offset, 4, 1, f);

    // Insert the image data
    fwrite(img.data, 1, sum, f);

//    fseek(f, ifd_offset, SEEK_SET);
    static ushort num_de = 14;
    fwrite(&num_de, 2, 1, f);
    // 256 image width
    static ushort tag_w = 256;
    fwrite(&tag_w, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&w, 4, 1, f);

    // 257 image height
    static ushort tag_h = 257;
    fwrite(&tag_h, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&h, 4, 1, f);

    // 258 bits per sample
    static ushort tag_bits_per_sample = 258;
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
        // 8 (IFH size) + (2 + 14 * 12 + 4) (0th IFD size: 2->size of the num of DE = 14, 12->size of one DE, 4->size of offset of next IFD) = 146
        uint tag_bits_per_sample_offset = sum + 182;
        fwrite(&tag_bits_per_sample_offset, 4, 1, f);
        break;
    }
    default: break;
    }

    // 259 compression
    static ushort tag_compression = 259;
    fwrite(&tag_compression, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // use 1 for uncompressed

    // 262 photometric interpretation
    static ushort tag_photometric_interpretation = 262;
    fwrite(&tag_photometric_interpretation, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(channel == 1 ? &count_1 : &TWO, 4, 1, f); // use 1 for black is zero, use 2 for RGB

    // 273 strip offsets
    static ushort tag_strip_offsets = 273;
    fwrite(&tag_strip_offsets, 2, 1, f);
    fwrite(&type_long, 2, 1, f);
    fwrite(&h, 4, 1, f);
    // 1 channel
    // 8 (IFH size) + (2 + 14 * 12 + 4) (0th IFD size: 2->size of the num of DE = 14, 12->size of one DE, 4->size of offset of next IFD) = 182
    // 3 channels
    // 182 (bits per sample offsets) + 3 * 2 (3 channels * 2 bytes) = 188
    uint tag_strip_offsets_offset = channel == 1 ? sum + 182 : sum + 188;
    fwrite(&tag_strip_offsets_offset, 4, 1, f);

    // 274 orientation
    static ushort tag_orientation = 274;
    fwrite(&tag_orientation, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // 1 = The 0th row represents the visual top of the image, and the 0th column represents the visual left-hand side.

    // 277 samples per pixel
    static ushort tag_samples_per_pixel = 277;
    fwrite(&tag_samples_per_pixel, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(channel == 1 ? &count_1 : &count_3, 4, 1, f); // use 1 for grayscale image, use 3 for 3-channel RGB image

    // 278 rows per strip
    static ushort tag_rows_per_strip = 278;
    fwrite(&tag_rows_per_strip, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // rows = strips

    // 279 strip byte counts
    static ushort tag_strip_byte_counts = 279;
    fwrite(&tag_strip_byte_counts, 2, 1, f);
    fwrite(&type_long, 2, 1, f);
    fwrite(&h, 4, 1, f);
    // 1 channel
    // 182 (offset of strip offsets), 4 * h (length of tag 273, strip_offsets)
    // 3 channels
    // 188 (offset of strip offsets), 4 * h (length of tag 273, strip_offsets)
    uint tag_strip_byte_counts_offset = channel == 1 ? sum + 182 + 4 * h : sum + 188 + 4 * h;
    fwrite(&tag_strip_byte_counts_offset, 4, 1, f);

    // 282 X resolution
    static ushort tag_x_resolution = 282;
    fwrite(&tag_x_resolution, 2, 1, f);
    fwrite(&type_fraction, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    uint tag_x_resolution_offset = channel == 1 ? sum + 182 + 8 * h : sum + 188 + 8 * h;
    fwrite(&tag_x_resolution_offset, 4, 1, f);

    // 283 Y resolution
    static ushort tag_y_resolution = 283;
    fwrite(&tag_y_resolution, 2, 1, f);
    fwrite(&type_fraction, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    uint tag_y_resolution_offset = channel == 1 ? sum + 190 + 8 * h : sum + 196 + 8 * h;
    fwrite(&tag_y_resolution_offset, 4, 1, f);

    // 284 planar configuration
    static ushort tag_planar_configuration = 284;
    fwrite(&tag_planar_configuration, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // use 1 for chunky format

    // 296 resolution unit
    static ushort tag_resolution_unit = 296;
    fwrite(&tag_resolution_unit, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&TWO, 4, 1, f); // use 2 for inch

    fwrite(&next_ifd_offset, 4, 1, f);

    // (rgb only) value for tag 258, offset: sum + 182
    if (channel == 3) for(i = 0; i < 3; i++) fwrite(&depth, 2, 1, f);

    // value for tag 273, offset: sum + 182 (gray), sum + 188 (rgb)
    uint strip_offset = 8;
    for(i = 0; i < h; i++, strip_offset += block_size) fwrite(&strip_offset, 4, 1, f);

    // value for tag 279, offset: sum + 182 + 4 * h (gray), sum + 188 + 4 * h (rgb)
    for(i = 0; i < h; i++) fwrite(&block_size, 4, 1, f);

    // value for tag 282, offset: sum + 182 + 8 * h (gray), sum + 188 + 8 * h (rgb)
    static uint res_physical = 1, res_pixel = 72;
    fwrite(&res_pixel, 4, 1, f); // fraction w-pixel
    fwrite(&res_physical, 4, 1, f); // denominator PhyWidth-inch

    // value for tag 283, offset: sum + 190 + 8 * h (gray), sum + 196 + 8 * h (rgb)
    fwrite(&res_pixel, 4, 1, f); // fraction h-pixel
    fwrite(&res_physical,4, 1, f); // denominator PhyHeight-inch

    fclose(f);
}

void Demo::stop_image_writing()
{
    if (save_original) on_SAVE_BMP_BUTTON_clicked();
    if (save_modified) on_SAVE_RESULT_BUTTON_clicked();
    QMessageBox::warning(wnd, "PROMPT", "too much memory used, stopping writing images");
//    QElapsedTimer t;
//    t.start();
//    while (t.elapsed() < 1000) ((QApplication*)QApplication::instance())->processEvents();
//    QThread::msleep(1000);
}

void Demo::switch_language()
{
    en ? (qApp->removeTranslator(&trans), qApp->setFont(monaco)) : (qApp->installTranslator(&trans), qApp->setFont(consolas));
    ui->retranslateUi(this);
    ui->TITLE->prog_settings->switch_language(en, &trans);
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

void Demo::closeEvent(QCloseEvent *event)
{
    shut_down();
    event->accept();
    ui->TITLE->prog_settings->reject();
    if (QFile::exists("HQVSDK.xml")) QFile::remove("HQVSDK.xml");
}

int Demo::shut_down() {
    if (record_original) on_SAVE_AVI_BUTTON_clicked();
    if (record_modified) on_SAVE_FINAL_BUTTON_clicked();

    grab_thread_state = false;
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
void Demo::enable_controls(bool cam_rdy) {
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
    ui->INFO_CHECK->setEnabled(start_grabbing);
    ui->CENTER_CHECK->setEnabled(start_grabbing);
}

void Demo::save_to_file(bool save_result) {
//    QString temp = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"),
//            dest = QString(save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//    cv::imwrite(temp.toLatin1().data(), save_result ? modified_result : img_mem);
//    QFile::rename(temp, dest);

    cv::Mat *temp = save_result ? &modified_result : &img_mem;
//    QPixmap::fromImage(QImage(temp->data, temp->cols, temp->rows, temp->step, temp->channels() == 3 ? QImage::Format_RGB888 : QImage::Format_Indexed8)).save(save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp", "BMP", 100);

//    std::thread t_save(Demo::save_image_bmp, *temp, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp");
//    t_save.detach();

//    if (!tp.append_task(std::bind(Demo::save_image_tif, tif_16, save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full();
    switch (pixel_depth) {
    case  8: if (!tp.append_task(std::bind(Demo::save_image_bmp, temp->clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"))) emit task_queue_full(); break;
    case 10:
    case 12: if (!tp.append_task(std::bind(Demo::save_image_tif, temp->clone(), save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full(); break;
    default: break;
    }
}

void Demo::save_scan_img() {
//    QString temp = QString(TEMP_SAVE_LOCATION + "/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp"),
//            dest = QString(save_location + "/" + scan_name + "/ori_bmp/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp");
//    cv::imwrite(temp.toLatin1().data(), img_mem);
//    QFile::rename(temp, dest);
//    if (ui->TITLE->prog_settings->save_scan_ori) QPixmap::fromImage(QImage(img_mem.data, img_mem.cols, img_mem.rows, img_mem.step, QImage::Format_Indexed8)).save(save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp", "BMP");
    if (ui->TITLE->prog_settings->save_scan_ori) {
//        std::thread t_ori(save_image_bmp, img_mem, save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp");
//        t_ori.detach();
        tp.append_task(std::bind(save_image_bmp, img_mem.clone(), save_location + "/" + scan_name + "/ori_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp"));
    }
//    dest = QString(save_location + "/" + scan_name + "/res_bmp/" + QString::number(delay_a_n + delay_a_u * 1000) + ".bmp");
//    cv::imwrite(temp.toLatin1().data(), modified_result);
//    QFile::rename(temp, dest);
//    if (ui->TITLE->prog_settings->save_scan_res) QPixmap::fromImage(QImage(modified_result.data, modified_result.cols, modified_result.rows, modified_result.step, QImage::Format_Indexed8)).save(save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp", "BMP");
    if (ui->TITLE->prog_settings->save_scan_res) {
//        std::thread t_res(save_image_bmp, modified_result, save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp");
//        t_res.detach();
        tp.append_task(std::bind(save_image_bmp, modified_result.clone(), save_location + "/" + scan_name + "/res_bmp/" + (base_unit == 2 ? QString::asprintf("%fm", delay_dist) : QString::asprintf("%dns", delay_a_n + delay_a_u * 1000)) + ".bmp"));
    }
}

void Demo::setup_serial_port(QSerialPort **port, int id, QString port_num, int baud_rate) {
    (*port)->setPortName("COM" + port_num);
//    qDebug("%p", *com);

    if ((*port)->open(QIODevice::ReadWrite)) {
        QThread::msleep(10);
        (*port)->clear();
        qDebug("COM%s connected\n", qPrintable(port_num));
        com_label[id]->setStyleSheet("color: #B0C4DE;");

        (*port)->setBaudRate(baud_rate);
        (*port)->setDataBits(QSerialPort::Data8);
        (*port)->setParity(QSerialPort::NoParity);
        (*port)->setStopBits(QSerialPort::OneStop);
        (*port)->setFlowControl(QSerialPort::NoFlowControl);

        // send initial data
        switch (id) {
        case 0:
//            convert_to_send_tcu(0x01, (laser_width_u * 1000 + laser_width_n + 8) / 8);
            communicate_display(0, convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
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
        ui->TITLE->prog_settings->display_baudrate(id, baud_rate);
    }
    else {
        com_label[id]->setStyleSheet("color: #CD5C5C;");
    }
}

void Demo::on_ENUM_BUTTON_clicked()
{
    if (curr_cam) delete curr_cam;
    curr_cam = new Cam;
    if (ui->TITLE->prog_settings->cameralink) curr_cam->cameralink = true;
    enable_controls(device_type = curr_cam->search_for_devices());
    ui->TITLE->prog_settings->enable_ip_editing(device_type == 1);
    if (device_type) {
        int ip, gateway;
        curr_cam->ip_address(true, &ip, &gateway);
        ui->TITLE->prog_settings->config_ip(true, ip, gateway);
    }
}

void Demo::on_START_BUTTON_clicked()
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
    ui->GAIN_SLIDER->setValue(0);
    connect(ui->GAIN_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_gain(int)));

    on_CONTINUOUS_RADIO_clicked();

    curr_cam->time_exposure(true, &time_exposure_edit);
    curr_cam->gain_analog(true, &gain_analog_edit);
    curr_cam->frame_rate(true, &frame_rate_edit);
    ui->GAIN_SLIDER->setValue((int)gain_analog_edit);
    ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)gain_analog_edit));
    ui->DUTY_EDIT->setText(QString::asprintf("%.3f", (time_exposure_edit) / 1000));
    ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.3f", frame_rate_edit));

    // adjust display size according to frame size
    curr_cam->get_frame_size(w, h);
    qInfo("frame w: %d, h: %d", w, h);
//    QRect region = ui->SOURCE_DISPLAY->geometry();
//    region.setHeight(ui->SOURCE_DISPLAY->width() * h / w);
//    ui->SOURCE_DISPLAY->setGeometry(region);
//    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
//    ui->SOURCE_DISPLAY->update_roi(QPoint());
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);

    ui->TITLE->prog_settings->set_pixel_format(0);
}

void Demo::on_SHUTDOWN_BUTTON_clicked()
{
    ui->STOP_GRABBING_BUTTON->click();
    shut_down();
    ui->IMG_ENHANCE_CHECK->setChecked(false);
    ui->FRAME_AVG_CHECK->setChecked(false);
    ui->IMG_3D_CHECK->setChecked(false);
    on_ENUM_BUTTON_clicked();
    clean();
}

void Demo::on_CONTINUOUS_RADIO_clicked()
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

void Demo::on_TRIGGER_RADIO_clicked()
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
void Demo::on_SOFTWARE_CHECK_stateChanged(int arg1)
{
    trigger_by_software = arg1;
    trigger_source = arg1 ? 1 : 2;
    curr_cam->trigger_source(false, &trigger_by_software);
    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(start_grabbing && arg1);
}

void Demo::on_SOFTWARE_TRIGGER_BUTTON_clicked()
{
    curr_cam->trigger_once();
}

void Demo::on_START_GRABBING_BUTTON_clicked()
{
    if (!device_on || start_grabbing || curr_cam == NULL) return;

    if (grab_thread_state || h_grab_thread) {
        grab_thread_state = false;
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
    }

    grab_thread_state = true;
    h_grab_thread = new GrabThread((void*)this);
    if (h_grab_thread == NULL) {
        grab_thread_state = false;
        QMessageBox::warning(this, "PROMPT", tr("Create thread fail"));
        return;
    }
    h_grab_thread->start();

    if (curr_cam->start_grabbing()) {
        grab_thread_state = false;
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
        return;
    }
    start_grabbing = true;
    ui->SOURCE_DISPLAY->grab = true;
    enable_controls(true);

#ifdef ICMOS
    ui->HIDE_BTN->click();
#endif
}

void Demo::on_STOP_GRABBING_BUTTON_clicked()
{
//    if (!device_on || !start_grabbing || curr_cam == NULL) return;

    if (record_original) on_SAVE_AVI_BUTTON_clicked();

    if (!img_q.empty()) std::queue<cv::Mat>().swap(img_q);

    grab_thread_state = false;
    if (h_grab_thread) {
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
    }

    start_grabbing = false;
    ui->SOURCE_DISPLAY->grab = false;
    QTimer::singleShot(10, this, SLOT(clean()));
    enable_controls(device_type);
    if (device_type == -1) ui->ENUM_BUTTON->click();

    curr_cam->stop_grabbing();
}

void Demo::on_SAVE_BMP_BUTTON_clicked()
{
    if (!QDir(save_location + "/ori_bmp").exists()) QDir().mkdir(save_location + "/ori_bmp");
    if (device_type == -1) {
        image_mutex.lock();
        save_original = 1;
        image_mutex.unlock();
        QThread::msleep(10);
        image_mutex.lock();
        save_original = 0;
        image_mutex.unlock();
    }
    else {
        save_original = !save_original;
        ui->SAVE_BMP_BUTTON->setText(save_original ? tr("Stop") : tr("ORI"));
    }
}

void Demo::on_SAVE_FINAL_BUTTON_clicked()
{
    static QString res_avi;
//    cv::imwrite(QString(save_location + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".jpg").toLatin1().data(), modified_result);
    if (record_modified) {
//        curr_cam->stop_recording(0);
        image_mutex.lock();
        vid_out[1].release();
        image_mutex.unlock();
        QString dest = save_location + "/" + res_avi.section('/', -1, -1);
        std::thread t(Demo::move_to_dest, QString(res_avi), QString(dest));
        t.detach();
//        QFile::rename(res_avi, dest);
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        image_mutex.lock();
        res_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_res.avi");
        vid_out[1].open(res_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w, h), image_3d);
        image_mutex.unlock();
    }
    record_modified = !record_modified;
    ui->SAVE_FINAL_BUTTON->setText(record_modified ? tr("Stop") : tr("RES"));
}

void Demo::on_SET_PARAMS_BUTTON_clicked()
{
    gain_analog_edit = ui->GAIN_EDIT->text().toFloat();
    duty = time_exposure_edit = ui->DUTY_EDIT->text().toFloat() * 1000;
    fps = frame_rate_edit = ui->CCD_FREQ_EDIT->text().toFloat();

    // CCD FREQUENCY
    communicate_display(0, convert_to_send_tcu(0x06, 1.25e8 / fps), 7, 1, false);

    // DUTY RATIO -> EXPO. TIME
    communicate_display(0, convert_to_send_tcu(0x07, duty * 1.25e2), 7, 1, false);

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

void Demo::on_GET_PARAMS_BUTTON_clicked()
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

void Demo::on_FILE_PATH_BROWSE_clicked()
{
    QString temp = QFileDialog::getExistingDirectory(this, tr("Select folder"), save_location);
    if (!temp.isEmpty()) save_location = temp;
    if (save_original && !QDir(temp + "/ori_bmp").exists()) QDir().mkdir(temp + "/ori_bmp");
    if (save_modified && !QDir(temp + "/res_bmp").exists()) QDir().mkdir(temp + "/res_bmp");

    ui->FILE_PATH_EDIT->setText(save_location);
}

void Demo::clean()
{
    ui->SOURCE_DISPLAY->clear();
    ui->DATA_EXCHANGE->clear();
    if (grab_thread_state) return;
    std::queue<cv::Mat>().swap(img_q);
    img_mem.release();
    modified_result.release();
    img_display.release();
    prev_img.release();
    prev_3d.release();
    for (auto& m: seq) m.release();
    seq_sum.release();
}

void Demo::setup_hz(int hz_unit)
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

void Demo::setup_stepping(int base_unit)
{
    this->base_unit = base_unit;
    switch (base_unit) {
    // ns
    case 0: ui->STEPPING_UNIT->setText("ns"); ui->STEPPING_EDIT->setText(QString::number((int)stepping)); break;
    // μs
    case 1: ui->STEPPING_UNIT->setText(QString::fromLocal8Bit("μs")); ui->STEPPING_EDIT->setText(QString::number(stepping / 1000, 'f', 2)); break;
    // m
    case 2: ui->STEPPING_UNIT->setText("m"); ui->STEPPING_EDIT->setText(QString::number(stepping * dist_ns, 'f', 2)); break;
    default: break;
    }
}

void Demo::setup_max_dist(int max_dist)
{
    this->max_dist = max_dist;
    ui->DELAY_SLIDER->setMaximum(max_dist);
}

// laser_on: 0b0101 -> laser[2] and laser[0] is on, laser[3] and laser[1] is off
void Demo::setup_laser(int laser_on)
{
    int change = laser_on ^ this->laser_on;
//    qDebug() << QString::number(laser_on, 2);
//    qDebug() << QString::number(change, 2);
    for(int i = 0; i < 4; i++) if ((change >> i) & 1) communicate_display(0, convert_to_send_tcu(0x1A + i, laser_on & (1 << i) ? 8 : 4), 7, 1, false);
    this->laser_on = laser_on;
}

void Demo::set_serial_port_share(bool share)
{
    share_serial_port = share;
}

void Demo::set_baudrate(int idx, int baudrate)
{
    if (serial_port[idx]->isOpen()) serial_port[idx]->setBaudRate(baudrate);
}

void Demo::com_write_data(int com_idx, QByteArray data)
{
    communicate_display(com_idx, data, data.length(), 0, true);
}

void Demo::display_baudrate(int idx)
{
    if (serial_port[idx]->isOpen()) ui->TITLE->prog_settings->display_baudrate(idx, serial_port[idx]->baudRate());
}

void Demo::set_auto_mcp(bool adaptive)
{
    this->auto_mcp = adaptive;
}

void Demo::set_dev_ip(int ip, int gateway)
{
    qDebug() << "modify ip: " << hex << curr_cam->ip_address(false, &ip, &gateway);
}

void Demo::change_pixel_format(int pixel_format)
{
    this->pixel_format = pixel_format;
    int ret = curr_cam->pixel_type(false, &pixel_format);
    is_color = pixel_format == PixelType_Gvsp_RGB8_Packed;
    switch (pixel_format) {
    case PixelType_Gvsp_Mono10: pixel_depth = 10; break;
    case PixelType_Gvsp_Mono12: pixel_depth = 12; break;
    default: pixel_depth = 8; break;
    }
}

void Demo::reset_frame_a()
{
    frame_a_3d ^= 1;
}

void Demo::joystick_button_pressed(int btn)
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
        else if (joybtn_A) on_LASER_ZOOM_IN_BTN_pressed(), lens_adjust_ongoing = 0b100;
        break;
    case JoystickThread::BUTTON::R1: joybtn_R1 = true; break;
    case JoystickThread::BUTTON::R2:
        joybtn_R2 = true;
        if      (joybtn_X) on_ZOOM_OUT_BTN_pressed(),       lens_adjust_ongoing = 0b001;
        else if (joybtn_Y) on_FOCUS_FAR_BTN_pressed(),      lens_adjust_ongoing = 0b010;
        else if (joybtn_A) on_LASER_ZOOM_OUT_BTN_pressed(), lens_adjust_ongoing = 0b100;
        break;
    case JoystickThread::BUTTON::HOME:
    case JoystickThread::BUTTON::SELECT:
    case JoystickThread::BUTTON::MINUS:
    case JoystickThread::BUTTON::PLUS:
    default: break;
    }
}

void Demo::joystick_button_released(int btn)
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
        if (lens_adjust_ongoing & 0b100) on_LASER_ZOOM_IN_BTN_released(), lens_adjust_ongoing = 0;
        break;
    case JoystickThread::BUTTON::R1: joybtn_R1 = false; break;
    case JoystickThread::BUTTON::R2:
        joybtn_R2 = false;
        if (lens_adjust_ongoing & 0b001) on_ZOOM_OUT_BTN_released(),       lens_adjust_ongoing = 0;
        if (lens_adjust_ongoing & 0b010) on_FOCUS_FAR_BTN_released(),      lens_adjust_ongoing = 0;
        if (lens_adjust_ongoing & 0b100) on_LASER_ZOOM_OUT_BTN_released(), lens_adjust_ongoing = 0;
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

void Demo::joystick_direction_changed(int direction)
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

void Demo::export_config()
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
        convert_write(out, TCU);
        convert_write(out, SCAN);
        convert_write(out, IMG);
        convert_write(out, TCU_PREF);
        config.close();
    }
    else {
        QMessageBox::warning(this, "PROMPT", tr("cannot create config file"));
    }
}

void Demo::prompt_for_config_file()
{
//    QString config_name = QFileDialog::getOpenFileName(this, tr("Load Configuration"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("*.ssy"));
    QString config_name = QFileDialog::getOpenFileName(this, tr("Load Configuration"), save_location, tr("YJS config(*.ssy);;All Files()"));
    load_config(config_name);
}

void Demo::load_config(QString config_name)
{
    if (config_name.isEmpty()) return;
    QFile config(config_name);
    config.open(QIODevice::ReadOnly);
    bool read_success = true;
    if (config.isOpen()) {
        QDataStream out(&config);
        read_success &= convert_read(out, WIN_PREF);
        read_success &= convert_read(out, TCU);
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
        communicate_display(0, convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
        ui->MCP_SLIDER->setValue(mcp);
        on_SET_PARAMS_BUTTON_clicked();
        ui->TITLE->prog_settings->data_exchange(false);
    }
    else {
        data_exchange(true);
        ui->TITLE->prog_settings->data_exchange(true);
        QMessageBox::warning(this, "PROMPT", tr("cannot read config file"));
    }
}

void Demo::prompt_for_serial_file()
{
    QString config_name = QFileDialog::getOpenFileName(this, tr("Load SN Config"), save_location, tr("(*.csv);;All Files()"));
    config_gatewidth(config_name);
}

// convert data to be sent to TCU-COM to hex buffer
QByteArray Demo::convert_to_send_tcu(uchar num, unsigned int send) {
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

inline QByteArray Demo::generate_ba(uchar *data, int len)
{
    QByteArray ba((char*)data, len);
    delete[] data;
    return ba;
}

// send and receive data from serial port, and display
QByteArray Demo::communicate_display(int id, QByteArray write, int write_size, int read_size, bool fb) {
    port_mutex.lock();
//    image_mutex.lock();
    QByteArray read;
    if (!use_tcp) {
        QString str_s("sent    "), str_r("received");

        for (char i = 0; i < write_size; i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
        emit append_text(str_s);

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
        QString str_s("sent    "), str_r("received");

        for (char i = 0; i < write_size; i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
        emit append_text(str_s);

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

void Demo::update_delay()
{
//    qDebug() << sender();
//    static QElapsedTimer t;
//    if (t.elapsed() < (fps > 9 ? 900 / fps : 100)) return;
//    t.start();

    // REPEATED FREQUENCY
    if (delay_dist < 0) delay_dist = 0;
    if (delay_dist > max_dist) delay_dist = max_dist;
//    qDebug("estimated distance: %f\n", delay_dist);

    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist));
    if (!qobject_cast<QSlider*>(sender())) ui->DELAY_SLIDER->setValue(delay_dist);

    int delay = std::round(delay_dist / dist_ns);
    delay_a_u = delay / 1000;
    delay_a_n = delay % 1000;
    delay_b_u = (delay + delay_n_n) / 1000;
    delay_b_n = (delay + delay_n_n) % 1000;

    if (scan) {
        rep_freq = ui->TITLE->prog_settings->rep_freq;
    }
    else {
        // change repeated frequency according to delay: rep frequency (kHz) <= 1s / delay (μs)
        if (ui->TITLE->prog_settings->auto_rep_freq) {
            rep_freq = delay_dist ? 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000) : 30;
            if (rep_freq > 30) rep_freq = 30;
//            if (rep_freq < 10) rep_freq = 10;
        }

        communicate_display(0, convert_to_send_tcu(0x00, 1.25e5 / rep_freq), 7, 1, false);
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
    communicate_display(0, convert_to_send_tcu(0x02, (delay_a_u * 1000 + delay_a_n) + offset_delay), 7, 1, false);

    // DELAY B
    communicate_display(0, convert_to_send_tcu(0x04, (delay_b_u * 1000 + delay_b_n) + offset_delay), 7, 1, false);

    setup_hz(hz_unit);
    ui->DELAY_A_EDIT_U->setText(QString::asprintf("%d", delay_a_u));
    ui->DELAY_A_EDIT_N->setText(QString::asprintf("%d", delay_a_n));
    ui->DELAY_B_EDIT_U->setText(QString::asprintf("%d", delay_b_u));
    ui->DELAY_B_EDIT_N->setText(QString::asprintf("%d", delay_b_n));
}

void Demo::update_gate_width() {
    static QElapsedTimer t;
    if (t.elapsed() < (fps > 9 ? 900 / fps : 100)) return;
    t.start();

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
    communicate_display(0, convert_to_send_tcu(0x03, gw_corrected), 7, 1, false);

    // GATE WIDTH B
//    send = gw - 18;
    communicate_display(0, convert_to_send_tcu(0x05, gw_corrected), 7, 1, false);

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

void Demo::filter_scan()
{
    static cv::Mat filter = img_mem.clone();
    filter = img_mem / 64;
//    qDebug("ratio %f\n", cv::countNonZero(filter) / (float)filter.total());
    if (cv::countNonZero(filter) / (float)filter.total() > 0.3) /*scan = !scan, */emit update_scan(true);
}

void Demo::update_current()
{
    if (!serial_port[3]) return;
//    QString send = "DIOD1:CURR " + ui->CURRENT_EDIT->text() + "\r";
    QString send = "PCUR " + ui->CURRENT_EDIT->text() + "\r";
    serial_port[3]->write(send.toLatin1().data(), send.length());
    serial_port[3]->waitForBytesWritten(100);
    serial_port[3]->readAll();
    qDebug() << send;
}

void Demo::convert_write(QDataStream &out, const int TYPE)
{
    data_exchange(true);
    out << uchar(0xAA) << uchar(0xAA);
    static ProgSettings *ps = ui->TITLE->prog_settings;
    switch (TYPE) {
    case WIN_PREF:
    {
        out << "WIN_PREF" << uchar('.');
        out << com_edit[0]->text().toInt() << com_edit[1]->text().toInt() << com_edit[2]->text().toInt() << com_edit[3]->text().toInt() << save_location.toUtf8().constData();
    }
    case TCU:
    {
        out << "TCU" << uchar('.');
        out << fps << duty << rep_freq << laser_width << mcp << delay_dist << delay_n_n << depth_of_view << stepping;
    }
    case SCAN:
    {
        out << "SCAN" << uchar('.');
        out << ps->start_pos << ps->end_pos << ps->frame_count << ps->step_size << ps->rep_freq << (int)(ps->save_scan_ori) << (int)(ps->save_scan_res);
    }
    case IMG:
    {
        out << "IMG" << uchar('.');
        out << ps->kernel << ps->gamma << ps->log << ps->low_in << ps->low_out << ps->high_in << ps->high_out << ps->dehaze_pct << ps->sky_tolerance << ps->fast_gf;
    }
    case TCU_PREF:
    {
        out << "TCU_PREF" << uchar('.');
        out << (int)(ps->auto_rep_freq) << ps->hz_unit << ps->base_unit << ps->max_dist;
    }
    default: break;
    }
    out << uchar('.') << uchar(0xFF) << uchar(0xFF);
}

bool Demo::convert_read(QDataStream &out, const int TYPE)
{
    uchar temp_uchar = 0x00;
    char *temp_str = (char*)malloc(100 * sizeof(char));
    memset(temp_str, 0, 100);
    out >> temp_uchar; if (temp_uchar != 0xAA) return false;
    out >> temp_uchar; if (temp_uchar != 0xAA) return false;
    static ProgSettings *ps = ui->TITLE->prog_settings;
    switch (TYPE) {
    case WIN_PREF:
    {
        out >> temp_str; if (std::strcmp(temp_str, "WIN_PREF")) return false;
        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
        int temp_int = 0;
        for (int i = 0; i < 4; i++) {
            out >> temp_int;
            com_edit[i]->setText(QString::number(temp_int));
        }
        out >> temp_str; save_location = QString::fromUtf8(temp_str);
    }
    case TCU:
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
        out >> ps->start_pos >> ps->end_pos >> ps->frame_count >> ps->step_size >> ps->rep_freq >> temp_bool;
        ps->save_scan_ori = temp_bool;
        out >> temp_bool;
        ps->save_scan_res = temp_bool;
    }
    case IMG:
    {
        out >> temp_str; if (std::strcmp(temp_str, "IMG")) return false;
        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
        out >> ps->kernel >> ps->gamma >> ps->log >> ps->low_in >> ps->low_out >> ps->high_in >> ps->high_out >> ps->dehaze_pct >> ps->sky_tolerance >> ps->fast_gf;
    }
    case TCU_PREF:
    {
        out >> temp_str; if (std::strcmp(temp_str, "TCU_PREF")) return false;
        out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
        int temp_bool;
        out >> temp_bool >> ps->hz_unit >> ps->base_unit >> ps->max_dist;
        ps->auto_rep_freq = temp_bool;
    }
    default: break;
    }
    out >> temp_uchar; if (temp_uchar != 0x2E /* '.' */) return false;
    out >> temp_uchar; if (temp_uchar != 0xFF) return false;
    out >> temp_uchar; if (temp_uchar != 0xFF) return false;
    free(temp_str);
    return true;
}

void Demo::read_gatewidth_lookup_table(QFile *fp)
{
    if (!fp || !fp->isOpen()) return;
    QList<QByteArray> line_sep;
    while (!fp->atEnd()) {
        line_sep = fp->readLine().split(',');
        gw_lut[line_sep[0].toInt()] = line_sep[1].toInt();
    }
}

void Demo::connect_to_serial_server_tcp()
{
    QString ip = "192.168.1.233";
    bool connected = false;
    tcp_port[0]->connectToHost(ip, 10001);
    tcp_port[2]->connectToHost(ip, 10002);
    connected = tcp_port[0]->waitForConnected(3000);
    connected = tcp_port[2]->waitForConnected(3000);
    use_tcp = connected;
    if (connected) QMessageBox::information(this, "serial port server", "connected to server");
    else           QMessageBox::information(this, "serial port server", "cannot connect to server");
}

void Demo::disconnect_from_serial_server_tcp()
{
    for (int i = 0; i < 4; i++) {
        tcp_port[i]->disconnectFromHost();
    }
    QMessageBox::information(this, "serial port server", "disconnected from server");
    use_tcp = false;
}

void Demo::on_DIST_BTN_clicked() {
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
    delay_dist = distance;
    update_delay();
//    update_gate_width();
//    change_mcp(150);

    rep_freq = 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000);
    if (rep_freq > 30) rep_freq = 30;
    if      (distance < 1000) depth_of_view =  500 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);
    else if (distance < 3000) depth_of_view = 1000 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);
    else if (distance < 6000) depth_of_view = 2000 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);
    else                      depth_of_view = 3500 * dist_ns, laser_width = std::round(depth_of_view / dist_ns);

    communicate_display(0, convert_to_send_tcu(0x1E, distance), 7, 1, true);
    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist));

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

void Demo::on_IMG_3D_CHECK_stateChanged(int arg1)
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

void Demo::on_ZOOM_IN_BTN_pressed()
{
    ui->ZOOM_IN_BTN->setText("x");

    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x40, 0x00, 0x00, 0x41}, 7), 7, 1, false);
}

void Demo::on_ZOOM_OUT_BTN_pressed()
{
    ui->ZOOM_OUT_BTN->setText("x");

    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x20, 0x00, 0x00, 0x21}, 7), 7, 1, false);
}

void Demo::on_FOCUS_NEAR_BTN_pressed()
{
    ui->FOCUS_NEAR_BTN->setText("x");
    focus_near();
}

inline void Demo::focus_near()
{
    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
}

inline void Demo::set_laser_preset_target(int *pos)
{
    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x81, uchar((pos[0] >> 8) & 0xFF), uchar(pos[0] & 0xFF), uchar((((pos[0] >> 8) & 0xFF) + (pos[0] & 0xFF) + 0x82) & 0xFF)}, 7), 7, 1, false);
    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4F, uchar((pos[1] >> 8) & 0xFF), uchar(pos[1] & 0xFF), uchar((((pos[1] >> 8) & 0xFF) + (pos[1] & 0xFF) + 0x51) & 0xFF)}, 7), 7, 1, false);
    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x4E, uchar((pos[2] >> 8) & 0xFF), uchar(pos[2] & 0xFF), uchar((((pos[2] >> 8) & 0xFF) + (pos[2] & 0xFF) + 0x50) & 0xFF)}, 7), 7, 1, false);
    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x81, uchar((pos[3] >> 8) & 0xFF), uchar(pos[3] & 0xFF), uchar((((pos[3] >> 8) & 0xFF) + (pos[3] & 0xFF) + 0x83) & 0xFF)}, 7), 7, 1, false);
    delete[] pos;
}

void Demo::goto_laser_preset(char target)
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

void Demo::on_FOCUS_FAR_BTN_pressed()
{
    ui->FOCUS_FAR_BTN->setText("x");
    focus_far();
}

inline void Demo::focus_far()
{
    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x80, 0x00, 0x00, 0x81}, 7), 7, 1, false);
}

inline void Demo::lens_stop() {
    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
    if (multi_laser_lenses) communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02}, 7), 7, 1, false);
}

void Demo::set_zoom()
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

    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, QByteArray((char*)out, 7), 7, 1, false);
}

void Demo::set_focus()
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

    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, QByteArray((char*)out, 7), 7, 1, false);
}


void Demo::start_laser()
{
    communicate_display(0, generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 7, 0, false);
    QTimer::singleShot(100000, this, SLOT(init_laser()));
}

void Demo::init_laser()
{
    ui->LASER_BTN->setEnabled(true);
    ui->LASER_BTN->setText(tr("OFF"));
    ui->CURRENT_EDIT->setEnabled(true);
    if (serial_port[3]) serial_port[3]->close();
    setup_serial_port(serial_port + 3, 3, com_edit[3]->text(), 9600);
    if (!serial_port[3]) return;

    // enable laser com
    serial_port[3]->write("MODE:RMT 1\r", 11);
    serial_port[3]->waitForBytesWritten(100);
    QThread::msleep(100);
    serial_port[3]->readAll();
    qDebug() << QString("MODE:RMT 1\r");

    // start
    serial_port[3]->write("ON\r", 3);
    serial_port[3]->waitForBytesWritten(100);
    QThread::msleep(100);
    serial_port[3]->readAll();
    qDebug() << QString("ON\r");

    // enable external trigger
    serial_port[3]->write("QSW:PRF 0\r", 10);
    serial_port[3]->waitForBytesWritten(100);
    QThread::msleep(100);
    serial_port[3]->readAll();
    qDebug() << QString("QSW:PRF 0\r");

    update_current();
}

void Demo::change_mcp(int val)
{
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    if (val == mcp) return;
    mcp = val;

//    convert_to_send_tcu(0x0A, mcp);
//    static QElapsedTimer t;
//    if (!h_grab_thread || t.elapsed() > (fps > 9 ? 900 / fps : 100)) communicate_display(0, convert_to_send_tcu(0x0A, mcp), 7, 1, false), t.start();
    communicate_display(0, convert_to_send_tcu(0x0A, mcp), 7, 1, false);

    ui->MCP_EDIT->setText(QString::number(val));
    ui->MCP_SLIDER->setValue(mcp);
}

void Demo::change_gain(int val)
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

void Demo::change_delay(int val)
{
    if (abs(delay_dist - val) < 1) return;
    delay_dist = val;
    update_delay();
}

void Demo::change_focus_speed(int val)
{
    if (val < 1)  val = 1;
    if (val > 64) val = 64;
    ui->FOCUS_SPEED_EDIT->setText(QString::asprintf("%d", val));
    ui->FOCUS_SPEED_SLIDER->setValue(val);
    if (val > 1) val -= 1;

    static QElapsedTimer t;
    if (t.elapsed() < 100) return;
    t.restart();

    int temp_serial_port_id = share_serial_port && serial_port[0]->isOpen() ? 0 : 2;

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
    communicate_display(temp_serial_port_id, QByteArray((char*)out_data, 9), 9, 0, false);

    out_data[0] = 0xB0;
    out_data[1] = 0x02;
    out_data[2] = 0xA0;
    out_data[3] = 0x01;
    out_data[4] = val;
    out_data[5] = val;
    out_data[6] = val;
    out_data[7] = val;

    out_data[8] = (4 * (uint)val + 0xA3) & 0xFF;
    communicate_display(temp_serial_port_id, QByteArray((char*)out_data, 9), 9, 0, false);
}

void Demo::on_ZOOM_IN_BTN_released()
{
    ui->ZOOM_IN_BTN->setText("+");
    lens_stop();
}

void Demo::on_ZOOM_OUT_BTN_released()
{
    ui->ZOOM_OUT_BTN->setText("-");
    lens_stop();
}

void Demo::on_FOCUS_NEAR_BTN_released()
{
    ui->FOCUS_NEAR_BTN->setText("+");
    lens_stop();
}

void Demo::on_FOCUS_FAR_BTN_released()
{
    ui->FOCUS_FAR_BTN->setText("-");
    lens_stop();
}

void Demo::keyPressEvent(QKeyEvent *event)
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
                    setup_serial_port(serial_port + i, i, edit->text(), 9600);
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
                communicate_display(0, convert_to_send_tcu(0x00, 1.25e5 / rep_freq), 7, 1, false);
            }
            else if (edit == ui->GATE_WIDTH_A_EDIT_U) {
                depth_of_view = (edit->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt()) * dist_ns;
                update_gate_width();
            }
            else if (edit == ui->LASER_WIDTH_EDIT_U) {
                laser_width = edit->text().toInt() * 1000 + ui->LASER_WIDTH_EDIT_N->text().toInt();
                communicate_display(0, convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
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
                communicate_display(0, convert_to_send_tcu(0x01, (laser_width + offset_laser_width) / 8), 7, 1, false);
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
            ui->TITLE->prog_settings->show();
            ui->TITLE->prog_settings->raise();
            break;
        case Qt::Key_E:
            export_config();
            break;
        case Qt::Key_R:
            prompt_for_config_file();
            break;
        case Qt::Key_Q:
            use_tcp ? disconnect_from_serial_server_tcp() : connect_to_serial_server_tcp();
            break;
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
    default: break;
    }
}

void Demo::resizeEvent(QResizeEvent *event)
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
    ui->DRAG_TOOL->move(region.x(), 15);
    ui->SELECT_TOOL->move(region.x() + 30, 15);
    ui->CURR_COORD->move(region.x() + 80, 5);
    ui->START_COORD->move(ui->CURR_COORD->geometry().right() + 20, 5);
    ui->SHAPE_INFO->move(ui->START_COORD->geometry().right() + 20, 5);
    ui->INFO_CHECK->move(region.right() - 60, 0);
    ui->CENTER_CHECK->move(region.right() - 60, 20);
    ui->HIDE_BTN->move(ui->MID->geometry().left() - 8, this->geometry().height() / 2 - 10);
//    ui->HIDE_BTN->move(ui->LEFT->geometry().right() - 10 + (ui->SOURCE_DISPLAY->geometry().left() + ui->MID->geometry().left() - ui->LEFT->geometry().right() + 10) / 2 + 2, this->geometry().height() / 2 - 10);
    ui->RULER_H->setGeometry(region.left(), region.bottom() - 10, region.width(), 32);
    ui->RULER_V->setGeometry(region.right() - 10, region.top(), 32, region.height());
    ui->FISHNET_RESULT->move(region.right() - 170, 5);

    image_mutex.lock();
    ui->SOURCE_DISPLAY->setGeometry(region);
    ui->SOURCE_DISPLAY->update_roi(center);
    qInfo("display region x: %d, y: %d, w: %d, h: %d", region.x(), region.y(), region.width(), region.height());
    image_mutex.unlock();

    event->accept();
}

void Demo::mousePressEvent(QMouseEvent *event)
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

void Demo::mouseMoveEvent(QMouseEvent *event)
{
    QMainWindow::mouseMoveEvent(event);
    if (ui->TITLE->is_maximized) return;

    static QCursor pointer = QCursor(QPixmap(":/cursor/cursor").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation), 0, 0),
                   resize_h = QCursor(QPixmap(":/cursor/resize_h").scaled(20, 20)),
                   resize_v = QCursor(QPixmap(":/cursor/resize_v").scaled(20, 20)),
                   resize_md = QCursor(QPixmap(":/cursor/resize_md").scaled(16, 16)),
                   resize_sd = QCursor(QPixmap(":/cursor/resize_sd").scaled(16, 16));

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
        if (diff.y() < 30 && diff.x() > width() - 5) setCursor(pointer);
        // cursor in main window
        else if (diff.y() < 5) {
            // cursor @ topleft corner
            if (diff.x() < 5) setCursor(resize_md);
            // cursor on min, max, exit button
            else if (diff.x() > width() - 120) setCursor(pointer);
            // cursor @ top border
            else setCursor(resize_v);
        }
        else if (diff.y() > height() - 5) {
            // cursor @ bottom left corner
            if (diff.x() < 5) setCursor(resize_sd);
            // cursor @ bottom right corner
            else if (diff.x() > width() - 5) setCursor(resize_md);
            // cursor @ bottom border
            else setCursor(resize_v);
        }
        else {
            // cursor @ left border
            if (diff.x() < 5) setCursor(resize_h);
            // cursor @ right border
            else if (diff.x() > width() - 5) setCursor(resize_h);
            // cursor inside window
            else setCursor(pointer);
        }
    }
}

void Demo::mouseReleaseEvent(QMouseEvent *event)
{
    QMainWindow::mouseReleaseEvent(event);

    if (event->button() != Qt::LeftButton) return;
    mouse_pressed = false;
    resize_place = 22;
}

void Demo::mouseDoubleClickEvent(QMouseEvent *event)
{
    QMainWindow::mouseDoubleClickEvent(event);

    mouse_pressed = false;
}

void Demo::dragEnterEvent(QDragEnterEvent *event)
{
    QMainWindow::dragEnterEvent(event);

    if (event->mimeData()->urls().length() < 3) event->acceptProposedAction();
}

void Demo::dropEvent(QDropEvent *event)
{
    QMainWindow::dropEvent(event);

    if (event->mimeData()->urls().length() == 1) {
        QString file_name = event->mimeData()->urls().first().toLocalFile();
        if (file_name.endsWith(".bmp", Qt::CaseInsensitive) || file_name.endsWith(".png", Qt::CaseInsensitive) || file_name.endsWith(".tif", Qt::CaseInsensitive)) {
            if (device_on) {
                QMessageBox::warning(this, "PROMPT", tr("Cannot read local image while cam is on"));
                return;
            }
            QImage temp_img;
            temp_img.load(file_name);
            start_static_display(temp_img);

            img_q.push(cv::Mat(h, w, CV_8UC1, (uchar*)temp_img.bits(), temp_img.bytesPerLine()).clone());
        }
        else load_config(file_name);
    }
    else {
        QString file_name1 = event->mimeData()->urls()[0].toLocalFile();
        QString file_name2 = event->mimeData()->urls()[1].toLocalFile();
        if ((file_name1.endsWith(".bmp", Qt::CaseInsensitive) || file_name1.endsWith(".png", Qt::CaseInsensitive) || file_name1.endsWith(".tif", Qt::CaseInsensitive)) &&
            (file_name2.endsWith(".bmp", Qt::CaseInsensitive) || file_name2.endsWith(".png", Qt::CaseInsensitive) || file_name2.endsWith(".tif", Qt::CaseInsensitive))) {
            if (device_on) {
                QMessageBox::warning(this, "PROMPT", tr("Cannot read local image while cam is on"));
                return;
            }
            QImage temp_img1, temp_img2;
            temp_img1.load(file_name1);
            temp_img2.load(file_name2);
            if (temp_img1.format() != QImage::Format_Indexed8) temp_img1 = temp_img1.convertToFormat(QImage::Format_Indexed8);
            if (temp_img2.format() != QImage::Format_Indexed8) temp_img2 = temp_img2.convertToFormat(QImage::Format_Indexed8);
            if (temp_img1.width() == temp_img2.width() && temp_img1.height() == temp_img2.height() && temp_img1.depth() == temp_img2.depth()) start_static_display(temp_img1);
            else {
                QMessageBox::warning(this, "PROMPT", tr("Multiple images need to have the same size"));
                return;
            }
            cv::Mat img1(h, w, CV_8UC1, (uchar*)temp_img1.bits(), temp_img1.bytesPerLine());
            cv::Mat img2(h, w, CV_8UC1, (uchar*)temp_img2.bits(), temp_img2.bytesPerLine());

            img_q.push(img1.clone());
            img_q.push(img2.clone());
        }
        else {
            QMessageBox::warning(this, "PROMPT", tr("Cannot read selected image files"));
        }
    }
}

void Demo::start_static_display(QImage img)
{
    w = img.width();
    h = img.height();
    is_color = false;
    pixel_depth = img.depth();
    device_type = -1;

    if (grab_thread_state || h_grab_thread) {
        grab_thread_state = false;
        if (h_grab_thread) {
            h_grab_thread->quit();
            h_grab_thread->wait();
            delete h_grab_thread;
            h_grab_thread = NULL;
        }
        clean();
    }

    grab_thread_state = true;
    h_grab_thread = new GrabThread((void*)this);
    if (h_grab_thread == NULL) {
        grab_thread_state = false;
        QMessageBox::warning(this, "PROMPT", tr("Cannot display image"));
        return;
    }
    h_grab_thread->start();
    start_grabbing = true;
    enable_controls(true);
    ui->START_BUTTON->setEnabled(false);
}

void Demo::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (this->focusWidget()) this->focusWidget()->clearFocus();
    this->setFocus();
//    qDebug() << this->focusWidget();
}

void Demo::on_SAVE_AVI_BUTTON_clicked()
{
    static QString raw_avi;
    if (record_original) {
//        curr_cam->stop_recording(0);
        image_mutex.lock();
        vid_out[0].release();
        image_mutex.unlock();
        QString dest = save_location + "/" + raw_avi.section('/', -1, -1);
        std::thread t(Demo::move_to_dest, QString(raw_avi), QString(dest));
        t.detach();
//        QFile::rename(raw_avi, dest);
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "/" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        image_mutex.lock();
        raw_avi = QString(TEMP_SAVE_LOCATION + "/" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_raw.avi");
        vid_out[0].open(raw_avi.toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(image_3d ? w - 104 : w, h), false);
        image_mutex.unlock();
    }
    record_original = !record_original;
    ui->SAVE_AVI_BUTTON->setText(record_original ? tr("Stop") : tr("ORI"));
}

void Demo::on_SCAN_BUTTON_clicked()
{
    static ProgSettings *settings = ui->TITLE->prog_settings;
    bool start_scan = ui->SCAN_BUTTON->text() == tr("Scan");

    while (updated);

    if (start_scan) {
        rep_freq = settings->rep_freq;
        delay_dist = settings->start_pos * dist_ns;
        scan_stopping_delay = settings->end_pos;
        scan_step = settings->step_size * dist_ns;

        if (ui->TITLE->prog_settings->save_scan_ori || ui->TITLE->prog_settings->save_scan_res) {
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
                                           settings->start_pos, settings->end_pos, settings->frame_count, settings->step_size,
                                           (int)rep_freq, laser_width, gate_width_a_u * 1000 + gate_width_a_n, mcp).toUtf8());
            params.close();
        }

        on_CONTINUE_SCAN_BUTTON_clicked();
    }
    else {
        emit update_scan(false);
        scan = false;

//        for (uint i = scan_q.size(); i; i--) qDebug("dist %f", scan_q.front()), scan_q.pop_front();
    }

    ui->SCAN_BUTTON->setText(start_scan ? tr("Pause") : tr("Scan"));
}

void Demo::on_CONTINUE_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);

    update_delay();
    communicate_display(0, convert_to_send_tcu(0x00, (uint)(1.25e5 / rep_freq)), 7, 1, false);
}

void Demo::on_RESTART_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);
    delay_dist = 200;

    on_CONTINUE_SCAN_BUTTON_clicked();
}

void Demo::append_data(QString text)
{
    ui->DATA_EXCHANGE->append(text);
}

void Demo::enable_scan_options(bool show)
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

void Demo::on_FRAME_AVG_CHECK_stateChanged(int arg1)
{
    if (arg1) {
        seq_sum.release();
        for (int i = 0; i < 10; i++) seq[i].release();
        seq_idx = 0;
    }
    calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 5 + 5;
}

void Demo::on_SAVE_RESULT_BUTTON_clicked()
{
    if (!QDir(save_location + "/res_bmp").exists()) QDir().mkdir(save_location + "/res_bmp");
    if (device_type == -1) {
        image_mutex.lock();
        save_modified = 1;
        image_mutex.unlock();
        QThread::msleep(10);
        image_mutex.lock();
        save_modified = 0;
        image_mutex.unlock();
    }
    else{
        save_modified = !save_modified;
        ui->SAVE_RESULT_BUTTON->setText(save_modified ? tr("Stop") : tr("RES"));
    }
}

void Demo::on_LASER_ZOOM_IN_BTN_pressed()
{
    ui->LASER_ZOOM_IN_BTN->setText("x");

    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
    if (multi_laser_lenses) communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x20, 0x00, 0x00, 0x22}, 7), 7, 1, false);
    if (multi_laser_lenses) communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x01, 0x00, 0x00, 0x00, 0x03}, 7), 7, 1, false);
    if (multi_laser_lenses) communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04}, 7), 7, 1, false);
}

void Demo::on_LASER_ZOOM_OUT_BTN_pressed()
{
    ui->LASER_ZOOM_OUT_BTN->setText("x");

    communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x04, 0x00, 0x00, 0x00, 0x05}, 7), 7, 1, false);
    if (multi_laser_lenses) communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x40, 0x00, 0x00, 0x42}, 7), 7, 1, false);
    if (multi_laser_lenses) communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x00, 0x80, 0x00, 0x00, 0x82}, 7), 7, 1, false);
    if (multi_laser_lenses) communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x02, 0x04, 0x00, 0x00, 0x00, 0x06}, 7), 7, 1, false);
}

void Demo::on_LASER_ZOOM_IN_BTN_released()
{
    ui->LASER_ZOOM_IN_BTN->setText("+");
    lens_stop();
}

void Demo::on_LASER_ZOOM_OUT_BTN_released()
{
    ui->LASER_ZOOM_OUT_BTN->setText("-");
    lens_stop();
}

void Demo::laser_preset_reached()
{
    lens_stop();
    curr_laser_idx = -curr_laser_idx;
}

void Demo::on_LASER_BTN_clicked()
{
    if (ui->LASER_BTN->text() == "ON") {
        ui->LASER_BTN->setEnabled(false);
        communicate_display(0, generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99}, 7), 7, 0, false);
        QTimer::singleShot(4000, this, SLOT(start_laser()));
    }
    else {
        if (serial_port[3] && serial_port[3]->isOpen()) {
            serial_port[3]->write("OFF\r", 10);
            serial_port[3]->waitForBytesWritten(100);
            QThread::msleep(100);
            serial_port[3]->readAll();
        }
        communicate_display(0, generate_ba(new uchar[7]{0x88, 0x08, 0x00, 0x00, 0x00, 0x02, 0x99}, 7), 7, 0, false);

        ui->LASER_BTN->setText(tr("ON"));
        ui->CURRENT_EDIT->setEnabled(false);
    }
}

void Demo::on_GET_LENS_PARAM_BTN_clicked()
{
    QByteArray read = communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56}, 7), 7, 7, true);
    zoom = ((read[4] & 0xFF) << 8) + read[5] & 0xFF;

    read = communicate_display(share_serial_port && serial_port[0]->isOpen() ? 0 : 2, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57}, 7), 7, 7, true);
    focus = ((read[4] & 0xFF) << 8) + read[5] & 0xFF;

    ui->ZOOM_EDIT->setText(QString::asprintf("%d", zoom));
    ui->FOCUS_EDIT->setText(QString::asprintf("%d", focus));
}

void Demo::on_AUTO_FOCUS_BTN_clicked()
{
    focus_direction = 1;
    focus_far();
}

void Demo::on_HIDE_BTN_clicked()
{
    hide_left ^= 1;
    ui->LEFT->setGeometry(10, 40, hide_left ? 0 : 210, 631);
//    ui->HIDE_BTN->setText(hide_left ? ">" : "<");
    ui->HIDE_BTN->setStyleSheet(QString::asprintf("padding: 2px; image: url(:/tools/%s);", hide_left ? "right" : "left"));
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
}

void Demo::on_COM_DATA_RADIO_clicked()
{
    display_option = 0;
    ui->DATA_EXCHANGE->show();
    ui->HIST_DISPLAY->hide();
    ui->PTZ_GRP->hide();
}

void Demo::on_HISTOGRAM_RADIO_clicked()
{
    display_option = 1;
    ui->DATA_EXCHANGE->hide();
    ui->HIST_DISPLAY->show();
    ui->PTZ_GRP->hide();
}

void Demo::on_PTZ_RADIO_clicked()
{
    display_option = 2;
    ui->DATA_EXCHANGE->hide();
    ui->HIST_DISPLAY->hide();
    ui->PTZ_GRP->show();
}

void Demo::screenshot()
{
    image_mutex.lock();
    window()->grab().save(save_location + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    image_mutex.unlock();
}

void Demo::on_DRAG_TOOL_clicked()
{
    ui->SELECT_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->drag = true;
}

void Demo::on_SELECT_TOOL_clicked()
{
    ui->DRAG_TOOL->setChecked(false);
    ui->SOURCE_DISPLAY->drag = false;
}

void Demo::on_FILE_PATH_EDIT_editingFinished()
{
    QString temp_location = ui->FILE_PATH_EDIT->text();
    if (QDir(temp_location).exists() || QDir().mkdir(temp_location)) save_location = temp_location;
    else QMessageBox::warning(this, "PROMPT", tr("cannot create directory"));
    ui->FILE_PATH_EDIT->setText(save_location);
}

void Demo::on_BINNING_CHECK_stateChanged(int arg1)
{
    int binning = arg1 ? 2 : 1;
    curr_cam->binning(false, &binning);
    curr_cam->get_frame_size(w, h);
    qInfo("frame w: %d, h: %d", w, h);
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
}

void Demo::transparent_transmission_file(int id)
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

void Demo::config_gatewidth(QString filename)
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

void Demo::send_ctrl_cmd(uchar dir)
{
    uchar buffer_out[7];
    buffer_out[0] = 0xFF;
    buffer_out[1] = 0x01;
    buffer_out[2] = 0x00;
    buffer_out[3] = dir;
    buffer_out[4] = dir < 5 ? ptz_speed : 0x00;
    buffer_out[5] = dir > 5 ? ptz_speed : 0x00;
    int sum = 0;
    for (int i = 1; i < 6; i++) sum += buffer_out[i];
    buffer_out[6] = sum & 0xFF;

    communicate_display(4, QByteArray((char*)buffer_out, 7), 7, 1, false);
}

void Demo::ptz_button_pressed(int id) {
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

void Demo::ptz_button_released(int id) {
//    qDebug("%dr", id);
    if (id == 4) return;
    communicate_display(4, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
}

void Demo::on_PTZ_SPEED_SLIDER_valueChanged(int value)
{
    ptz_speed = value;
    ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
}

void Demo::on_PTZ_SPEED_EDIT_editingFinished()
{
    ptz_speed = ui->PTZ_SPEED_EDIT->text().toInt();
    if (ptz_speed > 64) ptz_speed = 64;
    if (ptz_speed < 1)  ptz_speed = 1;
    ui->PTZ_SPEED_SLIDER->setValue(ptz_speed);
    ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
}

void Demo::on_STOP_BTN_clicked()
{
    communicate_display(4, generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}, 7), 7, 1, false);
}


void Demo::on_DUAL_LIGHT_BTN_clicked()
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

void Demo::display_fishnet_result(int result)
{
    switch (result) {
    case -1: ui->FISHNET_RESULT->setText("FISHNET<br>???"), ui->FISHNET_RESULT->setStyleSheet("color: #B0C4DE;");            break;
    case  0: ui->FISHNET_RESULT->setText("FISHNET<br>DOES NOT EXIST"), ui->FISHNET_RESULT->setStyleSheet("color: #CD5C5C;"); break;
    case  1: ui->FISHNET_RESULT->setText("FISHNET<br>EXISTS"), ui->FISHNET_RESULT->setStyleSheet("color: #B0C4DE;");         break;
    default: return;
    }
}
