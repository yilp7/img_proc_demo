#include "controller/auxpanelmanager.h"
#include "ui_user_panel.h"

#include <QButtonGroup>
#include <QFont>

AuxPanelManager::AuxPanelManager(QObject *parent)
    : QObject(parent),
      m_ui(nullptr),
      display_grp(nullptr),
      alt_display_option(0)
{
}

void AuxPanelManager::init(Ui::UserPanel *ui)
{
    m_ui = ui;

    // set up display info (left bottom corner)
    QStringList alt_options_str;
    alt_options_str << "DATA" << "HIST" << "PTZ" << "ALT" << "ADDON" << "VID" << "YOLO";
    m_ui->MISC_OPTION_1->addItems(alt_options_str);
    m_ui->MISC_OPTION_2->addItems(alt_options_str);
    m_ui->MISC_OPTION_3->addItems(alt_options_str);
    m_ui->MISC_OPTION_1->setCurrentIndex(0);
    m_ui->MISC_OPTION_2->setCurrentIndex(2);
    m_ui->MISC_OPTION_3->setCurrentIndex(5);

    QFont temp_f("consolas", 8);
    connect(m_ui->MISC_OPTION_1, SIGNAL(selected()), m_ui->MISC_RADIO_1, SLOT(click()));
    connect(m_ui->MISC_OPTION_2, SIGNAL(selected()), m_ui->MISC_RADIO_2, SLOT(click()));
    connect(m_ui->MISC_OPTION_3, SIGNAL(selected()), m_ui->MISC_RADIO_3, SLOT(click()));
    m_ui->MISC_OPTION_1->view()->setFont(temp_f);
    m_ui->MISC_OPTION_2->view()->setFont(temp_f);
    m_ui->MISC_OPTION_3->view()->setFont(temp_f);

    display_grp = new QButtonGroup(this);
    display_grp->addButton(m_ui->MISC_RADIO_1);
    display_grp->addButton(m_ui->MISC_RADIO_2);
    display_grp->addButton(m_ui->MISC_RADIO_3);
    display_grp->setExclusive(true);
    m_ui->MISC_RADIO_1->setChecked(true);
    m_ui->MISC_DISPLAY->setCurrentIndex(1);
}

void AuxPanelManager::on_MISC_RADIO_1_clicked()
{
    alt_display_option = m_ui->MISC_OPTION_1->currentIndex() + 1;
    m_ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
}

void AuxPanelManager::on_MISC_RADIO_2_clicked()
{
    alt_display_option = m_ui->MISC_OPTION_2->currentIndex() + 1;
    m_ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
}

void AuxPanelManager::on_MISC_RADIO_3_clicked()
{
    alt_display_option = m_ui->MISC_OPTION_3->currentIndex() + 1;
    m_ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
}

void AuxPanelManager::on_MISC_OPTION_1_currentIndexChanged(int index)
{
    alt_display_option = index + 1;
    m_ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
    m_ui->MISC_RADIO_1->setChecked(true);
}

void AuxPanelManager::on_MISC_OPTION_2_currentIndexChanged(int index)
{
    alt_display_option = index + 1;
    m_ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
    m_ui->MISC_RADIO_2->setChecked(true);
}

void AuxPanelManager::on_MISC_OPTION_3_currentIndexChanged(int index)
{
    alt_display_option = index + 1;
    m_ui->MISC_DISPLAY->setCurrentIndex(alt_display_option);
    m_ui->MISC_RADIO_3->setChecked(true);
}
