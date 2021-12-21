#include "progsettings.h"
#include "ui_settings.h"

ProgSettings::ProgSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgSettings),
    start_pos(0),
    end_pos(0),
    frame_count(1),
    step_size(0),
    rep_freq(10),
    save_scan_ori(true),
    save_scan_res(false),
    kernel(3),
    gamma(1.2),
    log(1.2),
    accu_base(1),
    low_in(0),
    high_in(0.05),
    low_out(0),
    high_out(1),
    dehaze_pct(0.95),
    sky_tolerance(40),
    fast_gf(1),
    dist_ns(3e8 / 2e9),
    auto_rep_freq(true),
    hz_unit(0),
    base_unit(0),
    max_dist(15000),
    laser_grp(NULL),
    laser_on(0),
    com_idx(0),
    cameralink(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);

    ui->IP_EDIT->setEnabled(false);

    ui->HZ_LIST->addItem("kHz");
    ui->HZ_LIST->addItem("Hz");
    ui->HZ_LIST->installEventFilter(this);

    ui->UNIT_LIST->addItem("ns");
    ui->UNIT_LIST->addItem("μs");
    ui->UNIT_LIST->addItem("m");
    ui->UNIT_LIST->setCurrentIndex(0);
    ui->UNIT_LIST->installEventFilter(this);

    connect(ui->START_POS_EDIT_U, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->START_POS_EDIT_N, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->END_POS_EDIT_U, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->END_POS_EDIT_N, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->FRAME_COUNT_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->STEP_SIZE_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));

    laser_grp = new QButtonGroup();
    laser_grp->addButton(ui->LASER_CHK_1, 0);
    laser_grp->addButton(ui->LASER_CHK_2, 1);
    laser_grp->addButton(ui->LASER_CHK_3, 2);
    laser_grp->addButton(ui->LASER_CHK_4, 3);
    laser_grp->setExclusive(false);
    connect(laser_grp, SIGNAL(buttonToggled(int, bool)), SLOT(toggle_laser(int, bool)));

    ui->BAUDRATE_LIST->addItem("9600");
    ui->BAUDRATE_LIST->addItem("115200");
    ui->BAUDRATE_LIST->installEventFilter(this);

    ui->COM_LIST->addItem("TCU");
    ui->COM_LIST->addItem("RANGE");
    ui->COM_LIST->addItem("LENS");
    ui->COM_LIST->addItem("LASER");
    ui->COM_LIST->installEventFilter(this);

    QFont temp = QFont(consolas);
    temp.setPointSize(8);
    ui->COM_DATA_EDT->setFont(temp);

    data_exchange(false);

}

ProgSettings::~ProgSettings()
{
    delete ui;
}

void ProgSettings::data_exchange(bool read)
{
    if (read) {
        start_pos = ui->START_POS_EDIT_U->text().toInt() * 1000 + ui->START_POS_EDIT_N->text().toInt();
        end_pos = ui->END_POS_EDIT_U->text().toInt() * 1000 + ui->END_POS_EDIT_N->text().toInt();
        frame_count = ui->FRAME_COUNT_EDIT->text().toInt();
        step_size = ui->STEP_SIZE_EDIT->text().toFloat();
        rep_freq = ui->REP_FREQ_EDIT->text().toFloat();
        save_scan_ori = ui->SAVE_SCAN_ORI_CHK->isChecked();
        save_scan_res = ui->SAVE_SCAN_RES_CHK->isChecked();
        switch (hz_unit) {
        // kHz
        case 0: rep_freq = ui->REP_FREQ_EDIT->text().toFloat(); break;
        // Hz
        case 1: rep_freq = ui->REP_FREQ_EDIT->text().toFloat() / 1000; break;
        default: break;
        }

        kernel = ui->KERNEL_EDIT->text().toInt();
        gamma = ui->GAMMA_EDIT->text().toFloat();
        log = ui->LOG_EDIT->text().toFloat();
        accu_base = ui->ACCU_BASE_EDIT->text().toFloat();
        low_in = ui->LOW_IN_EDIT->text().toFloat();
        high_in = ui->HIGH_IN_EDIT->text().toFloat();
        low_out = ui->LOW_OUT_EDIT->text().toFloat();
        high_out = ui->HIGH_OUT_EDIT->text().toFloat();
        dehaze_pct = ui->DEHAZE_PCT_EDIT->text().toFloat();
        sky_tolerance = ui->SKY_TOLERANCE_EDIT->text().toFloat();
        fast_gf = ui->FAST_GF_EDIT->text().toInt();

        auto_rep_freq = ui->AUTO_REP_FREQ_CHK->isChecked();
        base_unit = ui->UNIT_LIST->currentIndex();
        switch (base_unit) {
        // ns
        case 0: max_dist = ui->MAX_DIST_EDT->text().toInt() * dist_ns; break;
        // μs
        case 1: max_dist = ui->MAX_DIST_EDT->text().toInt() * dist_ns * 1000; break;
        // m
        case 2: max_dist = ui->MAX_DIST_EDT->text().toInt(); break;
        default: break;
        }
        laser_on = 0;
        laser_on ^= ui->LASER_CHK_1->isChecked() << 0;
        laser_on ^= ui->LASER_CHK_2->isChecked() << 1;
        laser_on ^= ui->LASER_CHK_3->isChecked() << 2;
        laser_on ^= ui->LASER_CHK_4->isChecked() << 3;
    }
    else {
        ui->START_POS_EDIT_N->setText(QString::number(start_pos % 1000));
        ui->START_POS_EDIT_U->setText(QString::number(start_pos / 1000));
        ui->END_POS_EDIT_N->setText(QString::number(end_pos % 1000));
        ui->END_POS_EDIT_U->setText(QString::number(end_pos / 1000));
        ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
        ui->STEP_SIZE_EDIT->setText(QString::number(step_size, 'f', 2));
        ui->SAVE_SCAN_ORI_CHK->setChecked(save_scan_ori);
        ui->SAVE_SCAN_RES_CHK->setChecked(save_scan_res);
        switch (hz_unit) {
        // kHz
        case 0: ui->REP_FREQ_EDIT->setText(QString::number((int)rep_freq)); ui->FREQ_UNIT->setText("kHz"); break;
        // Hz
        case 1: ui->REP_FREQ_EDIT->setText(QString::number((int)(rep_freq / 1000))); ui->FREQ_UNIT->setText("Hz"); break;
        default: break;
        }

        ui->KERNEL_EDIT->setText(QString::number(kernel));
        ui->GAMMA_EDIT->setText(QString::number(gamma, 'f', 2));
        ui->LOG_EDIT->setText(QString::number(log, 'f', 2));
        ui->ACCU_BASE_EDIT->setText(QString::number(accu_base, 'f', 2));
        ui->LOW_IN_EDIT->setText(QString::number(low_in, 'f', 2));
        ui->HIGH_IN_EDIT->setText(QString::number(high_in, 'f', 2));
        ui->LOW_OUT_EDIT->setText(QString::number(low_out, 'f', 2));
        ui->HIGH_OUT_EDIT->setText(QString::number(high_out, 'f', 2));
        ui->DEHAZE_PCT_EDIT->setText(QString::number(dehaze_pct, 'f', 2));
        ui->SKY_TOLERANCE_EDIT->setText(QString::number(sky_tolerance, 'f', 2));
        ui->FAST_GF_EDIT->setText(QString::number(fast_gf));

        ui->AUTO_REP_FREQ_CHK->setChecked(auto_rep_freq);
        ui->UNIT_LIST->setCurrentIndex(base_unit);
        switch (base_unit) {
        // ns
        case 0: ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist / dist_ns))); break;
        // μs
        case 1: ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist / dist_ns / 1000))); break;
        // m
        case 2: ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist))); break;
        default: break;
        }
        ui->LASER_CHK_1->setChecked(laser_on & 0b0001);
        ui->LASER_CHK_2->setChecked(laser_on & 0b0010);
        ui->LASER_CHK_3->setChecked(laser_on & 0b0100);
        ui->LASER_CHK_4->setChecked(laser_on & 0b1000);
    }
}

void ProgSettings::display_baudrate(int id, int baudrate)
{
    if (id != ui->COM_LIST->currentIndex()) return;
    int idx = 0;
    switch (baudrate) {
    case 9600: idx = 0; break;
    case 115200: idx = 1; break;
    default: break;
    }

    ui->BAUDRATE_LIST->setCurrentIndex(idx);
}

void ProgSettings::enable_ip_editing(bool enable)
{
    ui->IP_EDIT->setEnabled(enable);
}

void ProgSettings::config_ip(bool read, int ip, int gateway)
{
    if (read) {
        ui->IP_EDIT->setText(QString::asprintf("%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF));
    }
    else {
        int ip1, ip2, ip3, ip4;
        sscanf(ui->IP_EDIT->text().toLatin1().constData(), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
        emit set_dev_ip((ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4, (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + 1);
    }
}

void ProgSettings::update_scan()
{
    QLineEdit *source = qobject_cast<QLineEdit*>(sender());
    if (source == ui->STEP_SIZE_EDIT) {
        start_pos = ui->START_POS_EDIT_U->text().toInt() * 1000 + ui->START_POS_EDIT_N->text().toInt();
        end_pos = ui->END_POS_EDIT_U->text().toInt() * 1000 + ui->END_POS_EDIT_N->text().toInt();
        step_size = ui->STEP_SIZE_EDIT->text().toFloat();
        frame_count = step_size ? (end_pos - start_pos) / step_size : 0;
        ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
        ui->STEP_SIZE_EDIT->setText(QString::number(step_size, 'f', 2));
    }
    else {
        start_pos = ui->START_POS_EDIT_U->text().toInt() * 1000 + ui->START_POS_EDIT_N->text().toInt();
        end_pos = ui->END_POS_EDIT_U->text().toInt() * 1000 + ui->END_POS_EDIT_N->text().toInt();
        frame_count = ui->FRAME_COUNT_EDIT->text().toInt();
        if (!frame_count) frame_count = 1;
        step_size = 1.0 * (end_pos - start_pos) / frame_count;
        ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
        ui->STEP_SIZE_EDIT->setText(QString::number(step_size, 'f', 2));
    }
}

void ProgSettings::keyPressEvent(QKeyEvent *event)
{
    static QLineEdit *edit = NULL;
    if (this->focusWidget()) edit = qobject_cast<QLineEdit*>(this->focusWidget());

    switch(event->key()) {
    case Qt::Key_Escape:
        edit ? data_exchange(false), this->focusWidget()->clearFocus() : this->reject();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (edit == NULL)                  this->accept();
        else if (edit == ui->COM_DATA_EDT) send_cmd();
        else if (edit == ui->IP_EDIT)      config_ip(false);
        else                               data_exchange(true);
        if (edit) this->focusWidget()->clearFocus();
//        edit ? data_exchange(true) : this->accept(); break;
    default: break;
    }
    edit = NULL;
}

void ProgSettings::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();

    QDialog::mousePressEvent(event);
}

void ProgSettings::mouseMoveEvent(QMouseEvent *event)
{
    if (!pressed) return;
    // use globalPos instead of pos to prevent window shaking
    window()->move(window()->pos() + event->globalPos() - prev_pos);
    prev_pos = event->globalPos();

    QDialog::mouseMoveEvent(event);
}

void ProgSettings::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = false;

    QDialog::mouseReleaseEvent(event);
}

bool ProgSettings::eventFilter(QObject *obj, QEvent *event)
{
    if (qobject_cast<QComboBox*>(obj) && event->type() == QEvent::MouseMove) return true;
    return QDialog::eventFilter(obj, event);
}

void ProgSettings::on_SAVE_SCAN_ORI_CHK_stateChanged(int arg1)
{
    save_scan_ori = arg1;
}

void ProgSettings::on_SAVE_SCAN_RES_CHK_stateChanged(int arg1)
{
    save_scan_ori = arg1;
}

void ProgSettings::on_HZ_LIST_currentIndexChanged(int index)
{
    hz_unit = index;
    emit rep_freq_unit_changed(index);
    switch (hz_unit) {
    // kHz
    case 0: ui->REP_FREQ_EDIT->setText(QString::number((int)rep_freq)); ui->FREQ_UNIT->setText("kHz"); break;
    // Hz
    case 1: ui->REP_FREQ_EDIT->setText(QString::number((int)(rep_freq * 1000))); ui->FREQ_UNIT->setText("Hz"); break;
    default: break;
    }
}

void ProgSettings::on_UNIT_LIST_currentIndexChanged(int index)
{
    base_unit = index;
    emit base_unit_changed(index);
    switch (base_unit) {
    // ns
    case 0: ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist / dist_ns))); ui->MAX_DIST_UNIT->setText("ns"); break;
    // μs
    case 1: ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist / dist_ns / 1000))); ui->MAX_DIST_UNIT->setText("μs"); break;
    // m
    case 2: ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist))); ui->MAX_DIST_UNIT->setText("m"); break;
    default: break;
    }
}
void ProgSettings::on_AUTO_REP_FREQ_CHK_stateChanged(int arg1)
{
    auto_rep_freq = arg1;
}

void ProgSettings::on_MAX_DIST_EDT_editingFinished()
{
    emit max_dist_changed(max_dist);
}

void ProgSettings::toggle_laser(int id, bool on)
{
    if (on) laser_on += 1 << id;
    else    laser_on -= 1 << id;
    emit laser_toggled(laser_on);
}

void ProgSettings::send_cmd()
{
    QString send_str = ui->COM_DATA_EDT->text().simplified();
    send_str.replace(" ", "");
//    qDebug("%s\n", send_str.toLatin1().data());
    bool ok;
    QByteArray cmd(send_str.length() / 2, 0x00);
    for (int i = 0; i < send_str.length() / 2; i++) cmd[i] = send_str.mid(i * 2, 2).toInt(&ok, 16);

    emit com_write(ui->COM_LIST->currentIndex(), cmd);
}

void ProgSettings::on_BAUDRATE_LIST_activated(const QString &arg1)
{
    emit change_baudrate(ui->COM_LIST->currentIndex(), arg1.toInt());
}

void ProgSettings::on_SHARE_CHK_stateChanged(int arg1)
{
    emit share_serial_port(arg1);
}

void ProgSettings::on_COM_LIST_currentIndexChanged(int index)
{
    emit get_baudrate(index);
}

void ProgSettings::on_AUTO_MCP_CHK_stateChanged(int arg1)
{
    emit auto_mcp(arg1);
}

void ProgSettings::on_CAMERALINK_CHK_stateChanged(int arg1)
{
    cameralink = arg1;
}

void ProgSettings::on_FAST_GF_EDIT_editingFinished()
{
    int val = ui->FAST_GF_EDIT->text().toInt();
    if (val <  1) val = 1;
    if (val > 10) val = 10;
    ui->FAST_GF_EDIT->setText(QString::number(val));
}

