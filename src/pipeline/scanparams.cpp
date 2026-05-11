#include "scanparams.h"

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

std::vector<float> get_tcu_route(const ScanParams &p)
{
    std::vector<float> delay_value;
    for (int k = 0; k < p.frame_count; ++k) {
        delay_value.push_back(p.starting_delay + k * p.step_size_delay);
    }
    return delay_value;
}
