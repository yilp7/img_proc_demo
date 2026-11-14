#ifndef PTZFOLLOWCONTROLLER_H
#define PTZFOLLOWCONTROLLER_H

#include <QElapsedTimer>
#include <QObject>

#include "util/config.h"

// Closed-loop PTZ follow. Error is the target's angular offset in degrees and the
// output a velocity in deg/s, so gains hold across zoom. The loop runs at the
// detection rate with 100-200 ms of dead time, hence conservative defaults and no D.
class PtzFollowController : public QObject
{
    Q_OBJECT

public:
    struct Output {
        double horizontal_velocity = 0.0;
        double vertical_velocity = 0.0;
        bool valid = false;   // false means "send nothing", not "send zero"
    };

    explicit PtzFollowController(QObject *parent = nullptr);

    void set_settings(const Config::PidFollowSettings &settings);
    const Config::PidFollowSettings &get_settings() const { return m_settings; }

    // Sentinel for "not following". Any other value is a valid id, negative ones
    // included: callers use their own negative ids for non-detector sources.
    static constexpr int DISENGAGED = -1;

    // Track being followed. Always resets the loop.
    void engage(int track_id);
    void disengage();
    bool is_engaged() const { return m_track_id != DISENGAGED; }
    int get_track_id() const { return m_track_id; }

    // One measurement. Turret motion is compensated from the loop's own last command,
    // not from device feedback: the 530 status frame's rate bytes are not parsed, and
    // the commanded rate is what the turret was executing anyway.
    Output update(double error_horizontal_deg, double error_vertical_deg);

    // Detection did not produce the followed target this cycle. Coasts on the
    // last output briefly, then gives up.
    Output update_missing();

    // Suspends output without losing the loop state, for when something else
    // owns the command slot. Resuming re-arms the timing so the first sample
    // after the pause does not see a huge dt.
    void set_paused(bool paused);
    bool is_paused() const { return m_paused; }

    double get_last_error_horizontal() const { return m_last_error_h; }
    double get_last_error_vertical() const { return m_last_error_v; }
    double get_last_output_horizontal() const { return m_last_output_h; }
    double get_last_output_vertical() const { return m_last_output_v; }

signals:
    // For the tuning page: measured error and issued command, per cycle.
    void telemetry(double error_h, double error_v, double output_h, double output_v, bool active);

private:
    struct Axis {
        double integral = 0.0;
        double previous_error = 0.0;
        double derivative = 0.0;
        bool primed = false;
    };

    double step(Axis &axis, double error, double dt, double kp, double ki, double kd);
    void reset_state();

    Config::PidFollowSettings m_settings;
    Axis m_axis_h;
    Axis m_axis_v;
    int m_track_id = DISENGAGED;
    bool m_paused = false;

    QElapsedTimer m_elapsed;      // since the previous accepted sample
    QElapsedTimer m_missing;      // since the target was last seen
    double m_last_error_h = 0.0;
    double m_last_error_v = 0.0;
    double m_last_output_h = 0.0;
    double m_last_output_v = 0.0;
};

#endif // PTZFOLLOWCONTROLLER_H
