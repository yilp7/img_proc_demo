#include "ui_scanconfig.h"
#include "scanconfig.h"

ScanConfig::ScanConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScanConfig),
    pressed(false),
    dist_ns(3e8 / 2e9),
    hz_unit(0),
    base_unit(0),
    starting_delay(0),
    ending_delay(0),
    starting_gw(0),
    ending_gw(0),
    step_size_delay(1),
    step_size_gw(1),
    frame_count(1),
    rep_freq(10),
    capture_scan_ori(true),
    capture_scan_res(false),
    record_scan_ori(false),
    record_scan_res(true),
    starting_h(0),
    ending_h(0),
    starting_v(0),
    ending_v(0),
    step_h(0),
    step_v(0),
    count_h(1),
    count_v(1),
    ptz_direction(0),
    ptz_wait_time(4),
    num_single_pos(8)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    connect(ui->START_DELAY_EDIT_U,   SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->START_DELAY_EDIT_N,   SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->START_DELAY_EDIT_M,   SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->END_DELAY_EDIT_U,     SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->END_DELAY_EDIT_N,     SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->END_DELAY_EDIT_M,     SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->START_GW_EDIT_U,      SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->START_GW_EDIT_N,      SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->START_GW_EDIT_M,      SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->END_GW_EDIT_U,        SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->END_GW_EDIT_N,        SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->END_GW_EDIT_M,        SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->FRAME_COUNT_EDIT,     SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->STEP_SIZE_DELAY_EDIT, SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));
    connect(ui->STEP_SIZE_GW_EDIT,    SIGNAL(editingFinished()), this, SLOT(update_scan_tcu()));

    connect(ui->CAPTURE_SCAN_ORI_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ capture_scan_ori = arg1; });
    connect(ui->CAPTURE_SCAN_RES_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ capture_scan_res = arg1; });
    connect(ui->RECORD_SCAN_ORI_CHK,  &QCheckBox::stateChanged, this, [this](int arg1){ record_scan_ori = arg1; });
    connect(ui->RECORD_SCAN_RES_CHK,  &QCheckBox::stateChanged, this, [this](int arg1){ record_scan_res = arg1; });

    connect(ui->START_ANGLE_H_EDT, SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));
    connect(ui->START_ANGLE_V_EDT, SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));
    connect(ui->END_ANGLE_H_EDT,   SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));
    connect(ui->END_ANGLE_V_EDT,   SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));
    connect(ui->STEP_ANGLE_H_EDT,  SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));
    connect(ui->STEP_ANGLE_V_EDT,  SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));
    connect(ui->FRAME_H_EDT,       SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));
    connect(ui->FRAME_V_EDT,       SIGNAL(editingFinished()), this, SLOT(update_scan_ptz()));

    connect(ui->RADIO_PTZ_DIRECTION_N, &QRadioButton::clicked, this, [this]() { ui->RADIO_PTZ_DIRECTION_Z->setChecked(false); ptz_direction = 0; });
    connect(ui->RADIO_PTZ_DIRECTION_Z, &QRadioButton::clicked, this, [this]() { ui->RADIO_PTZ_DIRECTION_N->setChecked(false); ptz_direction = 1; });
    ui->RADIO_PTZ_DIRECTION_N->setChecked(true);

    connect(ui->PTZ_WAIT_TIME_EDT, &QLineEdit::editingFinished, this,
            [this]() {
                ptz_wait_time = ui->PTZ_WAIT_TIME_EDT->text().toInt();
                if (ptz_wait_time < 0) ptz_wait_time = 0;
                if (ptz_wait_time > 20) ptz_wait_time = 20;
                ui->PTZ_WAIT_TIME_EDT->setText(QString::number(ptz_wait_time));
            });
    connect(ui->NUM_SINGLE_POS_EDT, &QLineEdit::editingFinished, this,
            [this]() {
                num_single_pos = ui->NUM_SINGLE_POS_EDT->text().toInt();
                if (num_single_pos < 1) num_single_pos = 1;
                if (num_single_pos > 20) num_single_pos = 20;
                ui->NUM_SINGLE_POS_EDT->setText(QString::number(num_single_pos));
            });

    data_exchange(false);
    setup_hz(0);
    setup_unit(0);
}

ScanConfig::~ScanConfig()
{
    delete ui;
}

void ScanConfig::data_exchange(bool read)
{
    if (read) {
        switch (base_unit) {
        // ns
        case 0:
        // μs
        case 1:
            starting_delay = ui->START_DELAY_EDIT_U->text().toInt() * 1000 + ui->START_DELAY_EDIT_N->text().toInt();
            ending_delay = ui->END_DELAY_EDIT_U->text().toInt() * 1000 + ui->END_DELAY_EDIT_N->text().toInt();
            starting_gw = ui->START_GW_EDIT_U->text().toInt() * 1000 + ui->START_GW_EDIT_N->text().toInt();
            ending_gw = ui->END_GW_EDIT_U->text().toInt() * 1000 + ui->END_GW_EDIT_N->text().toInt();

            break;
        // m
        case 2:
            starting_delay = ui->START_GW_EDIT_M->text().toInt() / dist_ns;
            ending_delay = ui->END_GW_EDIT_M->text().toInt() / dist_ns;
            starting_gw = ui->START_GW_EDIT_M->text().toInt() / dist_ns;
            ending_gw = ui->END_GW_EDIT_M->text().toInt() / dist_ns;

            break;
        default: break;
        }
        frame_count = ui->FRAME_COUNT_EDIT->text().toInt();
        step_size_delay = ui->STEP_SIZE_DELAY_EDIT->text().toFloat();
        step_size_gw = ui->STEP_SIZE_GW_EDIT->text().toFloat();
        rep_freq = ui->REP_FREQ_EDIT->text().toFloat();

        capture_scan_ori = ui->CAPTURE_SCAN_ORI_CHK->isChecked();
        capture_scan_res = ui->CAPTURE_SCAN_RES_CHK->isChecked();
        record_scan_ori = ui->RECORD_SCAN_ORI_CHK->isChecked();
        record_scan_res = ui->RECORD_SCAN_RES_CHK->isChecked();

        starting_h = ui->START_ANGLE_H_EDT->text().toFloat();
        starting_v = ui->START_ANGLE_V_EDT->text().toFloat();
        ending_h = ui->END_ANGLE_H_EDT->text().toFloat();
        ending_v = ui->END_ANGLE_V_EDT->text().toFloat();
        step_h = ui->STEP_ANGLE_H_EDT->text().toFloat();
        step_v = ui->STEP_ANGLE_V_EDT->text().toFloat();
        count_h = ui->FRAME_H_EDT->text().toInt();
        count_v = ui->FRAME_V_EDT->text().toInt();
        ptz_wait_time = ui->PTZ_WAIT_TIME_EDT->text().toInt();
        num_single_pos = ui->NUM_SINGLE_POS_EDT->text().toInt();
    }
    else {
        switch (base_unit) {
        // ns
        case 0:
        // μs
        case 1:
            ui->START_DELAY_EDIT_U->setText(QString::number(starting_delay / 1000));
            ui->START_DELAY_EDIT_N->setText(QString::number(starting_delay % 1000));
            ui->END_DELAY_EDIT_U->setText(QString::number(ending_delay / 1000));
            ui->END_DELAY_EDIT_N->setText(QString::number(ending_delay % 1000));

            ui->START_GW_EDIT_U->setText(QString::number(starting_gw / 1000));
            ui->START_GW_EDIT_N->setText(QString::number(starting_gw % 1000));
            ui->END_GW_EDIT_U->setText(QString::number(ending_gw / 1000));
            ui->END_GW_EDIT_N->setText(QString::number(ending_gw % 1000));

            break;
        // m
        case 2:
            ui->START_GW_EDIT_M->setText(QString::number((int)(starting_delay * dist_ns)));
            ui->END_GW_EDIT_M->setText(QString::number((int)(ending_delay * dist_ns)));
            ui->START_GW_EDIT_M->setText(QString::number((int)(starting_gw * dist_ns)));
            ui->END_GW_EDIT_M->setText(QString::number((int)(ending_gw * dist_ns)));

            break;
        default: break;
        }
        ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
        ui->STEP_SIZE_DELAY_EDIT->setText(QString::number(step_size_delay, 'f', 2));
        ui->STEP_SIZE_GW_EDIT->setText(QString::number(step_size_gw, 'f', 2));
        ui->REP_FREQ_EDIT->setText(QString::number(rep_freq));

        ui->CAPTURE_SCAN_ORI_CHK->setChecked(capture_scan_ori);
        ui->CAPTURE_SCAN_RES_CHK->setChecked(capture_scan_res);
        ui->RECORD_SCAN_ORI_CHK->setChecked(record_scan_ori);
        ui->RECORD_SCAN_RES_CHK->setChecked(record_scan_res);

        ui->START_ANGLE_H_EDT->setText(QString::number(starting_h, 'f', 2));
        ui->START_ANGLE_V_EDT->setText(QString::number(starting_v, 'f', 2));
        ui->END_ANGLE_H_EDT->setText(QString::number(ending_h, 'f', 2));
        ui->END_ANGLE_V_EDT->setText(QString::number(ending_v, 'f', 2));
        ui->STEP_ANGLE_H_EDT->setText(QString::number(step_h, 'f', 2));
        ui->STEP_ANGLE_V_EDT->setText(QString::number(step_v, 'f', 2));
        ui->FRAME_H_EDT->setText(QString::number(count_h));
        ui->FRAME_V_EDT->setText(QString::number(count_v));
        ui->PTZ_WAIT_TIME_EDT->setText(QString::number(ptz_wait_time));
        ui->NUM_SINGLE_POS_EDT->setText(QString::number(num_single_pos));
    }
}

std::vector<std::pair<float, float>> ScanConfig::get_ptz_route()
{
    std::vector<std::pair<float, float>> scan_route;

    if (ptz_direction) {
        for (int j = 0; j < count_v; j++) {
            for (int i = 0; i < count_h; i++) {
                scan_route.push_back(std::make_pair((j % 2 ? ending_h - i * step_h : starting_h + i * step_h), starting_v + j * step_v));
            }
        }
    }
    else {
        for (int i = 0; i < count_h; i++) {
            for (int j = 0; j < count_v; j++) {
                scan_route.push_back(std::make_pair(starting_h + i * step_h, (i % 2 ? ending_v - j * step_v : starting_v + j * step_v)));
            }
        }
    }
//    qDebug() << scan_route;
    return scan_route;
}

std::vector<float> ScanConfig::get_tcu_route()
{
    std::vector<float> delay_value;
    for (int k = 0; k < frame_count; k++) {
        delay_value.push_back(starting_delay + k * step_size_delay);
    }
    return delay_value;
}

void ScanConfig::update_scan_tcu()
{
    switch (base_unit) {
    // ns
    case 0:
    // μs
    case 1:
        if (ui->START_DELAY_EDIT_N->text().toInt() > 999) starting_delay = ui->START_DELAY_EDIT_N->text().toInt();
        else starting_delay = ui->START_DELAY_EDIT_U->text().toInt() * 1000 + ui->START_DELAY_EDIT_N->text().toInt();
        if (ui->END_DELAY_EDIT_N->text().toInt() > 999) ending_delay = ui->END_DELAY_EDIT_N->text().toInt();
        else ending_delay = ui->END_DELAY_EDIT_U->text().toInt() * 1000 + ui->END_DELAY_EDIT_N->text().toInt();

        if (ui->START_GW_EDIT_N->text().toInt() > 999) starting_gw = ui->START_GW_EDIT_N->text().toInt();
        else starting_gw = ui->START_GW_EDIT_U->text().toInt() * 1000 + ui->START_GW_EDIT_N->text().toInt();
        if (ui->END_GW_EDIT_N->text().toInt() > 999) ending_gw = ui->END_GW_EDIT_N->text().toInt();
        else ending_gw = ui->END_GW_EDIT_U->text().toInt() * 1000 + ui->END_GW_EDIT_N->text().toInt();

        break;
    // m
    case 2:
        starting_delay = ui->START_GW_EDIT_M->text().toInt();
        ending_delay = ui->END_GW_EDIT_M->text().toInt();

        starting_gw = ui->START_GW_EDIT_M->text().toInt();
        ending_gw = ui->END_GW_EDIT_M->text().toInt();

        break;
    default: break;
    }

    QLineEdit *source = qobject_cast<QLineEdit*>(sender());
    if (source == ui->STEP_SIZE_DELAY_EDIT) {
        step_size_delay = ui->STEP_SIZE_DELAY_EDIT->text().toFloat();
        if (fabs(step_size_delay) < 1e-4) {
            step_size_delay = 0;
            frame_count = 1;
        }
        else {
            step_size_gw = 1.0 * (ending_gw - starting_gw) / (frame_count - 1);
            frame_count = 1.0 * (ending_delay - starting_delay) / step_size_delay + 1;
        }
//        if (!frame_count) frame_count = 1;
//        if (fabs(step_size_gw) < 1e-4) step_size_gw = 1;
    }
    else if (source == ui->STEP_SIZE_GW_EDIT) {
        step_size_gw = ui->STEP_SIZE_GW_EDIT->text().toFloat();
        if (fabs(step_size_gw) < 1e-4) {
            frame_count = 1;
            step_size_gw = 0;
        }
        else {
            frame_count = 1.0 * (ending_delay - starting_delay) / step_size_delay + 1;
            step_size_delay = 1.0 * (ending_delay - starting_delay) / (frame_count - 1);
        }
//        if (frame_count < 1) frame_count = 1;
//        if (fabs(step_size_delay) < 1e-4) step_size_delay = 1;
    }
    else {
        frame_count = ui->FRAME_COUNT_EDIT->text().toInt();
        if (frame_count <= 1) {
            frame_count = 1;
            step_size_delay = 0;
            step_size_gw = 0;
        }
        else {
            step_size_delay = 1.0 * (ending_delay - starting_delay) / (frame_count - 1);
            step_size_gw = 1.0 * (ending_gw - starting_gw) / (frame_count - 1);
            if (fabs(step_size_delay) < 1e-4) step_size_delay = 1;
            if (fabs(step_size_gw) < 1e-4) step_size_gw = 1;
        }
    }

    ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
    switch (base_unit) {
    // ns
    case 0:
    // μs
    case 1:
        ui->STEP_SIZE_DELAY_EDIT->setText(QString::number(step_size_delay, 'f', 2));
        ui->STEP_SIZE_GW_EDIT->setText(QString::number(step_size_gw, 'f', 2));
        break;
    // m
    case 2:
        ui->STEP_SIZE_DELAY_EDIT->setText(QString::number(step_size_delay * dist_ns, 'f', 2));
        ui->STEP_SIZE_GW_EDIT->setText(QString::number(step_size_gw * dist_ns, 'f', 2));
        break;
    default: break;
    }
}

void ScanConfig::update_scan_ptz()
{
    starting_h = ui->START_ANGLE_H_EDT->text().toFloat();
    starting_v = ui->START_ANGLE_V_EDT->text().toFloat();
    ending_h = ui->END_ANGLE_H_EDT->text().toFloat();
    ending_v = ui->END_ANGLE_V_EDT->text().toFloat();

    QLineEdit *source = qobject_cast<QLineEdit*>(sender());
    if (source == ui->STEP_ANGLE_H_EDT) {
        step_h = ui->STEP_ANGLE_H_EDT->text().toFloat();
        if (step_h == 0) {
            count_h = 1;
            step_h = 0;
        }
        else {
            count_h = std::ceil(1.0 * (ending_h - starting_h) / step_h) + 1;
            step_h = 1.0 * (ending_h - starting_h) / (count_h - 1);
        }
        ui->STEP_ANGLE_H_EDT->setText(QString::number(step_h, 'f', 2));
        ui->FRAME_H_EDT->setText(QString::number(count_h));
    }
    else if (source == ui->STEP_ANGLE_V_EDT) {
        step_v = ui->STEP_ANGLE_V_EDT->text().toFloat();
        if (step_v == 0) {
            count_v = 1;
            step_v = 0;
        }
        else {
            count_v = std::ceil(1.0 * (ending_v - starting_v) / step_v) + 1;
            step_v = 1.0 * (ending_v - starting_v) / (count_v - 1);
        }
        ui->STEP_ANGLE_V_EDT->setText(QString::number(step_v, 'f', 2));
        ui->FRAME_V_EDT->setText(QString::number(count_v));
    }
    else if (source == ui->FRAME_H_EDT || source == ui->START_ANGLE_H_EDT || source == ui->END_ANGLE_H_EDT) {
        count_h = ui->FRAME_H_EDT->text().toInt();
        if (count_h <= 1) {
            count_h = 1;
            step_h = 0;
        }
        else {
            step_h = 1.0 * (ending_h - starting_h) / (count_h - 1);
        }
        ui->STEP_ANGLE_H_EDT->setText(QString::number(step_h, 'f', 2));
    }
    else if (source == ui->FRAME_V_EDT || source == ui->START_ANGLE_V_EDT || source == ui->END_ANGLE_V_EDT) {
        count_v = ui->FRAME_V_EDT->text().toInt();
        if (count_v <= 1) {
            count_v = 1;
            step_v = 0;
        }
        else {
            step_v = 1.0 * (ending_v - starting_v) / (count_v - 1);
        }
        ui->STEP_ANGLE_V_EDT->setText(QString::number(step_v, 'f', 2));
    }
}

void ScanConfig::setup_hz(int idx)
{
    hz_unit = idx;
    switch (hz_unit) {
    // kHz
    case 0: ui->REP_FREQ_EDIT->setText(QString::number((int)rep_freq)); ui->FREQ_UNIT->setText("kHz"); break;
    // Hz
    case 1: ui->REP_FREQ_EDIT->setText(QString::number((int)(rep_freq * 1000))); ui->FREQ_UNIT->setText("Hz"); break;
    default: break;
    }
}

void ScanConfig::setup_unit(int idx)
{
    base_unit = idx;
    switch (base_unit) {
    // ns
    case 0:
    // μs
    case 1:
        ui->START_DELAY_EDIT_M->hide(); ui->END_DELAY_EDIT_M->hide();
        ui->START_GW_EDIT_M->hide(); ui->END_GW_EDIT_M->hide();
        ui->UNIT_M_1->hide(); ui->UNIT_M_2->hide(); ui->UNIT_M_3->hide(); ui->UNIT_M_4->hide();
        ui->START_DELAY_EDIT_U->show(); ui->START_DELAY_EDIT_N->show();
        ui->END_DELAY_EDIT_U->show(); ui->END_DELAY_EDIT_N->show();
        ui->START_GW_EDIT_U->show(); ui->START_GW_EDIT_N->show();
        ui->END_GW_EDIT_U->show(); ui->END_GW_EDIT_N->show();
        ui->UNIT_US_1->show(); ui->UNIT_US_2->show(); ui->UNIT_US_3->show(); ui->UNIT_US_4->show();
        ui->UNIT_NS_1->show(); ui->UNIT_NS_2->show(); ui->UNIT_NS_3->show(); ui->UNIT_NS_4->show();
        ui->STEP_SIZE_DELAY_EDIT->setText(QString::number(step_size_delay, 'f', 2));
        ui->STEP_SIZE_GW_EDIT->setText(QString::number(step_size_gw, 'f', 2));
        ui->UNIT_1->setText("ns"); ui->UNIT_2->setText("ns");
        break;
    // m
    case 2:
        ui->START_DELAY_EDIT_M->show(); ui->END_DELAY_EDIT_M->show();
        ui->START_GW_EDIT_M->show(); ui->END_GW_EDIT_M->show();
        ui->UNIT_M_1->show(); ui->UNIT_M_2->show(); ui->UNIT_M_3->show(); ui->UNIT_M_4->show();
        ui->START_DELAY_EDIT_U->hide(); ui->START_DELAY_EDIT_N->hide();
        ui->END_DELAY_EDIT_U->hide(); ui->END_DELAY_EDIT_N->hide();
        ui->START_GW_EDIT_U->hide(); ui->START_GW_EDIT_N->hide();
        ui->END_GW_EDIT_U->hide(); ui->END_GW_EDIT_N->show();
        ui->UNIT_US_1->hide(); ui->UNIT_US_2->hide(); ui->UNIT_US_3->hide(); ui->UNIT_US_4->hide();
        ui->UNIT_NS_1->hide(); ui->UNIT_NS_2->hide(); ui->UNIT_NS_3->hide(); ui->UNIT_NS_4->hide();
        ui->STEP_SIZE_DELAY_EDIT->setText(QString::number(step_size_delay * dist_ns, 'f', 2));
        ui->STEP_SIZE_GW_EDIT->setText(QString::number(step_size_gw * dist_ns, 'f', 2));
        ui->UNIT_1->setText("m"); ui->UNIT_2->setText("m");
        break;
    default: break;
    }
}

void ScanConfig::keyPressEvent(QKeyEvent *event)
{
    static QLineEdit *edit = NULL;
    edit = NULL;
    if (this->focusWidget()) edit = qobject_cast<QLineEdit*>(this->focusWidget());

    switch(event->key()) {
    case Qt::Key_Escape:
        edit ? data_exchange(false), edit->clearFocus() : this->focusWidget() ? this->focusWidget()->clearFocus() : this->reject();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (edit == NULL)                  this->accept();
        else                               data_exchange(true);
        if (edit) this->focusWidget()->clearFocus();
        data_exchange(false);
        break;
    default: break;
    }
    edit = NULL;
}

void ScanConfig::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (this->focusWidget()) this->focusWidget()->clearFocus();
    this->setFocus();
}

void ScanConfig::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();

    QDialog::mousePressEvent(event);
}

void ScanConfig::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed) {
        // use globalPos instead of pos to prevent window shaking
        window()->move(window()->pos() + event->globalPos() - prev_pos);
        prev_pos = event->globalPos();
    }
    else {
        setCursor(cursor_curr_pointer);
    }
    QDialog::mouseMoveEvent(event);
}

void ScanConfig::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = false;

    QDialog::mouseReleaseEvent(event);
}

