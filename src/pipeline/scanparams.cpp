#include "scanparams.h"

#include <algorithm>
#include <cmath>

namespace {

const int kMaxPanoramaStops = 2000;
const double kPi = 3.14159265358979323846;

struct PanoramaCalibrationPoint {
    double zoom;
    double focal_length_mm;
    double sensor_w_mm;
    double sensor_h_mm;
};

float normalize_angle(float value)
{
    float angle = std::fmod(value, 360.0f);
    if (angle < 0.0f) angle += 360.0f;
    if (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

int clamp_count(int count)
{
    if (count < 1) return 1;
    if (count > kMaxPanoramaStops) return kMaxPanoramaStops;
    return count;
}

float fallback_step(const ScanParams &p)
{
    float step = std::fabs(p.panorama_step_h);
    if (step < 1e-4f) step = std::fabs(p.panorama_fov_h);
    if (step < 1e-4f) step = 360.0f;
    return step;
}

int panorama_count_from_params(const ScanParams &p)
{
    if (p.panorama_count > 0) return clamp_count(p.panorama_count);
    return get_panorama_stop_count(p);
}

double degrees_to_radians(double value)
{
    return value * kPi / 180.0;
}

float radians_to_degrees(double value)
{
    return static_cast<float>(value * 180.0 / kPi);
}

double sensor_size_from_fov(double focal_length_mm, double fov_degrees)
{
    return 2.0 * focal_length_mm * std::tan(degrees_to_radians(fov_degrees) * 0.5);
}

float fov_from_sensor_size(double sensor_size_mm, double focal_length_mm)
{
    if (sensor_size_mm <= 0.0 || focal_length_mm <= 0.0) return 0.0f;
    return radians_to_degrees(2.0 * std::atan(sensor_size_mm / (2.0 * focal_length_mm)));
}

double calibration_value(const PanoramaCalibrationPoint &pt, int field)
{
    switch (field) {
    case 0: return pt.focal_length_mm;
    case 1: return pt.sensor_w_mm;
    case 2: return pt.sensor_h_mm;
    default: return 0.0;
    }
}

double interpolate_log_value(double zoom, double z0, double v0, double z1, double v1)
{
    if (z0 <= 0.0 || z1 <= 0.0 || v0 <= 0.0 || v1 <= 0.0) return v0;

    double t = 0.0;
    const double lz0 = std::log(z0);
    const double lz1 = std::log(z1);
    if (std::fabs(lz1 - lz0) > 1e-9) t = (std::log(zoom) - lz0) / (lz1 - lz0);

    return std::exp(std::log(v0) + t * (std::log(v1) - std::log(v0)));
}

double interpolate_calibration_value(const std::vector<PanoramaCalibrationPoint> &points,
                                     double zoom, int field)
{
    if (points.empty()) return 0.0;

    if (points.size() == 1) {
        const double value = calibration_value(points[0], field);
        if (field == 0 && points[0].zoom > 0.0) return value * zoom / points[0].zoom;
        return value;
    }

    size_t lo_idx = 0;
    size_t hi_idx = 1;
    if (zoom <= points.front().zoom) {
        lo_idx = 0;
        hi_idx = 1;
    } else if (zoom >= points.back().zoom) {
        lo_idx = points.size() - 2;
        hi_idx = points.size() - 1;
    } else {
        for (size_t i = 1; i < points.size(); ++i) {
            if (zoom <= points[i].zoom) {
                lo_idx = i - 1;
                hi_idx = i;
                break;
            }
        }
    }

    const PanoramaCalibrationPoint &lo = points[lo_idx];
    const PanoramaCalibrationPoint &hi = points[hi_idx];
    return interpolate_log_value(zoom, lo.zoom, calibration_value(lo, field),
                                 hi.zoom, calibration_value(hi, field));
}

} // namespace

std::vector<std::pair<float, float>> get_ptz_route(const ScanParams &p)
{
    std::vector<std::pair<float, float>> route;

    if (p.ptz_direction) {
        for (int j = 0; j < p.count_v; ++j) {
            for (int i = 0; i < p.count_h; ++i) {
                route.push_back(std::make_pair(
                    (j % 2 ? p.ending_h - i * p.step_h : p.starting_h + i * p.step_h),
                    p.starting_v + j * p.step_v));
            }
        }
    }
    else {
        for (int i = 0; i < p.count_h; ++i) {
            for (int j = 0; j < p.count_v; ++j) {
                route.push_back(std::make_pair(
                    p.starting_h + i * p.step_h,
                    (i % 2 ? p.ending_v - j * p.step_v : p.starting_v + j * p.step_v)));
            }
        }
    }
    return route;
}

std::vector<std::pair<float, float>> get_panorama_route(const ScanParams &p)
{
    std::vector<std::pair<float, float>> route;

    const int count = panorama_count_from_params(p);
    if (count <= 0) return route;

    const float sign = p.panorama_direction ? -1.0f : 1.0f;
    route.reserve(static_cast<size_t>(count));

    if (p.panorama_use_end_h) {
        const float sweep = get_panorama_sweep_degrees(p);
        const int segments = count - 1;
        for (int i = 0; i < count; ++i) {
            const float offset = segments > 0 ? sweep * static_cast<float>(i) / segments : 0.0f;
            route.push_back(std::make_pair(
                normalize_angle(p.panorama_start_h + sign * offset),
                p.panorama_fixed_v));
        }
        return route;
    }

    const float step = 360.0f / count;
    for (int i = 0; i < count; ++i) {
        route.push_back(std::make_pair(
            normalize_angle(p.panorama_start_h + sign * i * step),
            p.panorama_fixed_v));
    }

    return route;
}

std::vector<float> get_tcu_route(const ScanParams &p)
{
    std::vector<float> delay_value;
    for (int k = 0; k < p.frame_count; ++k) {
        delay_value.push_back(p.starting_delay + k * p.step_size_delay);
    }
    return delay_value;
}

std::pair<float, float> get_panorama_fov(float zoom, const std::vector<PanoramaZoomFovPoint> &points)
{
    std::vector<PanoramaCalibrationPoint> valid;
    valid.reserve(points.size());
    for (const PanoramaZoomFovPoint &pt : points) {
        if (pt.zoom <= 0.0f || pt.fov_h <= 0.0f || pt.fov_v <= 0.0f) continue;
        const double focal = pt.focal_length_mm > 0.0f
            ? static_cast<double>(pt.focal_length_mm)
            : static_cast<double>(pt.zoom) * 10.0;
        valid.push_back({
            static_cast<double>(pt.zoom),
            focal,
            sensor_size_from_fov(focal, pt.fov_h),
            sensor_size_from_fov(focal, pt.fov_v)
        });
    }

    if (valid.empty()) return std::make_pair(38.4f, 22.5f);

    std::sort(valid.begin(), valid.end(),
              [](const PanoramaCalibrationPoint &a, const PanoramaCalibrationPoint &b) {
        return a.zoom < b.zoom;
    });

    if (zoom <= 0.0f) zoom = 1.0f;
    const double z = static_cast<double>(zoom);
    const double focal = interpolate_calibration_value(valid, z, 0);
    const double sensor_w = interpolate_calibration_value(valid, z, 1);
    const double sensor_h = interpolate_calibration_value(valid, z, 2);
    return std::make_pair(fov_from_sensor_size(sensor_w, focal),
                          fov_from_sensor_size(sensor_h, focal));
}

void update_panorama_fov_from_zoom(ScanParams &p)
{
    if (p.panorama_zoom_level <= 0.0f) p.panorama_zoom_level = 1.0f;
    const std::pair<float, float> fov = get_panorama_fov(p.panorama_zoom_level, p.panorama_zoom_fov);
    p.panorama_fov_h = fov.first;
    p.panorama_fov_v = fov.second;
}

float get_panorama_sweep_degrees(const ScanParams &p)
{
    if (!p.panorama_use_end_h) return 360.0f;

    const float start = normalize_angle(p.panorama_start_h);
    const float end = normalize_angle(p.panorama_end_h);
    if (p.panorama_direction) return normalize_angle(start - end);
    return normalize_angle(end - start);
}

int get_panorama_stop_count(const ScanParams &p)
{
    const float sweep = get_panorama_sweep_degrees(p);
    if (p.panorama_use_end_h && sweep < 1e-4f) return 1;

    const float step = fallback_step(p);
    int count = static_cast<int>(std::ceil(sweep / step));
    if (p.panorama_use_end_h) count += 1;
    return clamp_count(count);
}

const char* get_panorama_fusion_name(int mode)
{
    switch (mode) {
    case ScanParams::PANORAMA_FUSION_NONE: return "none";
    case ScanParams::PANORAMA_FUSION_GAIN: return "gain";
    case ScanParams::PANORAMA_FUSION_MATCHING: return "matching";
    default: return "gain";
    }
}
