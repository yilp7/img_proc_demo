#ifndef PROCESSINGPARAMS_H
#define PROCESSINGPARAMS_H

#include <QSize>
#include <QString>
#include <opencv2/core.hpp>

// What the frame path needs from the TCU side, in the units of TCUController.
struct TcuReadout {
    int     base_unit     = 0;
    float   delay_dist    = 0.0f;   // m
    float   depth_of_view = 0.0f;   // m
    float   dist_ns       = 0.0f;   // m per ns
    bool    auto_mcp      = false;
    double  mcp           = 0.0;    // as the TCU port holds it
    double  offset_delay  = 0.0;    // ns, as the TCU port holds it
    double  offset_gw     = 0.0;    // ns, as the TCU port holds it
    QString note;                   // TCU state as JSON text, written into saved BMP files
};

// Snapshot of UI and controller state for the pipeline threads. The GUI thread writes it
// through update_processing_params() under m_params_mutex; the worker copies it at loop start.
// Per-display toggles live in PipelineConfig.
struct ProcessingParams {
    // global UI sliders / toggles (single value applies to whichever
    // display the pipeline is producing for).
    int   brightness           = 0;      // ui->BRIGHTNESS_SLIDER
    int   contrast             = 0;      // ui->CONTRAST_SLIDER
    int   gamma_slider         = 10;     // ui->GAMMA_SLIDER
    bool  show_info            = false;   // ui->INFO_CHECK
    bool  show_center          = false;   // ui->CENTER_CHECK
    bool  select_tool          = false;   // ui->SELECT_TOOL
    bool  dual_display         = false;   // ui->DUAL_DISPLAY_CHK
    bool  mcp_slider_focused   = false;   // ui->MCP_SLIDER->hasFocus()
    QSize hist_display_size;             // ui->HIST_DISPLAY->size()

    // display rotation (still global; not on the picker-controlled GENERAL tab)
    int   image_rotate         = 0;

    // display selection rect (from Display widget)
    cv::Point selection_v1;
    cv::Point selection_v2;
    cv::Rect  display_region;
    int       display_width = 0;
    int       display_height = 0;

    // from the setup overlay
    bool    split              = false;
    QString custom_info_text;

    // from the controllers and the display widgets
    TcuReadout tcu;
    int     alt_display_option = 0;      // AuxPanelManager; 2 shows the histogram
    QSize   display_size[3];             // widget size, by DisplayId
    QSize   secondary_size;              // display of the second window
    QString save_location;
};

#endif // PROCESSINGPARAMS_H
