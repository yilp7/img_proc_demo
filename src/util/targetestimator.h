#ifndef TARGETESTIMATOR_H
#define TARGETESTIMATOR_H

#include <QRect>
#include <QSize>

namespace TargetEstimator {

struct Optics {
    // Range inputs: focal length at the current zoom and the sensor diagonal, which
    // gives the pixel pitch. Both default to zero so an Optics never filled from the
    // config yields no range instead of a plausible wrong one.
    double focal_length_mm = 0.0;
    double sensor_diagonal_mm = 0.0;
    // Bearing inputs: full-frame FOV at the current zoom, interpolated from
    // the panorama_optics.zoom_fov calibration.
    double fov_horizontal_deg = 38.4;
    double fov_vertical_deg = 22.5;
};

struct Bearing {
    double azimuth_deg = 0.0;    // clockwise from true north
    double elevation_deg = 0.0;  // positive above the horizontal plane
};

// Range from a known physical height and the box's pixel height. Returns 0 when
// the inputs cannot support an estimate (unknown class height, degenerate box or
// frame). Accuracy is linear in the FOV, so it is only as good as the zoom value.
double range_meters(double target_height_meters, const QRect &source_box,
                    const QSize &source_size, const Optics &optics);

// Angular offset of the box centre from the frame centre, in degrees.
// Rectilinear (tangent) projection rather than a linear pixels-to-degrees
// ratio; the two diverge noticeably at the wide end of this lens.
void offset_from_centre_deg(const QRect &source_box, const QSize &source_size,
                            const Optics &optics, double *horizontal_deg, double *vertical_deg);

// Hull-frame direction (turret angles plus the in-frame offset) rotated to true
// north and the horizontal plane with the platform attitude: the inverse of the G1
// rotation. Assumes the turret is not gyro-stabilised and positive roll is starboard-down.
Bearing hull_to_geographic(double hull_azimuth_deg, double hull_elevation_deg,
                           double heading_deg, double pitch_deg, double roll_deg);

} // namespace TargetEstimator

#endif // TARGETESTIMATOR_H
