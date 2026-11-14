#include "util/targetestimator.h"

#include <QtMath>

#include <cmath>

namespace TargetEstimator {

double range_meters(double target_height_meters, const QRect &source_box,
                    const QSize &source_size, const Optics &optics)
{
    if (target_height_meters <= 0.0) return 0.0;
    if (source_box.height() <= 0) return 0.0;
    if (optics.focal_length_mm <= 0.0 || optics.sensor_diagonal_mm <= 0.0) return 0.0;

    // Same as estimate_distance_m in others/distance_estimate:
    //   pitch = sensor_diagonal / frame_diagonal
    //   range = focal * height / (box_px * pitch)
    const double diagonal_px = std::hypot(static_cast<double>(source_size.width()),
                                          static_cast<double>(source_size.height()));
    if (diagonal_px <= 0.0) return 0.0;

    const double pixel_size_mm = optics.sensor_diagonal_mm / diagonal_px;
    if (pixel_size_mm <= 0.0) return 0.0;

    return optics.focal_length_mm * target_height_meters / (source_box.height() * pixel_size_mm);
}

void offset_from_centre_deg(const QRect &source_box, const QSize &source_size,
                            const Optics &optics, double *horizontal_deg, double *vertical_deg)
{
    if (horizontal_deg) *horizontal_deg = 0.0;
    if (vertical_deg) *vertical_deg = 0.0;
    if (source_size.width() <= 0 || source_size.height() <= 0) return;
    if (optics.fov_horizontal_deg <= 0.0 || optics.fov_vertical_deg <= 0.0) return;

    const double half_w = source_size.width() / 2.0;
    const double half_h = source_size.height() / 2.0;
    const double focal_x = half_w / std::tan(qDegreesToRadians(optics.fov_horizontal_deg) / 2.0);
    const double focal_y = half_h / std::tan(qDegreesToRadians(optics.fov_vertical_deg) / 2.0);
    if (focal_x <= 0.0 || focal_y <= 0.0) return;

    const double cx = source_box.x() + source_box.width() / 2.0;
    const double cy = source_box.y() + source_box.height() / 2.0;

    if (horizontal_deg) *horizontal_deg = qRadiansToDegrees(std::atan((cx - half_w) / focal_x));
    // Image y grows downward; elevation grows upward.
    if (vertical_deg) *vertical_deg = qRadiansToDegrees(std::atan((half_h - cy) / focal_y));
}

Bearing hull_to_geographic(double hull_azimuth_deg, double hull_elevation_deg,
                           double heading_deg, double pitch_deg, double roll_deg)
{
    const double az = qDegreesToRadians(hull_azimuth_deg);
    const double el = qDegreesToRadians(hull_elevation_deg);
    const double p = qDegreesToRadians(pitch_deg);
    const double r = qDegreesToRadians(roll_deg);

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
    double azimuth = qRadiansToDegrees(std::atan2(y, x)) + heading_deg;
    azimuth = std::fmod(azimuth, 360.0);
    if (azimuth < 0.0) azimuth += 360.0;
    out.azimuth_deg = azimuth;
    out.elevation_deg = qRadiansToDegrees(std::asin(qBound(-1.0, z, 1.0)));
    return out;
}

} // namespace TargetEstimator
