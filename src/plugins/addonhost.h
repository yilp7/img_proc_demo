#ifndef ADDONHOST_H
#define ADDONHOST_H

#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>

#include "pipeline/detectedtarget.h"

// What an addon may ask of the application, and what the application tells
// every addon. The panel implements the virtuals once; an addon receives the
// object through PluginInterface::attach() and connects to the signals it
// cares about. An addon compiles against this header, plugininterface.h and
// detectedtarget.h only; anything else it needs is an answer from here, not a
// pointer into the application.
class AddonHost : public QObject
{
    Q_OBJECT

public:
    explicit AddonHost(QObject *parent = nullptr) : QObject(parent) {}

    enum LensOperation {
        VisibleZoomIn,
        VisibleZoomOut,
        VisibleFocusNear,
        VisibleFocusFar,
        InfraredZoomIn,
        InfraredZoomOut,
        InfraredFocusNear,
        InfraredFocusFar
    };

    // Turret. Angles are in the device frame unless the name says display.
    virtual float ptz_angle_h() const = 0;
    virtual float ptz_angle_v() const = 0;
    virtual float ptz_display_angle_h(float device_angle_h) const = 0;
    virtual float ptz_device_angle_h(float display_angle_h) const = 0;
    virtual float ptz_zero_angle_h() const = 0;
    virtual bool ptz_connected() const = 0;
    virtual void ptz_send_angle(float h, float v) = 0;
    virtual void ptz_move_velocity(float h, float v) = 0;
    virtual void ptz_stop() = 0;
    // Device-side follow of whatever is centred now.
    virtual void ptz_follow_centre() = 0;

    // Lens, as held operations: pressed until released.
    virtual void lens_press(LensOperation operation) = 0;
    virtual void lens_release() = 0;

    // Imaging channels, laser and the gate.
    virtual bool visible_light_on() const = 0;
    virtual bool infrared_on() const = 0;
    virtual bool laser_enabled() const = 0;
    virtual void set_channel_states(bool visible_light, bool infrared) = 0;
    virtual void set_laser_enabled(bool enabled) = 0;
    virtual void set_laser_width(float ns) = 0;
    virtual void set_delay(int metres) = 0;
    virtual void set_gate_width(int metres) = 0;
    virtual void set_gain(int value) = 0;

    // Pipeline.
    virtual void set_enhance(bool enabled, int option) = 0;
    // A negative mode leaves the current one in place.
    virtual void set_frame_average(bool enabled, int mode) = 0;

    // Targets. The class table and the optics stay with the application; an
    // addon gets answers about a box, not the tables behind them.
    virtual bool device_tracking() const = 0;
    virtual bool target_class(int model_idx, const QString &class_name,
                              int *type, int *category, double *height_m) const = 0;
    virtual void target_offset_deg(const QRect &box, const QSize &frame,
                                   double *horizontal, double *vertical) const = 0;
    virtual double target_range_m(double height_m, const QRect &box, const QSize &frame) const = 0;

    // Heading reference for the radar. Any source may call it; the widget
    // expires a reference that stops arriving.
    virtual void set_heading_reference(float heading_deg, bool valid) = 0;

    // Called by the application; each one fans out to every addon.
    void publish_targets(const QVector<DetectedTarget> &targets) { emit targets_updated(targets); }
    void publish_ptz_angle(float h, float v) { emit ptz_angle_changed(h, v); }
    void notify_operator_took_turret() { emit operator_took_turret(); }
    void notify_operator_took_lens() { emit operator_took_lens(); }
    void notify_tracking_state_changed() { emit tracking_state_changed(); }

signals:
    void targets_updated(const QVector<DetectedTarget> &targets);
    void ptz_angle_changed(float h, float v);
    // Manual controls took the shared PTZ command slot; an addon holding a
    // baseline against it must drop it.
    void operator_took_turret();
    void operator_took_lens();
    void tracking_state_changed();
};

#endif // ADDONHOST_H
