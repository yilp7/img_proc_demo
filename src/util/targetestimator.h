#ifndef TARGETESTIMATOR_H
#define TARGETESTIMATOR_H

#include <QRect>
#include <QSize>

namespace TargetEstimator {

struct Optics {
    // Range inputs: focal length at the current zoom, and the sensor diagonal
    // used to derive pixel pitch. Both are zero by default so an Optics that
    // was never filled from config yields no range rather than a plausible
    // wrong one — panorama_optics in default.json is the only source.
    double focalLengthMm = 0.0;
    double sensorDiagonalMm = 0.0;
    // Bearing inputs: full-frame FOV at the current zoom, interpolated from
    // the panorama_optics.zoom_fov calibration.
    double fovHorizontalDeg = 38.4;
    double fovVerticalDeg = 22.5;
};

struct Bearing {
    double azimuthDeg = 0.0;    // clockwise from true north
    double elevationDeg = 0.0;  // positive above the horizontal plane
};

// Range from a known physical height and the box's pixel height:
//   range = height * focal_px / box_px,  focal_px = frame_h / (2 tan(vfov/2))
// Returns 0 when the inputs cannot support an estimate (unknown class height,
// degenerate box or frame). Accuracy is linear in the FOV, so it is only as
// good as the zoom value behind `optics`.
double rangeMeters(double targetHeightMeters, const QRect &sourceBox,
                   const QSize &sourceSize, const Optics &optics);

// Angular offset of the box centre from the frame centre, in degrees.
// Rectilinear (tangent) projection rather than a linear pixels-to-degrees
// ratio; the two diverge noticeably at the wide end of this lens.
void offsetFromCentreDeg(const QRect &sourceBox, const QSize &sourceSize,
                         const Optics &optics, double *horizontalDeg, double *verticalDeg);

// Hull-frame direction (turret angles plus the in-frame offset above) rotated
// out to true north / horizontal, using the platform attitude. This is the
// inverse of the G1 geographic-to-hull rotation; with a zero attitude it is
// the identity. Same assumptions apply: turret not gyro-stabilised, positive
// roll starboard-down.
Bearing hullToGeographic(double hullAzimuthDeg, double hullElevationDeg,
                         double headingDeg, double pitchDeg, double rollDeg);

} // namespace TargetEstimator

#endif // TARGETESTIMATOR_H
