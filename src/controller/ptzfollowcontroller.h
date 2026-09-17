#ifndef PTZFOLLOWCONTROLLER_H
#define PTZFOLLOWCONTROLLER_H

#include <QElapsedTimer>
#include <QObject>

#include "util/config.h"

// Closed-loop PTZ follow.
//
// Error is the target's angular offset from the optical axis in degrees and
// output is a velocity command in deg/s, so the gains do not have to be
// retuned per zoom. The loop runs at the detection rate (~10 Hz), which with
// inference latency leaves 100-200 ms of dead time — the reason the defaults
// are conservative and D is off.
class PtzFollowController : public QObject
{
    Q_OBJECT

public:
    struct Output {
        double horizontalVelocity = 0.0;
        double verticalVelocity = 0.0;
        bool valid = false;   // false means "send nothing", not "send zero"
    };

    explicit PtzFollowController(QObject *parent = nullptr);

    void setSettings(const Config::PidFollowSettings &settings);
    const Config::PidFollowSettings &settings() const { return m_settings; }

    // Sentinel for "not following". Any other value is a valid id, including
    // negative ones — callers use negative sentinels of their own to identify
    // non-detector sources, so this cannot be a >= 0 test.
    static constexpr int Disengaged = -1;

    // Track being followed. Always resets the loop.
    void engage(int trackId);
    void disengage();
    bool engaged() const { return m_track_id != Disengaged; }
    int trackId() const { return m_track_id; }

    // One measurement. Turret motion is compensated from the loop's own last
    // command rather than from device feedback: the 530 status frame's rate
    // bytes are deliberately not parsed (they would corrupt the commanded
    // velocity), and the commanded rate is what the turret was executing over
    // the interval anyway.
    Output update(double errorHorizontalDeg, double errorVerticalDeg);

    // Detection did not produce the followed target this cycle. Coasts on the
    // last output briefly, then gives up.
    Output updateMissing();

    // Suspends output without losing the loop state, for when something else
    // owns the command slot. Resuming re-arms the timing so the first sample
    // after the pause does not see a huge dt.
    void setPaused(bool paused);
    bool paused() const { return m_paused; }

    double lastErrorHorizontal() const { return m_last_error_h; }
    double lastErrorVertical() const { return m_last_error_v; }
    double lastOutputHorizontal() const { return m_last_output_h; }
    double lastOutputVertical() const { return m_last_output_v; }

signals:
    // For the tuning page: measured error and issued command, per cycle.
    void telemetry(double errorH, double errorV, double outputH, double outputV, bool active);

private:
    struct Axis {
        double integral = 0.0;
        double previousError = 0.0;
        double derivative = 0.0;
        bool primed = false;
    };

    double step(Axis &axis, double error, double dt, double kp, double ki, double kd);
    void resetState();

    Config::PidFollowSettings m_settings;
    Axis m_axis_h;
    Axis m_axis_v;
    int m_track_id = Disengaged;
    bool m_paused = false;

    QElapsedTimer m_elapsed;      // since the previous accepted sample
    QElapsedTimer m_missing;      // since the target was last seen
    double m_last_error_h = 0.0;
    double m_last_error_v = 0.0;
    double m_last_output_h = 0.0;
    double m_last_output_v = 0.0;
};

#endif // PTZFOLLOWCONTROLLER_H
