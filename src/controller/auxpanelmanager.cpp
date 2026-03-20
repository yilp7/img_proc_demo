#include "controller/auxpanelmanager.h"

AuxPanelManager::AuxPanelManager(QObject *parent)
    : QObject(parent),
      alt_display_option(0)
{
}

void AuxPanelManager::select_display(int combo_index)
{
    alt_display_option = combo_index + 1;
    emit display_page_changed(alt_display_option);
}
