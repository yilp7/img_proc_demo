#include "developeroptions.h"
#include "ui_developeroptions.h"

DeveloperOptions::DeveloperOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeveloperOptions)
{
    ui->setupUi(this);
}

DeveloperOptions::~DeveloperOptions()
{
    delete ui;
}
