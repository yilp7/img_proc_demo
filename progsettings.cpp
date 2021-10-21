#include "progsettings.h"
#include "ui_settings.h"

ProgSettings::ProgSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgSettings),
    start_pos(0),
    end_pos(0),
    frame_count(1),
    step_size(0)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);

    ui->START_POS_EDIT->setText(QString::number(start_pos));
    ui->END_POS_EDIT->setText(QString::number(end_pos));
    ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
    ui->STEP_SIZE_EDIT->setText(QString::number(step_size, 'f', 2));

    connect(ui->START_POS_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->END_POS_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->FRAME_COUNT_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
    connect(ui->STEP_SIZE_EDIT, SIGNAL(editingFinished()), SLOT(update_scan()));
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
    }
    else {
        ui->START_POS_EDIT->setText(QString::number(start_pos));
        ui->END_POS_EDIT->setText(QString::number(end_pos));
        ui->FRAME_COUNT_EDIT->setText(QString::number(frame_count));
        ui->STEP_SIZE_EDIT->setText(QString::number(step_size, 'f', 2));
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
        if (!edit) this->accept(); break;
    default: break;
    }
    if (edit) this->focusWidget()->clearFocus(), edit = NULL;
}
