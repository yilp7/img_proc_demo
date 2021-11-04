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
    save_scan(true),
    kernel(3),
    gamma(1.2),
    log(1.2),
    low_in(0),
    high_in(0.05),
    low_out(0),
    high_out(1),
    auto_rep_freq(true),
    simplify_step(false),
    max_dist(15000)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);

    connect(ui->START_POS_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->END_POS_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->FRAME_COUNT_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->STEP_SIZE_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));

    data_exchange(false);

}

ProgSettings::~ProgSettings()
{
    delete ui;
}

void ProgSettings::data_exchange(bool read)
{
    if (read) {
        start_pos = ui->START_POS_EDIT->text().toInt();
        end_pos = ui->END_POS_EDIT->text().toInt();
        frame_count = ui->FRAME_COUNT_EDIT->text().toInt();
        step_size = ui->STEP_SIZE_EDIT->text().toFloat();
        rep_freq = ui->REP_FREQ_EDIT->text().toFloat();
        save_scan = ui->SAVE_SCAN_CHK->isChecked();

        kernel = ui->KERNEL_EDIT->text().toInt();
        gamma = ui->GAMMA_EDIT->text().toFloat();
        log = ui->LOG_EDIT->text().toFloat();
        low_in = ui->LOW_IN_EDIT->text().toFloat();
        high_in = ui->HIGH_IN_EDIT->text().toFloat();
        low_out = ui->LOW_OUT_EDIT->text().toFloat();
        high_out = ui->HIGH_OUT_EDIT->text().toFloat();

        auto_rep_freq = ui->AUTO_REP_FREQ_CHK->isChecked();
        simplify_step = ui->SIMPLIFY_STEP_CHK->isChecked();
        max_dist = ui->MAX_DIST_EDT->text().toInt();
    }
    else {
        ui->START_POS_EDIT->setText(QString::number(start_pos));
        ui->END_POS_EDIT->setText(QString::number(end_pos));
        ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
        ui->STEP_SIZE_EDIT->setText(QString::number(step_size, 'f', 2));
        ui->SAVE_SCAN_CHK->setChecked(save_scan);

        ui->KERNEL_EDIT->setText(QString::number(kernel));
        ui->GAMMA_EDIT->setText(QString::number(gamma, 'f', 2));
        ui->LOG_EDIT->setText(QString::number(log, 'f', 2));
        ui->LOW_IN_EDIT->setText(QString::number(low_in, 'f', 2));
        ui->HIGH_IN_EDIT->setText(QString::number(high_in, 'f', 2));
        ui->LOW_OUT_EDIT->setText(QString::number(low_out, 'f', 2));
        ui->HIGH_OUT_EDIT->setText(QString::number(high_out, 'f', 2));
        ui->REP_FREQ_EDIT->setText(QString::number(rep_freq, 'f', 2));

        ui->AUTO_REP_FREQ_CHK->setChecked(auto_rep_freq);
        ui->SIMPLIFY_STEP_CHK->setChecked(simplify_step);
        ui->MAX_DIST_EDT->setText(QString::number(max_dist));
    }
}

void ProgSettings::update_scan()
{
    QLineEdit *source = qobject_cast<QLineEdit*>(sender());
    if (source == ui->STEP_SIZE_EDIT) {
        start_pos = ui->START_POS_EDIT->text().toInt();
        end_pos = ui->END_POS_EDIT->text().toInt();
        step_size = ui->STEP_SIZE_EDIT->text().toFloat();
        frame_count = step_size ? (end_pos - start_pos) / step_size : 0;
        ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
        ui->STEP_SIZE_EDIT->setText(QString::number(step_size, 'f', 2));
    }
    else {
        start_pos = ui->START_POS_EDIT->text().toInt();
        end_pos = ui->END_POS_EDIT->text().toInt();
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
        edit ? data_exchange(false) : this->reject(); break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        edit ? data_exchange(true) : this->accept(); break;
    default: break;
    }
    if (edit) this->focusWidget()->clearFocus(), edit = NULL;
}

void ProgSettings::mousePressEvent(QMouseEvent *event)
{
    QDialog::mousePressEvent(event);

    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();
}

void ProgSettings::mouseMoveEvent(QMouseEvent *event)
{
    QDialog::mouseMoveEvent(event);

    if (!pressed) return;
    // use globalPos instead of pos to prevent window shaking
    window()->move(window()->pos() + event->globalPos() - prev_pos);
    prev_pos = event->globalPos();
}

void ProgSettings::mouseReleaseEvent(QMouseEvent *event)
{
    QDialog::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    pressed = false;
}

void ProgSettings::on_SIMPLIFY_STEP_CHK_stateChanged(int arg1)
{
    simplify_step = arg1;
    emit simplify_step_chk_clicked(!arg1);
}

void ProgSettings::on_AUTO_REP_FREQ_CHK_stateChanged(int arg1)
{
    auto_rep_freq = arg1;
}

void ProgSettings::on_SAVE_SCAN_CHK_stateChanged(int arg1)
{
    save_scan = arg1;
}

void ProgSettings::on_MAX_DIST_EDT_editingFinished()
{
    emit max_dist_changed(max_dist);
}
