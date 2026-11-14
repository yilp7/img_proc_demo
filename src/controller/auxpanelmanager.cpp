#include "controller/auxpanelmanager.h"

AuxPanelManager::AuxPanelManager(QObject *parent) :
    QObject(parent),
    m_alt_display_option(0)
{
}

void AuxPanelManager::select_display(int combo_index)
{
    m_alt_display_option = combo_index + 1;
    emit display_page_changed(m_alt_display_option);
}
