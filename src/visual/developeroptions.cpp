#include "developeroptions.h"
#include "ui_developeroptions.h"

DeveloperOptions::DeveloperOptions(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::DeveloperOptions)
{
    m_ui->setupUi(this);
}

DeveloperOptions::~DeveloperOptions()
{
    delete m_ui;
}
