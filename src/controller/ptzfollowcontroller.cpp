#include "controller/ptzfollowcontroller.h"

#include <algorithm>
#include <cmath>

namespace {

double clamp(double value, double limit)
{
    return std::max(-limit, std::min(limit, value));
}

} // namespace

PtzFollowController::PtzFollowController(QObject *parent) :
    QObject(parent)
{
}

void PtzFollowController::set_settings(const Config::PidFollowSettings &settings)
{
    m_settings = settings;
    // Integral is in output units, so a changed limit must be applied to what
    // has already accumulated or an old oversized term keeps acting.
    m_axis_h.integral = clamp(m_axis_h.integral, m_settings.integral_limit_deg_s);
    m_axis_v.integral = clamp(m_axis_v.integral, m_settings.integral_limit_deg_s);
}

void PtzFollowController::engage(int track_id)
{
    // Always resets, including when re-engaging the same id: this is only
    // called by explicit operator designation, and carrying the previous
    // target's integral into a fresh one would kick the turret.
    m_track_id = track_id;
    reset_state();
}

void PtzFollowController::disengage()
{
    m_track_id = DISENGAGED;
    reset_state();
}

void PtzFollowController::set_paused(bool paused)
{
    if (paused == m_paused) return;
    m_paused = paused;
    if (!paused) {
        // The gap while paused is not a control interval. Invalidating the
        // timer makes the next sample use the nominal dt instead of a dt
        // covering the whole pause, which would produce a huge D spike.
        m_elapsed.invalidate();
        m_missing.invalidate();
    }
}

void PtzFollowController::reset_state()
{
    m_axis_h = Axis();
    m_axis_v = Axis();
    m_elapsed.invalidate();
    m_missing.invalidate();
    m_last_error_h = 0.0;
    m_last_error_v = 0.0;
    m_last_output_h = 0.0;
    m_last_output_v = 0.0;
}

double PtzFollowController::step(Axis &axis, double error, double dt,
                                 double kp, double ki, double kd)
{
    // Deadband before anything else: inside it the target counts as centred,
    // and letting the integral keep winding there would cause slow drift.
    if (std::fabs(error) < m_settings.deadband_deg) {
        error = 0.0;
        axis.integral *= 0.9;
    }
    else {
        axis.integral += error * dt * ki;
        axis.integral = clamp(axis.integral, m_settings.integral_limit_deg_s);
    }

    double derivative_term = 0.0;
    if (kd != 0.0 && m_settings.derivative_filter > 0.0) {
        if (axis.primed && dt > 0.0) {
            const double raw = (error - axis.previous_error) / dt;
            // First-order low-pass: a raw derivative on a jittering centroid at
            // this sample rate is mostly noise.
            axis.derivative += m_settings.derivative_filter * (raw - axis.derivative);
            derivative_term = kd * axis.derivative;
        }
    }
    axis.previous_error = error;
    axis.primed = true;

    return kp * error + axis.integral + derivative_term;
}

PtzFollowController::Output PtzFollowController::update(double error_horizontal_deg,
                                                        double error_vertical_deg)
{
    Output out;
    if (!is_engaged() || m_paused) {
        emit telemetry(error_horizontal_deg, error_vertical_deg, 0.0, 0.0, false);
        return out;
    }

    // Nominal detection interval; used for the first sample and whenever the
    // measured interval is implausible.
    constexpr double NOMINAL_DT = 0.1;
    double dt = NOMINAL_DT;
    if (m_elapsed.isValid()) {
        dt = m_elapsed.elapsed() / 1000.0;
        if (dt <= 0.0 || dt > 1.0) dt = NOMINAL_DT;
    }
    m_elapsed.restart();
    m_missing.invalidate();

    double error_h = error_horizontal_deg;
    double error_v = error_vertical_deg;
    if (m_settings.compensate_turret_motion) {
        // The frame shows the target one interval ago while the turret kept moving.
        // Removing the angle covered in that interval closes the loop on target motion,
        // not on the turret's own lag.
        error_h -= m_last_output_h * dt;
        error_v -= m_last_output_v * dt;
    }

    m_last_error_h = error_h;
    m_last_error_v = error_v;

    double vh = step(m_axis_h, error_h, dt, m_settings.kp_h, m_settings.ki_h, m_settings.kd_h);
    double vv = step(m_axis_v, error_v, dt, m_settings.kp_v, m_settings.ki_v, m_settings.kd_v);

    const double limit = m_settings.max_velocity_deg_s;
    const double vh_clamped = clamp(vh, limit);
    const double vv_clamped = clamp(vv, limit);
    // Anti-windup: while saturated the integral must not keep growing, or the
    // loop overshoots badly when the target finally comes back in range.
    if (vh_clamped != vh) m_axis_h.integral = clamp(m_axis_h.integral, m_settings.integral_limit_deg_s * 0.5);
    if (vv_clamped != vv) m_axis_v.integral = clamp(m_axis_v.integral, m_settings.integral_limit_deg_s * 0.5);

    m_last_output_h = vh_clamped;
    m_last_output_v = vv_clamped;

    out.horizontal_velocity = vh_clamped;
    out.vertical_velocity = vv_clamped;
    out.valid = true;
    emit telemetry(error_h, error_v, vh_clamped, vv_clamped, true);
    return out;
}

PtzFollowController::Output PtzFollowController::update_missing()
{
    Output out;
    if (!is_engaged() || m_paused) return out;

    if (!m_missing.isValid()) m_missing.start();
    if (m_missing.elapsed() > m_settings.coast_ms) {
        // Giving up has to be an explicit stop rather than silence: the last
        // MANUAL_SEARCH velocity is retransmitted by the TX timer, so simply
        // stopping would leave the turret slewing indefinitely.
        disengage();
        out.horizontal_velocity = 0.0;
        out.vertical_velocity = 0.0;
        out.valid = true;
        emit telemetry(0.0, 0.0, 0.0, 0.0, false);
        return out;
    }

    // Short gap: hold the last command. The target is probably still moving as
    // it was, and re-commanding zero would jerk the turret on every dropout.
    out.horizontal_velocity = m_last_output_h;
    out.vertical_velocity = m_last_output_v;
    out.valid = true;
    emit telemetry(m_last_error_h, m_last_error_v, m_last_output_h, m_last_output_v, true);
    return out;
}
