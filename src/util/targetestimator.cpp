#include "util/targetestimator.h"

#include <QtMath>

#include <cmath>

namespace TargetEstimator {

double rangeMeters(double targetHeightMeters, const QRect &sourceBox,
                   const QSize &sourceSize, const Optics &optics)
{
    if (targetHeightMeters <= 0.0) return 0.0;
    if (sourceBox.height() <= 0) return 0.0;
    if (optics.focalLengthMm <= 0.0 || optics.sensorDiagonalMm <= 0.0) return 0.0;

    // Port of estimate_distance_m / sensor_pixel_size_mm in
    // others/distance_estimate:
    //   pitch = sensor_diagonal / frame_diagonal
    //   range = focal * height / (box_px * pitch)
    const double diagonal_px = std::hypot(static_cast<double>(sourceSize.width()),
                                          static_cast<double>(sourceSize.height()));
    if (diagonal_px <= 0.0) return 0.0;

    const double pixel_size_mm = optics.sensorDiagonalMm / diagonal_px;
    if (pixel_size_mm <= 0.0) return 0.0;

    return optics.focalLengthMm * targetHeightMeters / (sourceBox.height() * pixel_size_mm);
}

void offsetFromCentreDeg(const QRect &sourceBox, const QSize &sourceSize,
                         const Optics &optics, double *horizontalDeg, double *verticalDeg)
{
    if (horizontalDeg) *horizontalDeg = 0.0;
    if (verticalDeg) *verticalDeg = 0.0;
    if (sourceSize.width() <= 0 || sourceSize.height() <= 0) return;
    if (optics.fovHorizontalDeg <= 0.0 || optics.fovVerticalDeg <= 0.0) return;

    const double half_w = sourceSize.width() / 2.0;
    const double half_h = sourceSize.height() / 2.0;
    const double focal_x = half_w / std::tan(qDegreesToRadians(optics.fovHorizontalDeg) / 2.0);
    const double focal_y = half_h / std::tan(qDegreesToRadians(optics.fovVerticalDeg) / 2.0);
    if (focal_x <= 0.0 || focal_y <= 0.0) return;

    const double cx = sourceBox.x() + sourceBox.width() / 2.0;
    const double cy = sourceBox.y() + sourceBox.height() / 2.0;

    if (horizontalDeg) *horizontalDeg = qRadiansToDegrees(std::atan((cx - half_w) / focal_x));
    // Image y grows downward; elevation grows upward.
    if (verticalDeg) *verticalDeg = qRadiansToDegrees(std::atan((half_h - cy) / focal_y));
}

Bearing hullToGeographic(double hullAzimuthDeg, double hullElevationDeg,
                         double headingDeg, double pitchDeg, double rollDeg)
{
    const double az = qDegreesToRadians(hullAzimuthDeg);
    const double el = qDegreesToRadians(hullElevationDeg);
    const double p = qDegreesToRadians(pitchDeg);
    const double r = qDegreesToRadians(rollDeg);

    double x = std::cos(el) * std::cos(az);
    double y = std::cos(el) * std::sin(az);
    double z = std::sin(el);

    // Reapply roll, then pitch: the reverse order, and opposite signs, of the
    // geographic-to-hull rotation used for G1.
    const double yr = y * std::cos(r) - z * std::sin(r);
    const double zr = y * std::sin(r) + z * std::cos(r);
    y = yr;
    z = zr;

    const double xp = x * std::cos(p) - z * std::sin(p);
    const double zp = x * std::sin(p) + z * std::cos(p);
    x = xp;
    z = zp;

    Bearing out;
    double azimuth = qRadiansToDegrees(std::atan2(y, x)) + headingDeg;
    azimuth = std::fmod(azimuth, 360.0);
    if (azimuth < 0.0) azimuth += 360.0;
    out.azimuthDeg = azimuth;
    out.elevationDeg = qRadiansToDegrees(std::asin(qBound(-1.0, z, 1.0)));
    return out;
}

} // namespace TargetEstimator
