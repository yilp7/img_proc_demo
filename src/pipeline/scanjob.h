#ifndef SCANJOB_H
#define SCANJOB_H

#include <QMetaType>
#include <QString>
#include <QtGlobal>
#include <utility>
#include <vector>

#include "pipeline/scanparams.h"

// What the frame path needs to know about a scan, as one value. ScanController builds it
// on the GUI thread and announces every change; the frame thread works on its own copy.
struct ScanJob {
    quint64 revision   = 0;      // raised with every change, so a reader can tell a new job
    quint64 generation = 0;      // raised when a scan is started, not when one is continued
    bool    running    = false;
    int     ptz_idx    = -1;     // the position and profile ScanController last saw the scan
    int     tcu_idx    = -1;     // at; a frame thread that starts during a scan resumes there
    QString name;                // the scan's folder under the save folder
    ScanParams params;
    std::vector<std::pair<float, float>> ptz_route;  // device angles: horizontal, vertical
    std::vector<float> tcu_route;                    // delay of each profile, ns
    std::vector<float> gate_width_route;             // gate width of each profile, ns
};

// One step of a scan, as the frame thread asks for it. ScanController carries it out on
// the GUI thread: the frame thread itself commands no device.
struct ScanStep {
    quint64 generation = 0;         // the scan this step belongs to
    int     ptz_idx    = -1;        // position reached with this step
    int     tcu_idx    = -1;        // profile reached with this step
    bool    move_ptz   = false;
    float   angle_h    = 0.0f;      // device angle
    float   angle_v    = 0.0f;
    bool    set_delay  = false;
    float   delay_ns   = 0.0f;
    bool    set_gate_width = false;
    float   gate_width_ns  = 0.0f;
};

// it crosses from the frame thread to the GUI thread as a queued signal argument
Q_DECLARE_METATYPE(ScanStep)

#endif // SCANJOB_H
