#include "preferences.h"
#include "ui_preferences.h"

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preferences)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    ui->DEVICES_TAB->setup(0, ui->SEP_0->pos().y() + 10);
    ui->SERIAL_TAB->setup(0, ui->SEP_1->pos().y() + 10);
    ui->TCU_TAB->setup(0, ui->SEP_2->pos().y() + 10);
    ui->IMG_PROC_TAB->setup(0, ui->SEP_3->pos().y() + 10);

    connect(ui->DEVICES_TAB, SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
    connect(ui->SERIAL_TAB, SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
    connect(ui->TCU_TAB, SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
    connect(ui->IMG_PROC_TAB, SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
}

Preferences::~Preferences()
{
    delete ui;
}

void Preferences::jump_to_content(int pos)
{
    ui->PREFERENCES->verticalScrollBar()->setSliderPosition(pos);
}
