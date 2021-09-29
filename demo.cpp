#include "demo.h"
#ifdef PARAM
    #include "./ui_demo_dev.h"
#else
    #include "./ui_demo_rls.h"
#endif

GrabThread::GrabThread(void *info)
{
    p_info = info;
}

void GrabThread::run()
{
    ((Demo*)p_info)->grab_thread_process();
}

MouseThread::MouseThread(void *info) : QThread()
{
    p_info = info;
    t = NULL;
    connect(this, SIGNAL(set_cursor(QCursor)), (Demo*)info, SLOT(draw_cursor(QCursor)));
}

void MouseThread::run()
{
    if (!t) draw_cursor(), t = new QTimer();
    connect(t, SIGNAL(timeout()), this, SLOT(draw_cursor()), Qt::QueuedConnection);
    t->start(5);
    this->exec();
}

void MouseThread::draw_cursor()
{
    Demo* ptr = (Demo*)p_info;
    if (QApplication::mouseButtons() == Qt::LeftButton || ptr->is_maximized()) return;
//    QWidget *w = QApplication::widgetAt(ptr->cursor().pos());
    QPoint diff = ptr->cursor().pos() - ptr->pos();
    if (diff.y() < 30 && diff.x() > ptr->width() - 5) emit set_cursor(QCursor(QPixmap(":/cursor/cursor.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation), 0, 0));
    else if (diff.y() < 5) {
        if (diff.x() < 5) emit set_cursor(QCursor(QPixmap(":/cursor/resize_md.png").scaled(16, 16)));
        else if (diff.x() > ptr->width() - 120) emit set_cursor(QCursor(QPixmap(":/cursor/cursor.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation), 0, 0));
        else emit set_cursor(QCursor(QPixmap(":/cursor/resize_v.png").scaled(20, 20)));
    }
    else if (diff.y() > ptr->height() - 5) {
        if (diff.x() < 5) emit set_cursor(QCursor(QPixmap(":/cursor/resize_sd.png").scaled(16, 16)));
        else if (diff.x() > ptr->width() - 5) emit set_cursor(QCursor(QPixmap(":/cursor/resize_md.png").scaled(16, 16)));
        else emit set_cursor(QCursor(QPixmap(":/cursor/resize_v.png").scaled(20, 20)));
    }
    else {
        if (diff.x() < 5) emit set_cursor(QCursor(QPixmap(":/cursor/resize_h.png").scaled(20, 20)));
        else if (diff.x() > ptr->width() - 5) emit set_cursor(QCursor(QPixmap(":/cursor/resize_h.png").scaled(20, 20)));
        else emit set_cursor(QCursor(QPixmap(":/cursor/cursor.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation), 0, 0));
    }
}

Demo* wnd;

Demo::Demo(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Demo)
    , calc_avg_option(5)
    , range_threshold(0)
    , trigger_by_software(false)
    , curr_cam(NULL)
    , time_exposure_edit(5000)
    , gain_analog_edit(0)
    , frame_rate_edit(10)
    , save_location("")
    , com{NULL}
    , out_buffer{0}
    , in_buffer{0}
    , rep_freq(30)
    , laser_width_u(0)
    , laser_width_n(500)
    , delay_n_n(0)
    , stride(10)
    , fps(10)
    , duty(5000)
    , mcp(5)
    , zoom(0)
    , focus(0)
    , distance(0)
    , delay_dist(15)
    , depth_of_vision(15)
    , focus_direction(0)
    , display_option(1)
    , device_on(false)
    , start_grabbing(false)
    , record_original(false)
    , record_modified(false)
    , save_original(false)
    , save_modified(false)
    , image_3d(false)
    , w(640)
    , h(400)
    , h_grab_thread(NULL)
    , grab_thread_state(false)
    , h_mouse_thread(NULL)
    , seq_idx(0)
    , accu_idx(0)
    , scan(false)
    , scan_distance(200)
    , c(3e8)
    , frame_a_3d(false)
    , hide_left(false)
    , resize_place(22)
    , en(false)
    , mouse_pressed(false)
{
    ui->setupUi(this);
    wnd = this;

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    trans.load("zh.qm");
    h_mouse_thread = new MouseThread(this);
    h_mouse_thread->start();

    dist_ns = c * 1e-9 / 2;

    // initialization
    // - default save path
    save_location += "C:\\Users\\";
    save_location += QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section("/", -1, -1);
    save_location += "\\Pictures";

    // - image operations
    QComboBox *enhance_options = ui->ENHANCE_OPTIONS;
    QComboBox *sp_options = ui->SP_OPTIONS;
    QComboBox *calc_avg_options = ui->FRAME_AVG_OPTIONS;
    enhance_options->addItem(tr("None"));
    enhance_options->addItem(tr("Histogram"));
    enhance_options->addItem(tr("Laplace"));
    enhance_options->addItem(tr("Log-based"));
    enhance_options->addItem(tr("Gamma-based"));
    enhance_options->addItem(tr("Accumulative"));
    enhance_options->addItem(tr("Custom"));
    enhance_options->setCurrentIndex(0);
    sp_options->addItem("1");
    sp_options->addItem("2");
    sp_options->addItem("3");
    sp_options->addItem("4");
    sp_options->addItem("5");
    sp_options->addItem("6");
    sp_options->addItem("7");
    sp_options->setCurrentIndex(4);
    calc_avg_options->addItem("a");
    calc_avg_options->addItem("b");
    calc_avg_options->setCurrentIndex(0);

    // - write default params
    data_exchange(false);
    enable_controls(false);

    // - set-up COMs
//    foreach (const QSerialPortInfo & info, QSerialPortInfo::availablePorts()) qDebug("%s\n", qPrintable(info.portName()));
    com_label[0] = ui->TCU_COM;
    com_label[1] = ui->RANGE_COM;
    com_label[2] = ui->FOCUS_COM;
    com_label[3] = ui->LASER_COM;

    com_edit[0] = ui->TCU_COM_EDIT;
    com_edit[1] = ui->RANGE_COM_EDIT;
    com_edit[2] = ui->FOCUS_COM_EDIT;
    com_edit[3] = ui->LASER_COM_EDIT;

    setup_com(com + 0, 0, com_edit[0]->text(), 9600);
    setup_com(com + 1, 1, com_edit[1]->text(), 115200);
    setup_com(com + 2, 2, com_edit[2]->text(), 9600);
    setup_com(com + 3, 3, com_edit[3]->text(), 9600);

    // - set up sliders
    ui->MCP_SLIDER->setMinimum(0);
    ui->MCP_SLIDER->setMaximum(255);
    ui->MCP_SLIDER->setSingleStep(1);
    ui->MCP_SLIDER->setPageStep(10);
    ui->MCP_SLIDER->setValue(5);
    connect(ui->MCP_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_mcp(int)));

    ui->DELAY_SLIDER->setMinimum(0);
    ui->DELAY_SLIDER->setMaximum(15000);
    ui->DELAY_SLIDER->setSingleStep(10);
    ui->DELAY_SLIDER->setPageStep(100);
    ui->DELAY_SLIDER->setValue(0);
    connect(ui->DELAY_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_delay(int)));

    ui->FOCUS_SPEED_SLIDER->setMinimum(1);
    ui->FOCUS_SPEED_SLIDER->setMaximum(64);
    ui->FOCUS_SPEED_SLIDER->setSingleStep(1);
    ui->FOCUS_SPEED_SLIDER->setPageStep(4);
    ui->FOCUS_SPEED_SLIDER->setValue(5);
    connect(ui->FOCUS_SPEED_SLIDER, SIGNAL(valueChanged(int)), SLOT(change_focus_speed(int)), Qt::QueuedConnection);

    ui->CONTINUE_SCAN_BUTTON->hide();
    ui->RESTART_SCAN_BUTTON->hide();

    // connect QLineEdit, QButton signals used in thread
    connect(this, SIGNAL(append_text(QString)), SLOT(append_data(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(update_scan(bool)), SLOT(enable_scan_options(bool)), Qt::QueuedConnection);
//    connect(this, &Demo::appendText, &Demo::append_data);
    connect(this, SIGNAL(update_delay_in_thread()), SLOT(update_delay()), Qt::QueuedConnection);

    ui->BRIGHTNESS_SLIDER->setMinimum(-10);
    ui->BRIGHTNESS_SLIDER->setMaximum(10);
    ui->BRIGHTNESS_SLIDER->setSingleStep(1);
    ui->BRIGHTNESS_SLIDER->setPageStep(3);
    ui->BRIGHTNESS_SLIDER->setValue(0);
    ui->BRIGHTNESS_SLIDER->setTickInterval(5);

    ui->CONTRAST_SLIDER->setMinimum(0);
    ui->CONTRAST_SLIDER->setMaximum(20);
    ui->CONTRAST_SLIDER->setSingleStep(1);
    ui->CONTRAST_SLIDER->setPageStep(3);
    ui->CONTRAST_SLIDER->setValue(10);
    ui->CONTRAST_SLIDER->setTickInterval(5);

    on_ENUM_BUTTON_clicked();
    if (com[0] && com[3]) on_LASER_BTN_clicked();

    ui->COM_DATA_RADIO->click();

    // - set startup focus
    (ui->START_BUTTON->isEnabled() ? ui->START_BUTTON : ui->ENUM_BUTTON)->setFocus();

    ui->RULER_V->vertical = true;
//    ui->label->setup_animation(":/pics/model.png", QSize(700, 400), 500);
//    ui->label_2->setup_animation(":/pics/wave.png", QSize(512, 512), 100);
//    ui->label_3->setup_animation(":/pics/run.png", QSize(512, 526), 100);
    ui->DRAG_TOOL->click();
    ui->CURR_COORD->setup("current");
    connect(ui->SOURCE_DISPLAY, SIGNAL(curr_pos(QPoint)), ui->CURR_COORD, SLOT(display_pos(QPoint)));
    ui->START_COORD->setup("start");
    connect(ui->SOURCE_DISPLAY, SIGNAL(start_pos(QPoint)), ui->START_COORD, SLOT(display_pos(QPoint)));
    ui->SHAPE_INFO->setup("size");
    connect(ui->SOURCE_DISPLAY, SIGNAL(shape_size(QPoint)), ui->SHAPE_INFO, SLOT(display_pos(QPoint)));

    QTableWidget *roi_table = ui->ROI_TABLE;
    QStringList header_str;
    header_str << "operation" << "apply";
    QFont f = font();
    f.setBold(true);
    for (int i = 0; i < 2; i++) {
        QTableWidgetItem *h = new QTableWidgetItem(header_str[i]);
        h->setFont(f);
        roi_table->setHorizontalHeaderItem(i, h);
    }
    roi_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    roi_table->setColumnWidth(0, 134);
    roi_table->setColumnWidth(1, 39);
    roi_table->verticalHeader()->setMaximumWidth(16);
    roi_table->verticalHeader()->setMinimumWidth(16);
    for (int i = 0; i < 5; i++) {
        QTableWidgetItem *h = new QTableWidgetItem(QString::number(i + 1));
        h->setFont(f);
        roi_table->setVerticalHeaderItem(i, h);
    }

    // for presentation
    ui->CTRL_STATIC->hide();
    ui->ROI_TABLE->hide();
    ui->ROI_RADIO->hide();
#ifndef PARAM
    qDebug("%s\n", "presentation");
    ui->DISTANCE->hide();
    ui->TCU_STATIC->hide();
    ui->CTRL_STATIC->show();
    ui->LOGO1->show();
    ui->LOGO2->show();
    ui->APP_NAME->show();
    ui->HISTOGRAM_RADIO->click();
#endif
}

Demo::~Demo()
{
    delete ui;

    h_mouse_thread->quit();
    h_mouse_thread->wait();
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

        rep_freq = ui->FREQ_EDIT->text().toInt();
        laser_width_u = ui->LASER_WIDTH_EDIT_U->text().toInt();
        laser_width_n = ui->LASER_WIDTH_EDIT_N->text().toInt();
        gate_width_a_u = ui->GATE_WIDTH_A_EDIT_U->text().toInt();
        gate_width_a_n = ui->GATE_WIDTH_A_EDIT_N->text().toInt();
        delay_a_u = ui->DELAY_A_EDIT_U->text().toInt();
        delay_a_n = ui->DELAY_A_EDIT_N->text().toInt();
//        gate_width_b_u = ui->GATE_WIDTH_B_EDIT_U->text().toInt();
//        gate_width_b_n = ui->GATE_WIDTH_B_EDIT_N->text().toInt();
        stride = ui->STRIDE_EDIT->text().toInt();
        delay_b_u = ui->DELAY_B_EDIT_U->text().toInt();
        delay_b_n = ui->DELAY_B_EDIT_N->text().toInt();
        delay_n_n = ui->DELAY_N_EDIT_N->text().toInt();
        // use toFloat() since float representation in line edit
        fps = ui->CCD_FREQ_EDIT->text().toFloat();
        duty = ui->DUTY_EDIT->text().toFloat() * 1000;

        zoom = ui->ZOOM_EDIT->text().toInt();
        focus = ui->FOCUS_EDIT->text().toInt();

//        delay = delay_a_u * 1000 + delay_a_n;
        delay_dist = (delay_a_u * 1000 + delay_a_n) * dist_ns;
        depth_of_vision = (gate_width_a_u * 1000 + gate_width_a_n) * dist_ns;
    }
    else {
//        ui->DEVICE_SELECTION->setCurrentIndex(device_idx);
        ui->FRAME_AVG_OPTIONS->setCurrentIndex(calc_avg_option / 5 - 1);

        ui->SOFTWARE_CHECK->setChecked(trigger_by_software);
        ui->IMG_3D_CHECK->setChecked(image_3d);

        ui->GAIN_EDIT->setText(QString::asprintf("%d", (int)gain_analog_edit));
        ui->DUTY_EDIT->setText(QString::asprintf("%.2f", time_exposure_edit / 1000));
        ui->CCD_FREQ_EDIT->setText(QString::asprintf("%.2f", frame_rate_edit));
        ui->FILE_PATH_EDIT->setText(save_location);

        delay_a_u = (int)(delay_dist / dist_ns) / 1000;
        delay_a_n = (int)(delay_dist / dist_ns) % 1000;
        delay_b_u = (int)(delay_dist / dist_ns + delay_n_n) / 1000;
        delay_b_n = (int)(delay_dist / dist_ns + delay_n_n) % 1000;
//        laser_width_u = gate_width_a_u = gate_width_b_u = (int)(depth_of_vision / dist_ns) / 1000;
//        laser_width_n = gate_width_a_n = gate_width_b_n = (int)(depth_of_vision / dist_ns) % 1000;
//        gate_width_a_u = gate_width_b_u = (int)(depth_of_vision / dist_ns) / 1000;
//        gate_width_a_n = gate_width_b_n = (int)(depth_of_vision / dist_ns) % 1000;
#ifdef PARAM
        gate_width_a_u = (int)(depth_of_vision / dist_ns) / 1000;
        gate_width_a_n = (int)(depth_of_vision / dist_ns) % 1000;
#else
        laser_width_u = gate_width_a_u = (int)(depth_of_vision / dist_ns) / 1000;
        laser_width_n = gate_width_a_n = (int)(depth_of_vision / dist_ns) % 1000;
#endif
        ui->STRIDE_EDIT->setText(QString::asprintf("%d", stride));

        ui->FREQ_EDIT->setText(QString::asprintf("%d", rep_freq));
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

        ui->ZOOM_EDIT->setText(QString::asprintf("%d", zoom));
        ui->FOCUS_EDIT->setText(QString::asprintf("%d", focus));
    }
}

int Demo::grab_thread_process() {
    Display *disp = ui->SOURCE_DISPLAY;
    QImage stream;
    cv::Mat sobel;
    while (grab_thread_state) {
        if (img_q.empty()) {
            QThread::msleep(5);
            continue;
        }
        img_mem = img_q.front();
        img_q.pop();

        save_img_mux.lock();

        // tenengrad (sobel) auto-focus
        cv::Sobel(img_mem, sobel, CV_16U, 1, 1);
        clarity[2] = clarity[1], clarity[1] = clarity[0], clarity[0] = cv::mean(sobel)[0] * 1000;
        if (focus_direction) {
//            qDebug() << "qqqq" << focus_direction;
            if (clarity[2] > clarity[1] && clarity[1] > clarity[0]) {
                focus_direction *= -2;
                // TO-DO: change speed
                if (abs(focus_direction) > 7) lens_stop(), focus_direction = 0;
                else if (focus_direction > 0) lens_stop(), change_focus_speed(8), focus_far();
                else if (focus_direction < 0) lens_stop(), change_focus_speed(16), focus_near();
            }
        }

        if (seq_sum.empty()) seq_sum = cv::Mat::zeros(h, w, CV_16U);
        if (ui->FRAME_AVG_CHECK->isChecked()) {
            calc_avg_option = ui->FRAME_AVG_OPTIONS->currentIndex() * 5 + 5;
            if (seq[9].empty()) for (auto& m: seq) m = cv::Mat::zeros(h, w, CV_16U);

            img_mem.convertTo(seq[seq_idx], CV_16U);
            for(int i = 0; i < calc_avg_option; i++) seq_sum += seq[i];

            seq_idx = (seq_idx + 1) % calc_avg_option;

            seq_sum.convertTo(modified_result, CV_8U, 1.0 / calc_avg_option);
            seq_sum.release();
        }
        else modified_result = img_mem.clone();

        if (scan && delay_dist > 10000) {on_SCAN_BUTTON_clicked();}
        if (scan) {
            emit update_delay_in_thread();

            delay_dist += 100;
            filter_scan();
        }

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
                img_log *= 1.2;
                cv::normalize(img_log, img_log, 0, 255, cv::NORM_MINMAX);
                cv::convertScaleAbs(img_log, modified_result);
                break;
            }
            // gamma
            case 4: {
                cv::Mat img_gamma;
                modified_result.convertTo(img_gamma, CV_32F, 1.0 / 255, 0);
                cv::pow(img_gamma, 2.5, img_gamma);
                img_gamma.convertTo(modified_result, CV_8U, 255, 0);
                break;
            }
            case 5: {
                uchar *img = modified_result.data;
                uchar *accu_frame = accu[accu_idx].data;
                for (int i = 0; i < h; i++) {
                    for (int j = 0; j < w; j++) {
                        uchar p = img[i * modified_result.step + j];
                        if (p < 64)       {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.48);}
                        else if (p < 112) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.36);}
                        else if (p < 144) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.32);}
                        else if (p < 160) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.30);}
                        else if (p < 176) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.28);}
                        else if (p < 192) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.25);}
                        else if (p < 200) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.24);}
                        else if (p < 208) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.23);}
                        else if (p < 216) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.225);}
                        else if (p < 224) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.22);}
                        else if (p < 240) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.21);}
                        else if (p < 256) {accu_frame[i * modified_result.step + j] = cv::saturate_cast<uchar>(p * 0.2);}

                    }
                }
                modified_result = cv::Mat::zeros(h, w, CV_8U);
                for(auto m: accu) modified_result += m;
                accu_idx = (accu_idx + 1) % 5;
            }
            case 6: {
                uchar *img = modified_result.data;
                cv::Mat img_log, img_nonLT = cv::Mat(h, w, CV_8U);
                modified_result.convertTo(img_log, CV_32F);
                modified_result += 1.0;
                cv::log(img_log, img_log);
                img_log *= 8;
                double m = 0, kv = 0, mean = cv::mean(modified_result)[0];
                uchar p;
                for (int i = 0; i < h; i++) {
                    for (int j = 0; j < w; j++) {
                        p = img[i * modified_result.step + j];
                        if (!p) {
                            img_nonLT.data[i * img_nonLT.step + j] = 0;
                            continue;
                        }
                        if (p <= 60) kv = 7;
                        else if (p <= 200) kv = 7 + (p - 60) / 70;
                        else if (p <= 255) kv = 9 + (p - 200) / 55;
                        m = kv * (p / (p + mean));
                        img_nonLT.data[i * img_nonLT.step + j] = (int)(2 / (1 + exp(-m)) - 1) * 255 - p;
                    }
                }
                cv::normalize(img_log, img_log, 0, 255, cv::NORM_MINMAX);
                cv::convertScaleAbs(img_log, img_log);
                modified_result = 0.05 * img_log + 0.05 * img_nonLT + 0.8 * modified_result;
            }
            // none
            default:
                break;
            }
        }
        if (ui->SP_CHECK->isChecked()) ImageProc::plateau_equl_hist(&modified_result, &modified_result, ui->SP_OPTIONS->currentIndex());

        // brightness & contrast
        int val = ui->BRIGHTNESS_SLIDER->value() * 12.8;
        modified_result *= ui->CONTRAST_SLIDER->value() / 10.0;
        modified_result += val;
        // do not change pixel of value 0 when adjusting brightness
        for (int i = 0; i < h; i++) for (int j = 0; j < w; j++) {
            if (modified_result.at<uchar>(i, j) == val) modified_result.at<uchar>(i, j) = 0;
        }

        if (image_3d) {
            range_threshold = ui->RANGE_THRESH_EDIT->text().toFloat();
            modified_result = frame_a_3d ? prev_3d : ImageProc::gated3D(prev_img, img_mem, delay_dist / dist_ns, depth_of_vision / dist_ns, range_threshold);
            if (!frame_a_3d) prev_3d = modified_result.clone();
            frame_a_3d ^= 1;
        }
        prev_img = img_mem.clone();

        if (ui->INFO_CHECK->isChecked()) {
            cv::putText(modified_result, QString::asprintf("DIST %05d m", (int)delay_dist).toLatin1().data(), cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 3);
            cv::putText(modified_result, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(w - 240, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 3);
        }

        cv::Rect region = cv::Rect(disp->display_region.tl() * (image_3d ? w + 104 : w) / disp->width(), disp->display_region.br() * (image_3d ? w + 104 : w) / disp->width());
        if (region.height > h) region.height = h;
//        qDebug("x: %d y: %d, w: %d, h: %d\n", region.x, region.y, region.width, region.height);
        cropped_img = modified_result(region);
        // resize to display size
        cv::resize(cropped_img, cropped_img, cv::Size(ui->SOURCE_DISPLAY->width(), ui->SOURCE_DISPLAY->height()), 0, 0, cv::INTER_AREA);
        // draw the center cross
        if (ui->CENTER_CHECK->isChecked()) {
//            for (int i = cropped_img.cols / 2 - 9; i < cropped_img.cols / 2 + 10; i++) cropped_img.at<uchar>(cropped_img.rows / 2, i) = 255 - cropped_img.at<uchar>(cropped_img.rows / 2, i);
//            for (int i = cropped_img.rows / 2 - 9; i < cropped_img.rows / 2 + 10; i++) cropped_img.at<uchar>(i, cropped_img.cols / 2) = 255 - cropped_img.at<uchar>(i, cropped_img.cols / 2);
            for (int i = cropped_img.cols / 2 - 9; i < cropped_img.cols / 2 + 10; i++) cropped_img.at<uchar>(cropped_img.rows / 2, i) = cropped_img.at<uchar>(cropped_img.rows / 2, i) > 127 ? 0 : 255;
            for (int i = cropped_img.rows / 2 - 9; i < cropped_img.rows / 2 + 10; i++) cropped_img.at<uchar>(i, cropped_img.cols / 2) = cropped_img.at<uchar>(i, cropped_img.cols / 2) > 127 ? 0 : 255;
        }
        if (ui->SELECT_TOOL->isChecked() && disp->selection_v1 != disp->selection_v2) cv::rectangle(cropped_img, disp->selection_v1, disp->selection_v2, cv::Scalar(255));

//        stream = QImage(cropped_img.data, cropped_img.cols, cropped_img.rows, cropped_img.step, QImage::Format_RGB888);
        stream = QImage(cropped_img.data, cropped_img.cols, cropped_img.rows, cropped_img.step, image_3d ? QImage::Format_RGB888 : QImage::Format_Indexed8);
        ui->SOURCE_DISPLAY->setPixmap(QPixmap::fromImage(stream.scaled(ui->SOURCE_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        if (save_original) save_to_file(false);
        if (save_modified) save_to_file(true);
        if (record_original) {
            cv::putText(img_mem, QString::asprintf("DIST %05d m", (int)delay_dist).toLatin1().data(), cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 3);
            cv::putText(img_mem, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(w - 240, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 3);
            vid_out[0].write(img_mem);
        }
        if (record_modified) {
            cv::putText(modified_result, QString::asprintf("DIST %05d m", (int)delay_dist).toLatin1().data(), cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 3);
            cv::putText(modified_result, QDateTime::currentDateTime().toString("hh:mm:ss:zzz").toLatin1().data(), cv::Point(w - 240, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 3);
            vid_out[1].write(modified_result);
        }

        if (ui->HISTOGRAM_RADIO->isChecked()) {
            if (modified_result.channels() != 1) continue;
            uchar *img = modified_result.data;
            int step = modified_result.step;
            memset(hist, 0, 256 * sizeof(uint));
            for (int i = 0; i < h; i++) for (int j = 0; j < w; j++)  hist[(img + i * step)[j]]++;
            uint max = 0;
            for (int i = 1; i < 256; i++) {
                if (hist[i] > 50000) hist[i] = 0;
                if (hist[i] > max) max = hist[i];
            }
            cv::Mat hist_image = cv::Mat::zeros(200, 256, CV_8UC3);
            for (int i = 0; i < 256; i++) {
                cv::rectangle(hist_image, cv::Point(i, 200), cv::Point(i + 2, 200 - hist[i] * 200.0 / max), cv::Scalar(222, 196, 176));
            }
            ui->HIST_DISPLAY->setPixmap(QPixmap::fromImage(QImage(hist_image.data, hist_image.cols, hist_image.rows, hist_image.step, QImage::Format_RGB888).scaled(ui->HIST_DISPLAY->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }

        save_img_mux.unlock();
    }
    return 0;
}

inline bool Demo::is_maximized()
{
    return ui->TITLE->is_maximized;
}

void Demo::draw_cursor(QCursor c)
{
    setCursor(c);
}

void Demo::switch_language()
{
    en ? qApp->removeTranslator(&trans) : qApp->installTranslator(&trans);
    ui->retranslateUi(this);
    int idx = ui->ENHANCE_OPTIONS->currentIndex();
    ui->ENHANCE_OPTIONS->clear();
    ui->ENHANCE_OPTIONS->addItem(tr("None"));
    ui->ENHANCE_OPTIONS->addItem(tr("Histogram"));
    ui->ENHANCE_OPTIONS->addItem(tr("Laplace"));
    ui->ENHANCE_OPTIONS->addItem(tr("Log-based"));
    ui->ENHANCE_OPTIONS->addItem(tr("Gamma-based"));
    ui->ENHANCE_OPTIONS->setCurrentIndex(idx);
    en ^= 1;
}

void Demo::closeEvent(QCloseEvent *event)
{
    shut_down();
    event->accept();
    if (QFile::exists("HQVSDK.xml")) QFile::remove("HQVSDK.xml");
}

int Demo::shut_down() {
    if (record_original) on_SAVE_AVI_BUTTON_clicked();

    grab_thread_state = false;
    if (h_grab_thread) {
        h_grab_thread->quit();
        h_grab_thread->wait();
        delete h_grab_thread;
        h_grab_thread = NULL;
    }

    if (!img_q.empty()) std::queue<cv::Mat>().swap(img_q);

    if (curr_cam) curr_cam->shut_down();

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
    ui->SOFTWARE_CHECK->setEnabled(device_on && !start_grabbing && trigger_mode_on);
    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(start_grabbing && trigger_mode_on && trigger_by_software);
    ui->IMG_3D_CHECK->setEnabled(start_grabbing);
    ui->IMG_ENHANCE_CHECK->setEnabled(start_grabbing);
    ui->SP_CHECK->setEnabled(start_grabbing);
    ui->FRAME_AVG_CHECK->setEnabled(start_grabbing);
    ui->ENHANCE_OPTIONS->setEnabled(start_grabbing);
    ui->SP_OPTIONS->setEnabled(start_grabbing);
    ui->FRAME_AVG_OPTIONS->setEnabled(start_grabbing);
    ui->RANGE_THRESH_EDIT->setEnabled(start_grabbing);
//    ui->BRIGHTNESS_LABEL->setEnabled(start_grabbing);
    ui->BRIGHTNESS_SLIDER->setEnabled(start_grabbing);
//    ui->CONTRAST_LABEL->setEnabled(start_grabbing);
    ui->CONTRAST_SLIDER->setEnabled(start_grabbing);
    ui->SCAN_BUTTON->setEnabled(start_grabbing);
    ui->INFO_CHECK->setEnabled(start_grabbing);
    ui->CENTER_CHECK->setEnabled(start_grabbing);
}

void Demo::save_to_file(bool save_result) {
    cv::imwrite(QString(save_location + (save_result ? "\\res_bmp\\" : "\\raw_bmp\\") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp").toLatin1().data(), save_result ? modified_result : img_mem);
}

void Demo::setup_com(QSerialPort **com, int id, QString port_num, int baud_rate) {
    if (*com) delete *com;
    *com = new QSerialPort;
    (*com)->setPortName("COM" + port_num);
    qDebug("%p\n", *com);

    if ((*com)->open(QIODevice::ReadWrite)) {
        QThread::msleep(10);
        (*com)->clear();
        qDebug("COM%s connected\n", qPrintable(port_num));
        com_label[id]->setStyleSheet("color: #B0C4DE;");

        (*com)->setBaudRate(baud_rate);
        (*com)->setDataBits(QSerialPort::Data8);
        (*com)->setParity(QSerialPort::NoParity);
        (*com)->setStopBits(QSerialPort::OneStop);
        (*com)->setFlowControl(QSerialPort::NoFlowControl);

        // send initial data
        switch (id) {
        case 0:
            convert_to_send_tcu(0x01, (laser_width_u * 1000 + laser_width_n + 8) / 8);
            communicate_display(com[0], 1, 7, false);
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
    }
    else {
        com_label[id]->setStyleSheet("color: #CD5C5C;");
        delete *com;
        *com = NULL;
    }
}

void Demo::on_ENUM_BUTTON_clicked()
{
    if (curr_cam) delete curr_cam;
    curr_cam = new Cam;
    enable_controls(curr_cam->search_for_devices());
}

void Demo::on_START_BUTTON_clicked()
{
    if (device_on) return;
    data_exchange(true);

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
    data_exchange(false);

    // adjust display size according to frame size
    curr_cam->get_frame_size(w, h);
    qDebug("frame w: %d, h: %d\n", w, h);
//    QRect region = ui->SOURCE_DISPLAY->geometry();
//    region.setHeight(ui->SOURCE_DISPLAY->width() * h / w);
//    ui->SOURCE_DISPLAY->setGeometry(region);
//    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
//    ui->SOURCE_DISPLAY->update_roi(QPoint());
    resizeEvent(new QResizeEvent(this->size(), this->size()));

}

void Demo::on_SHUTDOWN_BUTTON_clicked()
{
    shut_down();
    enable_controls(true);
    clean();
}

void Demo::on_CONTINUOUS_RADIO_clicked()
{
    data_exchange(true);
    ui->CONTINUOUS_RADIO->setChecked(true);
    ui->TRIGGER_RADIO->setChecked(false);
    ui->SOFTWARE_CHECK->setEnabled(false);
    trigger_mode_on = false;
    curr_cam->trigger_mode(false, &trigger_mode_on);

    ui->SOFTWARE_TRIGGER_BUTTON->setEnabled(false);
}

void Demo::on_TRIGGER_RADIO_clicked()
{
    data_exchange(true);
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
}

void Demo::on_STOP_GRABBING_BUTTON_clicked()
{
    if (!device_on || !start_grabbing || curr_cam == NULL) return;

    if (record_original) on_SAVE_AVI_BUTTON_clicked();
    if (curr_cam->stop_grabbing()) return;

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
    ui->SOURCE_DISPLAY->clear();
    enable_controls(true);
}

void Demo::on_SAVE_BMP_BUTTON_clicked()
{
    save_original = !save_original;
    if (save_original && !QDir(save_location + "\\raw_bmp").exists()) QDir().mkdir(save_location + "\\raw_bmp");
    ui->SAVE_BMP_BUTTON->setText(save_original ? tr("Stop") : tr("ORI"));
}

void Demo::on_SAVE_FINAL_BUTTON_clicked()
{
//    cv::imwrite(QString(save_location + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".jpg").toLatin1().data(), modified_result);
    if (record_modified) {
//        curr_cam->stop_recording(0);
        save_img_mux.lock();
        vid_out[1].release();
        save_img_mux.unlock();
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "\\" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        save_img_mux.lock();
        vid_out[1].open(QString(save_location + "\\" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_res.avi").toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w, h), false);
        save_img_mux.unlock();
    }
    record_modified = !record_modified;
    ui->SAVE_FINAL_BUTTON->setText(record_modified ? tr("Stop") : tr("RES"));
}

void Demo::on_SET_PARAMS_BUTTON_clicked()
{
    data_exchange(true);

#ifndef PARAM
    duty = 1e6 / fps - 1000;
#endif

    uint send = 0;

    // CCD FREQUENCY
    send = 1.25e8 / fps;
    convert_to_send_tcu(0x06, send);
    communicate_display(com[0], 1, 7, false);

    // DUTY RATIO -> EXPO. TIME
    send = duty * 1.25e2;
    convert_to_send_tcu(0x07, send);
    communicate_display(com[0], 1, 7, false);

    time_exposure_edit = duty;
    frame_rate_edit = fps;
    if (device_on) {
        curr_cam->time_exposure(false, &time_exposure_edit);
        curr_cam->frame_rate(false, &frame_rate_edit);
        curr_cam->gain_analog(false, &gain_analog_edit);
    }
    data_exchange(false);
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

    data_exchange(false);
}

void Demo::on_FILE_PATH_BROWSE_clicked()
{
    QString temp = QFileDialog::getExistingDirectory(this, tr("Select folder"), save_location);
    if (!temp.isEmpty()) save_location = temp;
    data_exchange(false);
}

void Demo::clean()
{
    ui->SOURCE_DISPLAY->clear();
    ui->DATA_EXCHANGE->clear();
    if (grab_thread_state) return;
    std::queue<cv::Mat>().swap(img_q);
    img_mem.release();
    modified_result.release();
    cropped_img.release();
    prev_img.release();
    prev_3d.release();
    for (auto& m: seq) m.release();
    seq_sum.release();
}

// convert data to be sent to TCU-COM to hex buffer
void Demo::convert_to_send_tcu(uchar num, unsigned int send) {
    out_buffer[0] = 0x88;
    out_buffer[1] = num;
    out_buffer[6] = 0x99;

    out_buffer[5] = send & 0xFF; send >>= 8;
    out_buffer[4] = send & 0xFF; send >>= 8;
    out_buffer[3] = send & 0xFF; send >>= 8;
    out_buffer[2] = send & 0xFF;
}

// send and receive data from COM, and display
void Demo::communicate_display(QSerialPort *com, int receive_size, int send_size, bool fb) {
    QString str_s("sent    "), str_r("received");

    for (char i = 0; i < 7; i++) str_s += QString::asprintf(" %02X", i + send_size - 7 < 0 ? 0 : out_buffer[i + send_size - 7]);
    emit append_text(str_s);

    if (com == NULL) return;
    com->clear();
    com->write(QByteArray((char*)out_buffer, send_size));
    while (com->waitForBytesWritten(10)) ;

    if (fb) while (com->waitForReadyRead(100)) ;
    memset(in_buffer, 0, 7);
    memcpy(in_buffer, com->readAll(), receive_size);
    for (char i = 0; i < 7; i++) str_r += QString::asprintf(" %02X", i + receive_size - 7 < 0 ? 0 : in_buffer[i + receive_size - 7]);
    emit append_text(str_r);

    QThread().msleep(10);
}

void Demo::update_delay()
{
    unsigned int send = 0;

    // REPEATED FREQUENCY
    if (delay_dist < 0) delay_dist = 0;
    if (delay_dist > 15000) delay_dist = 15000;
//    qDebug("estimated distance: %f\n", delay_dist);

    ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist));
    ui->DELAY_SLIDER->setValue(delay_dist);

    if (scan) {
        rep_freq = 10;
    }
    else {
        // change fps according to delay: fps (kHz) <= 1s / delay (μs)
        rep_freq = delay_dist ? 1.0 * 1e6 / (delay_dist / dist_ns + depth_of_vision / dist_ns + 3) : 30;
        if (rep_freq > 30) rep_freq = 30;
        if (rep_freq < 10) rep_freq = 10;

        send = 1.25e5 / rep_freq;
        convert_to_send_tcu(0x00, send);
        communicate_display(com[0], 1, 7, false);
    }

    data_exchange(false);
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
    send = (delay_a_u * 1000 + delay_a_n) + 68;
    convert_to_send_tcu(0x02, send);
    communicate_display(com[0], 1, 7, false);

    // DELAY B
    send = (delay_b_u * 1000 + delay_b_n) + 68;
    convert_to_send_tcu(0x04, send);
    communicate_display(com[0], 1, 7, false);
}

void Demo::update_gate_width() {
    if (depth_of_vision < 0) depth_of_vision = 0;
    if (depth_of_vision > 1500) depth_of_vision = 1500;

    unsigned int send = 0;
    int gw = depth_of_vision / dist_ns;

    ui->GATE_WIDTH->setText(QString::asprintf("%.2f m", depth_of_vision));
//    gate_width_a_n = gate_width_b_n = laser_width_n = gw % 1000;
//    gate_width_a_u = gate_width_b_u = laser_width_u = gw / 1000;
#ifdef PARAM
    gate_width_a_n = gw % 1000;
    gate_width_a_u = gw / 1000;
#else
    gate_width_a_n = laser_width_n = gw % 1000;
    gate_width_a_u = laser_width_u = gw / 1000;

    // LASER WIDTH
    send = (gw - 18) / 8;
    convert_to_send_tcu(0x01, send);
    communicate_display(com[0], 1, 7, false);
#endif

    // GATE WIDTH A
//    send = gw - 18;
    send = gw + 8;
    convert_to_send_tcu(0x03, send);
    communicate_display(com[0], 1, 7, false);

    // GATE WIDTH B
//    send = gw - 18;
    send = gw + 8;
    convert_to_send_tcu(0x05, send);
    communicate_display(com[0], 1, 7, false);

    data_exchange(false);
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
    if (!com[3]) return;
    QString send = "DIOD1:CURR " + ui->CURRENT_EDIT->text() + "\r";
    com[3]->write(send.toLatin1().data(), send.length());
    com[3]->waitForBytesWritten(100);
    com[3]->readAll();
    qDebug() << send;
}

void Demo::on_DIST_BTN_clicked() {
    if (com[1]) {
        com[1]->write(QByteArray(1, 0xA5));
        while (com[1]->waitForReadyRead(100)) ;
        uchar read1[6] = {0};
        memcpy(read1, com[1]->readAll().data(), 6);
        QString read_dist1;
        for (int i = 0; i < 6; i++) read_dist1 += QString::asprintf(" %02X", read1[i]);
        qDebug("%s", read_dist1.toLatin1().data());

        out_buffer[0] = 0xEE;
        out_buffer[1] = 0x16;
        out_buffer[2] = 0x02;
        out_buffer[3] = 0x03;
        out_buffer[4] = 0x02;
        out_buffer[5] = 0x05;

        com[1]->write(QByteArray((char*)out_buffer, 6));
        while (com[1]->waitForReadyRead(1000)) ;
        uchar read[10] = {0};
        memcpy(read, com[1]->readAll().data(), 10);
        QString read_dist;
        for (int i = 0; i < 10; i++) read_dist += QString::asprintf(" %02X", read[i]);
        qDebug("%s", read_dist.toLatin1().data());
//        communicate_display(com[1], 7, 7, true);
//        QString ascii;
//        for (int i = 0; i < 7; i++) ascii += QString::asprintf(" %2c", in_buffer[i]);
//        emit append_text("received " + ascii);

        distance = read[7] + (read[6] << 8);
    }

    ui->DISTANCE->setText(QString::asprintf("%d m", distance));

    data_exchange(true);

    // change delay and gate width according to distance
    delay_dist = distance;
    depth_of_vision = 300;
    ui->MCP_SLIDER->setValue(150);

    update_delay();
    update_gate_width();
    change_mcp(150);
}

void Demo::on_IMG_3D_CHECK_stateChanged(int arg1)
{
    QRect region = ui->SOURCE_DISPLAY->geometry();
    region.setHeight(ui->SOURCE_DISPLAY->width() * h / (arg1 ? w + 104 : w));
    ui->SOURCE_DISPLAY->setGeometry(region);
//    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
    ui->SOURCE_DISPLAY->update_roi(QPoint());

    data_exchange(true);
}

/*
case IDC_OPEN_BUTTON:
    GetDlgItem(IDC_OPEN_BUTTON)->SetWindowText(_T("stop"));

    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x02;
    out_buffer[3] = 0x00;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x03;

    communicate_single(h_com_focus, 1, 7);
    break;
case IDC_CLOSE_BUTTON:
    GetDlgItem(IDC_CLOSE_BUTTON)->SetWindowText(_T("stop"));

    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x04;
    out_buffer[3] = 0x00;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x05;

    communicate_single(h_com_focus, 1, 7);
    break;
*/
void Demo::on_ZOOM_IN_BTN_pressed()
{
    ui->ZOOM_IN_BTN->setText("x");

    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x40;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x41;

//    out_buffer[0] = 0xFF;
//    out_buffer[1] = 0x01;
//    out_buffer[2] = 0x01;
//    out_buffer[3] = 0x00;
//    out_buffer[4] = 0x00;
//    out_buffer[5] = 0x00;
//    out_buffer[6] = 0x02;

    communicate_display(com[2], 1, 7, false);
}

void Demo::on_ZOOM_OUT_BTN_pressed()
{
    ui->ZOOM_OUT_BTN->setText("x");

    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x20;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x21;

//    out_buffer[0] = 0xFF;
//    out_buffer[1] = 0x01;
//    out_buffer[2] = 0x00;
//    out_buffer[3] = 0x80;
//    out_buffer[4] = 0x00;
//    out_buffer[5] = 0x00;
//    out_buffer[6] = 0x81;

    communicate_display(com[2], 1, 7, false);
}

void Demo::on_FOCUS_NEAR_BTN_pressed()
{
    ui->FOCUS_NEAR_BTN->setText("x");
    focus_near();
}

void Demo::focus_near()
{
    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x01;
    out_buffer[3] = 0x00;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x02;

//    out_buffer[0] = 0xFF;
//    out_buffer[1] = 0x01;
//    out_buffer[2] = 0x00;
//    out_buffer[3] = 0x40;
//    out_buffer[4] = 0x00;
//    out_buffer[5] = 0x00;
//    out_buffer[6] = 0x41;

    communicate_display(com[2], 1, 7, false);
}

void Demo::on_FOCUS_FAR_BTN_pressed()
{
    ui->FOCUS_FAR_BTN->setText("x");
    focus_far();
}

void Demo::focus_far()
{
    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x80;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x81;

//    out_buffer[0] = 0xFF;
//    out_buffer[1] = 0x01;
//    out_buffer[2] = 0x00;
//    out_buffer[3] = 0x20;
//    out_buffer[4] = 0x00;
//    out_buffer[5] = 0x00;
//    out_buffer[6] = 0x21;

    communicate_display(com[2], 1, 7, false);
}

void Demo::lens_stop() {
    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x00;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x01;

    communicate_display(com[2], 1, 7, false);
}

void Demo::set_zoom()
{
    data_exchange(true);
    unsigned short sum = 0;
    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x4F;
    out_buffer[4] = (zoom >> 8) & 0xFF;
    out_buffer[5] = zoom & 0xFF;
    for (int i = 1; i < 6; i++)
        sum += out_buffer[i];
    out_buffer[6] = sum & 0xFF;

    communicate_display(com[2], 1, 7, false);
}

void Demo::set_focus()
{
    data_exchange(true);
    unsigned short sum = 0;
    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x4E;
    out_buffer[4] = (focus >> 8) & 0xFF;
    out_buffer[5] = focus & 0xFF;
    for (int i = 1; i < 6; i++)
        sum += out_buffer[i];
    out_buffer[6] = sum & 0xFF;

    communicate_display(com[2], 1, 7, false);
}


void Demo::start_laser()
{
    out_buffer[0]=0x88;
    out_buffer[1]=0x08;
    out_buffer[2]=0x00;
    out_buffer[3]=0x00;
    out_buffer[4]=0x00;
    out_buffer[5]=0x01;
    out_buffer[6]=0x99;
    communicate_display(com[0], 0, 7, false);
    QTimer::singleShot(100000, this, SLOT(init_laser()));
}

void Demo::init_laser()
{
    ui->LASER_BTN->setEnabled(true);
    ui->LASER_BTN->setText(tr("OFF"));
    ui->CURRENT_EDIT->setEnabled(true);
    if (com[3]) com[3]->close();
    setup_com(com + 3, 3, com_edit[3]->text(), 9600);
    if (!com[3]) return;

    // enable laser com
    com[3]->write("MODE:RMT 1\r", 11);
    com[3]->waitForBytesWritten(100);
    QThread::msleep(100);
    com[3]->readAll();
    qDebug() << QString("MODE:RMT 1\r");

    // start
    com[3]->write("ON\r", 3);
    com[3]->waitForBytesWritten(100);
    QThread::msleep(100);
    com[3]->readAll();
    qDebug() << QString("ON\r");

    // enable external trigger
    com[3]->write("QSW:PRF 0\r", 10);
    com[3]->waitForBytesWritten(100);
    QThread::msleep(100);
    com[3]->readAll();
    qDebug() << QString("QSW:PRF 0\r");

    update_current();
}

void Demo::change_mcp(int val)
{
    mcp = val;

    convert_to_send_tcu(0x0A, mcp);
    communicate_display(com[0], 1, 7, false);

    ui->MCP_SLIDER->setStatusTip(QString::number(val));
}

void Demo::change_gain(int val)
{
    gain_analog_edit = val;
    curr_cam->gain_analog(false, &gain_analog_edit);
    data_exchange(false);
}

void Demo::change_delay(int val)
{
    if (abs(delay_dist - val) < 1) return;
    delay_dist = val;
    update_delay();
    data_exchange(false);
}

void Demo::change_focus_speed(int val)
{
    if (com[2]) com[2]->clear();
    ui->FOCUS_SPEED_EDIT->setText(QString::asprintf("%d", val));
    if (val > 1) val -= 1;

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

    if (com[2]) com[2]->write(QByteArray((char*)out_data, 9));
    while (com[2] && com[2]->waitForReadyRead(100)) ;
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
    uint send;

    if (event->key() == Qt::Key_Escape) {
        this->focusWidget()->clearFocus();
        data_exchange(false);
        return;
    }
    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (!(this->focusWidget())) break;
        edit = qobject_cast<QLineEdit*>(this->focusWidget());
        if (!edit) break;

        for (int i = 0; i < 4; i++) {
            if (edit == com_edit[i]) {
                if (com[i]) com[i]->close();
                setup_com(com + i, i, edit->text(), i == 1 ? 115200 : 9600);
            }
        }
        if (edit == ui->FREQ_EDIT) {
            fps = ui->FREQ_EDIT->text().toInt();
            send = 1.25e5 / fps;
            convert_to_send_tcu(0x00, send);
            communicate_display(com[0], 1, 7, false);
        }
        else if (edit == ui->GATE_WIDTH_A_EDIT_U) {
            depth_of_vision = (edit->text().toInt() * 1000 + ui->GATE_WIDTH_A_EDIT_N->text().toInt()) * dist_ns;
            update_gate_width();
        }
        else if (edit == ui->LASER_WIDTH_EDIT_U) {
            laser_width_u = edit->text().toInt();
//            send = (laser_width_u * 1000 + laser_width_n - 18) / 8;
            send = (laser_width_u * 1000 + laser_width_n + 8) / 8;
            convert_to_send_tcu(0x01, send);
            communicate_display(com[0], 1, 7, false);
        }
        else if (edit == ui->GATE_WIDTH_A_EDIT_N) {
            depth_of_vision = (edit->text().toInt() + ui->GATE_WIDTH_A_EDIT_U->text().toInt() * 1000) * dist_ns;
            update_gate_width();
        }
        else if (edit == ui->LASER_WIDTH_EDIT_N) {
            laser_width_n = edit->text().toInt();
//            send = (laser_width_u * 1000 + laser_width_n - 18) / 8;
            send = (laser_width_u * 1000 + laser_width_n + 8) / 8;
            convert_to_send_tcu(0x01, send);
            communicate_display(com[0], 1, 7, false);
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
            delay_dist = (edit->text().toInt() + ui->DELAY_A_EDIT_U->text().toInt() * 1000) * dist_ns;
            update_delay();
        }
        else if (edit == ui->DELAY_B_EDIT_N) {
            delay_dist = (edit->text().toInt() + ui->DELAY_B_EDIT_U->text().toInt() * 1000 - delay_n_n) * dist_ns;
            update_delay();
        }
        else if (edit == ui->DELAY_N_EDIT_N) {
            delay_n_n = edit->text().toInt();
            update_delay();
        }
        else if (edit == ui->STRIDE_EDIT) {
            stride = edit->text().toInt();
        }
        else if (edit == ui->ZOOM_EDIT) {
            set_zoom();
        }
        else if (edit == ui->FOCUS_EDIT) {
            set_focus();
        }
        else if (edit == ui->CCD_FREQ_EDIT || edit == ui->DUTY_EDIT) {
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
        delay_dist += stride * 5;
        update_delay();
        break;
    case Qt::Key_S:
        delay_dist -= stride * 5;
        update_delay();
        break;
    case Qt::Key_D:
        delay_dist += stride;
        update_delay();
        break;
    case Qt::Key_A:
        delay_dist -= stride;
        update_delay();
        break;
    // 50m => 333ns, 5m => 33ns
    case Qt::Key_I:
        depth_of_vision += stride * 5;
        update_gate_width();
        break;
    case Qt::Key_K:
        depth_of_vision -= stride * 5;
        update_gate_width();
        break;
    case Qt::Key_L:
        depth_of_vision += stride;
        update_gate_width();
        break;
    case Qt::Key_J:
        depth_of_vision -= stride;
        update_gate_width();
        break;
    default: break;
    }
}

void Demo::resizeEvent(QResizeEvent *event)
{
    QRect window = this->geometry();
//    qDebug() << window;
    int grp_width = ui->RIGHT->width();

    ui->RIGHT->move(window.width() - 12 - grp_width, 40);
//    ui->TCU_STATIC->move(pos_x, 20);
//    ui->LENS_STATIC->move(pos_x, 260);
//    ui->IMG_SAVE_STATIC->move(pos_x, 360);
//    ui->DATA_EXCHANGE->move(pos_x, 460);
//    ui->NAME->move(pos_x + 70, 50);
//    ui->CTRL_STATIC->move(pos_x, 140);
//    ui->LOGO1->move(pos_x, window.height() - 21 - 74);
//    ui->LOGO2->move(pos_x + 70, window.height() - 21 - 74 - 10 - 90);

    QRect region = ui->SOURCE_DISPLAY->geometry();
    QPoint center = ui->SOURCE_DISPLAY->center;

    int mid_width = window.width() - 12 - grp_width - 10 - ui->LEFT->width();
    ui->MID->setGeometry(10 + ui->LEFT->width(), 40, mid_width, window.height() - 50);
    int width = mid_width - 20, height = width * h / w;
//    qDebug("width %d, height %d\n", width, height);
    int height_constraint = window.height() - 30 - 50 - 20,
        width_constraint = height_constraint * w / h;
//    qDebug("width_constraint %d, height_constraint %d\n", width_constraint, height_constraint);
    if (width < width_constraint) width_constraint = width, height_constraint = width_constraint * h / w;
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
#ifndef PARAM
    ui->APP_NAME->move(region.right() - 160, 0);
#endif
    ui->HIDE_BTN->setGeometry(hide_left ? 2 : 212, this->geometry().height() / 2 - 10, 16, 16);
    ui->RULER_H->setGeometry(region.left(), region.bottom() - 10, region.width(), 32);
    ui->RULER_V->setGeometry(region.right() - 10, region.top(), 32, region.height());

    save_img_mux.lock();
    ui->SOURCE_DISPLAY->setGeometry(region);
    ui->SOURCE_DISPLAY->update_roi(center);
    qDebug("display region x: %d, y: %d, w: %d, h: %d\n", region.x(), region.y(), region.width(), region.height());
    save_img_mux.unlock();

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
    if (!mouse_pressed || ui->TITLE->is_maximized) return;

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

void Demo::on_SAVE_AVI_BUTTON_clicked()
{
    if (record_original) {
//        curr_cam->stop_recording(0);
        save_img_mux.lock();
        vid_out[0].release();
        save_img_mux.unlock();
    }
    else {
//        curr_cam->start_recording(0, QString(save_location + "\\" + QDateTime::currentDateTime().toString("MMddhhmmsszzz") + ".avi").toLatin1().data(), w, h, result_fps);
        save_img_mux.lock();
        vid_out[0].open(QString(save_location + "\\" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + "_raw.avi").toLatin1().data(), cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), frame_rate_edit, cv::Size(w, h), false);
        save_img_mux.unlock();
    }
    record_original = !record_original;
    ui->SAVE_AVI_BUTTON->setText(record_original ? tr("Stop") : tr("ORI"));
}

void Demo::on_GAIN_EDIT_textEdited(const QString &arg1)
{
    gain_analog_edit = (int)arg1.toFloat();
    if (gain_analog_edit < 0) gain_analog_edit = 0;
    if (gain_analog_edit > 480) gain_analog_edit = 480;
    ui->GAIN_SLIDER->setValue(gain_analog_edit);
    data_exchange(false);
}

void Demo::on_SCAN_BUTTON_clicked()
{
    bool start_scan = ui->SCAN_BUTTON->text() == tr("Scan");

    if (start_scan) {
        delay_dist = 200;

        on_CONTINUE_SCAN_BUTTON_clicked();
    }
    else {
        emit update_scan(false);
        scan = false;

        for (uint i = scan_q.size(); i; i--) qDebug("dist %f", scan_q.front()), scan_q.pop_front();
    }

    ui->SCAN_BUTTON->setText(start_scan ? tr("Stop") : tr("Scan"));
}

void Demo::on_CONTINUE_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);

    convert_to_send_tcu(0x00, (uint)(1.25e5 / rep_freq));
    communicate_display(com[0], 1, 7, false);
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
    if (delay_dist > scan_q.back()) scan_q.push_back(delay_dist);
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
    data_exchange(true);
}

void Demo::on_FOCUS_SPEED_EDIT_textEdited(const QString &arg1)
{
    uint speed = arg1.toUInt();
    if (speed < 1)  ui->FOCUS_SPEED_EDIT->setText(QString::asprintf("%u", 1));
    if (speed > 64) ui->FOCUS_SPEED_EDIT->setText(QString::asprintf("%u", 64));
}

void Demo::on_SAVE_RESULT_BUTTON_clicked()
{
    save_modified = !save_modified;
    if (save_modified && !QDir(save_location + "\\res_bmp").exists()) QDir().mkdir(save_location + "\\res_bmp");
    ui->SAVE_RESULT_BUTTON->setText(save_modified ? tr("Stop") : tr("RES"));
}

void Demo::on_LASER_ZOOM_IN_BTN_pressed()
{
    ui->LASER_ZOOM_IN_BTN->setText("x");

    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x02;
    out_buffer[3] = 0x00;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x03;

    communicate_display(com[2], 1, 7, false);
}

void Demo::on_LASER_ZOOM_OUT_BTN_pressed()
{
    ui->LASER_ZOOM_OUT_BTN->setText("x");

    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x04;
    out_buffer[3] = 0x00;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x05;

    communicate_display(com[2], 1, 7, false);
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

void Demo::on_LASER_BTN_clicked()
{
    if (ui->LASER_BTN->text() == "ON") {
        ui->LASER_BTN->setEnabled(false);
        out_buffer[0]=0x88;
        out_buffer[1]=0x08;
        out_buffer[2]=0x00;
        out_buffer[3]=0x00;
        out_buffer[4]=0x00;
        out_buffer[5]=0x01;
        out_buffer[6]=0x99;
        communicate_display(com[0], 0, 7, false);
        QTimer::singleShot(4000, this, SLOT(start_laser()));
    }
    else {
        out_buffer[0]=0x88;
        out_buffer[1]=0x08;
        out_buffer[2]=0x00;
        out_buffer[3]=0x00;
        out_buffer[4]=0x00;
        out_buffer[5]=0x02;
        out_buffer[6]=0x99;
        communicate_display(com[0], 0, 7, false);

        ui->LASER_BTN->setText(tr("ON"));
        ui->CURRENT_EDIT->setEnabled(false);
    }
}

void Demo::on_GET_LENS_PARAM_BTN_clicked()
{
    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x55;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x56;

    communicate_display(com[2], 7, 7, true);
    zoom = (in_buffer[4] << 8) + in_buffer[5];

    out_buffer[0] = 0xFF;
    out_buffer[1] = 0x01;
    out_buffer[2] = 0x00;
    out_buffer[3] = 0x56;
    out_buffer[4] = 0x00;
    out_buffer[5] = 0x00;
    out_buffer[6] = 0x57;

    communicate_display(com[2], 7, 7, true);
    focus = (in_buffer[4] << 8) + in_buffer[5];

    data_exchange(false);
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
    ui->HIDE_BTN->setText(hide_left ? ">>" : "<<");
    resizeEvent(new QResizeEvent(this->size(), this->size()));
}

void Demo::on_COM_DATA_RADIO_clicked()
{
    display_option = 1;
    ui->HISTOGRAM_RADIO->setChecked(false);
    ui->DATA_EXCHANGE->show();
    ui->HIST_DISPLAY->hide();
}

void Demo::on_HISTOGRAM_RADIO_clicked()
{
    display_option = 2;
    ui->COM_DATA_RADIO->setChecked(false);
    ui->DATA_EXCHANGE->hide();
    ui->HIST_DISPLAY->show();
}

void Demo::screenshot()
{
    save_img_mux.lock();
    window()->grab().save(save_location + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    save_img_mux.unlock();
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

void Demo::on_ENHANCE_OPTIONS_currentIndexChanged(int index)
{
    if (index == 5) {
        accu_sum.release();
        for (auto& m: accu) m.release();
        accu_sum = cv::Mat::zeros(h, w, CV_8U);
        for (auto& m: accu) m = cv::Mat::zeros(h, w, CV_8U);
        accu_idx = 0;
    }
}
