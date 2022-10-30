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
    step_size_delay(0),
    step_size_gw(0),
    frame_count(1),
    rep_freq(10),
    capture_scan_ori(true),
    capture_scan_res(false),
    record_scan_ori(false),
    record_scan_res(true)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    connect(ui->START_DELAY_EDIT_U,   SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->START_DELAY_EDIT_N,   SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->START_DELAY_EDIT_M,   SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->END_DELAY_EDIT_U,     SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->END_DELAY_EDIT_N,     SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->END_DELAY_EDIT_M,     SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->START_GW_EDIT_U,      SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->START_GW_EDIT_N,      SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->START_GW_EDIT_M,      SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->END_GW_EDIT_U,        SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->END_GW_EDIT_N,        SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->END_GW_EDIT_M,        SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->FRAME_COUNT_EDIT,     SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->STEP_SIZE_DELAY_EDIT, SIGNAL(editingFinished()), this, SLOT(update_scan()));
    connect(ui->STEP_SIZE_GW_EDIT,    SIGNAL(editingFinished()), this, SLOT(update_scan()));

    connect(ui->CAPTURE_SCAN_ORI_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ capture_scan_ori = arg1; });
    connect(ui->CAPTURE_SCAN_RES_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ capture_scan_res = arg1; });
    connect(ui->RECORD_SCAN_ORI_CHK,  &QCheckBox::stateChanged, this, [this](int arg1){ record_scan_ori = arg1; });
    connect(ui->RECORD_SCAN_RES_CHK,  &QCheckBox::stateChanged, this, [this](int arg1){ record_scan_res = arg1; });

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
    }
}

void ScanConfig::update_scan()
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
        if (!step_size_delay) step_size_delay = 1;
        frame_count = 1.0 * (ending_delay - starting_delay) / step_size_delay;
        if (!frame_count) frame_count = 1;
        step_size_gw = 1.0 * (ending_gw - starting_gw) / frame_count;
    }
    else if (source == ui->STEP_SIZE_GW_EDIT) {
        step_size_gw = ui->STEP_SIZE_GW_EDIT->text().toFloat();
        if (!step_size_gw) step_size_gw = 1;
        frame_count = 1.0 * (ending_delay - starting_delay) / step_size_delay;
        if (!frame_count) frame_count = 1;
        step_size_delay = 1.0 * (ending_delay - starting_delay) / frame_count;
    }
    else {
        frame_count = ui->FRAME_COUNT_EDIT->text().toInt();
        if (!frame_count) frame_count = 1;
        step_size_delay = 1.0 * (ending_delay - starting_delay) / frame_count;
        step_size_gw = 1.0 * (ending_gw - starting_gw) / frame_count;
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

