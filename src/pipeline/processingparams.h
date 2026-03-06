#ifndef PROCESSINGPARAMS_H
#define PROCESSINGPARAMS_H

#include <QSize>
#include <QString>

// Thread-safe parameter snapshot for grab_thread_process().
// GUI thread writes via update_processing_params() under m_params_mutex;
// worker thread copies at loop start.
struct ProcessingParams {
    // main window checkboxes / sliders
    bool  frame_avg_enabled    = false;  // ui->FRAME_AVG_CHECK
    int   frame_avg_mode       = 0;      // ui->FRAME_AVG_OPTIONS
    bool  enable_3d            = false;   // ui->IMG_3D_CHECK
    bool  enhance_enabled      = false;   // ui->IMG_ENHANCE_CHECK
    int   enhance_option       = 0;      // ui->ENHANCE_OPTIONS
    int   brightness           = 0;      // ui->BRIGHTNESS_SLIDER
    int   contrast             = 0;      // ui->CONTRAST_SLIDER
    int   gamma_slider         = 10;     // ui->GAMMA_SLIDER
    bool  pseudocolor_enabled  = false;   // ui->PSEUDOCOLOR_CHK
    bool  show_info            = false;   // ui->INFO_CHECK
    bool  show_center          = false;   // ui->CENTER_CHECK
    bool  select_tool          = false;   // ui->SELECT_TOOL
    bool  dual_display         = false;   // ui->DUAL_DISPLAY_CHK
    bool  mcp_slider_focused   = false;   // ui->MCP_SLIDER->hasFocus()
    QSize hist_display_size;             // ui->HIST_DISPLAY->size()

    // from Preferences
    bool    split              = false;   // pref->split
    float   custom_3d_delay    = 0.0f;   // pref->ui->CUSTOM_3D_DELAY_EDT
    float   custom_3d_gate_width = 0.0f; // pref->ui->CUSTOM_3D_GW_EDT
    QString custom_info_text;            // pref->ui->CUSTOM_INFO_EDT
};

#endif // PROCESSINGPARAMS_H
