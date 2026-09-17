#include "yolo/gatetracker.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

float median(std::vector<float> &values)
{
    if (values.empty()) return 0.0f;
    const size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + mid, values.end());
    return values[mid];
}

// Per-frame scale is clamped hard: a target's apparent size cannot change much
// between frames, and an unclamped estimate from a noisy point set collapses
// the gate to nothing in a few frames.
constexpr float MaxScaleStep = 0.05f;
// Cumulative bounds relative to the designated size, so a slow drift in the
// scale estimate cannot walk the gate away over a long track.
constexpr float MinCumulativeScale = 0.25f;
constexpr float MaxCumulativeScale = 4.0f;

// Correlation floor for accepting a template match, and the higher bar the
// frozen anchor has to clear to take a drifted track back.
constexpr double MatchAcceptScore = 0.45;
constexpr double AnchorRecoverScore = 0.55;
// Only a confident match is allowed to teach the adaptive template, and then
// only slightly; this is the difference between tracking appearance change and
// dissolving into the background.
constexpr double TemplateLearnScore = 0.70;
constexpr double TemplateLearnRate = 0.08;
// Frames the gate may coast on a failed match before the track is abandoned.
constexpr int MaxTemplateMisses = 10;

} // namespace

void GateTracker::setSettings(const Settings &settings)
{
    // The two methods keep entirely different state, so a live track cannot
    // survive the switch.
    if (settings.mode != m_settings.mode) clear();
    m_settings = settings;
}

void GateTracker::clear()
{
    m_previous.release();
    m_frame_size = cv::Size();
    m_points.clear();
    m_template.release();
    m_anchor.release();
    m_box = cv::Rect();
    m_seed_size = cv::Size();
    m_active = false;
    m_score = 0.0;
    m_misses = 0;
}

bool GateTracker::detectFeatures(const cv::Mat &gray, bool core_only)
{
    cv::Rect region = m_box;
    if (core_only) {
        // Mid-track top-ups draw from the middle of the gate only. Recruiting
        // from the edges is how a gate that has drifted slightly onto the
        // background makes that drift permanent.
        const int inset_x = region.width / 6;
        const int inset_y = region.height / 6;
        region.x += inset_x;
        region.y += inset_y;
        region.width -= 2 * inset_x;
        region.height -= 2 * inset_y;
    }

    const cv::Rect bounded = region & cv::Rect(0, 0, gray.cols, gray.rows);
    if (bounded.width < 8 || bounded.height < 8) return false;

    std::vector<cv::Point2f> corners;
    cv::goodFeaturesToTrack(gray(bounded), corners, m_max_features, 0.01, 5.0);
    if (static_cast<int>(corners.size()) < m_min_features) return false;

    // goodFeaturesToTrack works in the sub-image, so lift back to frame coords.
    m_points.clear();
    m_points.reserve(corners.size());
    for (const cv::Point2f &corner : corners)
        m_points.push_back(corner + cv::Point2f(static_cast<float>(bounded.x),
                                                static_cast<float>(bounded.y)));
    return true;
}

void GateTracker::seed(const cv::Mat &gray, const cv::Rect &gate)
{
    clear();
    if (gray.empty() || gate.width < 8 || gate.height < 8) return;

    const cv::Rect bounded = gate & cv::Rect(0, 0, gray.cols, gray.rows);
    if (bounded.width < 8 || bounded.height < 8) return;

    m_box = bounded;
    m_seed_size = bounded.size();

    m_frame_size = gray.size();

    if (m_settings.mode == Mode::Template) {
        gray(bounded).copyTo(m_template);
        m_template.copyTo(m_anchor);
        m_score = 1.0;
    } else {
        if (!detectFeatures(gray, false)) {
            // A gate over flat water or sky has nothing to lock onto. Failing
            // here is better than reporting a track that will immediately
            // wander.
            clear();
            return;
        }
        gray.copyTo(m_previous);
    }

    m_active = true;
}

bool GateTracker::moveBox(float dx, float dy, float scale, const cv::Size &frame)
{
    const float cx = m_box.x + m_box.width * 0.5f + dx;
    const float cy = m_box.y + m_box.height * 0.5f + dy;

    scale = std::min(std::max(scale, 1.0f - MaxScaleStep), 1.0f + MaxScaleStep);
    float width = m_box.width * scale;
    float height = m_box.height * scale;

    if (m_seed_size.width > 0 && m_seed_size.height > 0) {
        width = std::min(std::max(width, MinCumulativeScale * m_seed_size.width),
                         MaxCumulativeScale * m_seed_size.width);
        height = std::min(std::max(height, MinCumulativeScale * m_seed_size.height),
                          MaxCumulativeScale * m_seed_size.height);
    }

    m_box = cv::Rect(static_cast<int>(std::lround(cx - width * 0.5f)),
                     static_cast<int>(std::lround(cy - height * 0.5f)),
                     std::max(8, static_cast<int>(std::lround(width))),
                     std::max(8, static_cast<int>(std::lround(height))));

    // Mostly off-frame: the turret has lost it rather than the tracker.
    return (m_box & cv::Rect(0, 0, frame.width, frame.height)).area() >= m_box.area() / 4;
}

bool GateTracker::update(const cv::Mat &gray)
{
    if (!m_active || gray.empty()) return false;
    if (m_frame_size.area() > 0 && gray.size() != m_frame_size) {
        // Source resolution changed under us; the gate no longer means anything.
        clear();
        return false;
    }

    return m_settings.mode == Mode::Template ? updateTemplate(gray) : updateOpticalFlow(gray);
}

bool GateTracker::updateOpticalFlow(const cv::Mat &gray)
{
    if (m_previous.empty() || m_points.empty()) return false;

    std::vector<cv::Point2f> next;
    std::vector<uchar> status;
    std::vector<float> err;
    cv::calcOpticalFlowPyrLK(m_previous, gray, m_points, next, status, err,
                             cv::Size(21, 21), 3);

    // Forward-backward validation. Tracking the results back to the previous
    // frame and demanding they land where they started rejects bad points
    // individually, which the median-agreement filter below cannot do once the
    // majority has already drifted.
    std::vector<float> fb_error;
    float fb_tolerance = 0.0f;
    if (m_settings.forward_backward_check) {
        std::vector<cv::Point2f> back;
        std::vector<uchar> back_status;
        std::vector<float> back_err;
        cv::calcOpticalFlowPyrLK(gray, m_previous, next, back, back_status, back_err,
                                 cv::Size(21, 21), 3);

        fb_error.assign(next.size(), std::numeric_limits<float>::max());
        std::vector<float> valid;
        for (size_t i = 0; i < next.size(); ++i) {
            if (!status[i] || i >= back_status.size() || !back_status[i]) continue;
            const float ex = back[i].x - m_points[i].x;
            const float ey = back[i].y - m_points[i].y;
            fb_error[i] = std::sqrt(ex * ex + ey * ey);
            valid.push_back(fb_error[i]);
        }
        // Median keeps the better half when the whole set is degrading, while
        // the floor avoids rejecting points that are all simply good.
        fb_tolerance = std::max(1.5f, median(valid));
    }

    std::vector<cv::Point2f> kept_prev;
    std::vector<cv::Point2f> kept_next;
    std::vector<float> dx;
    std::vector<float> dy;
    for (size_t i = 0; i < next.size(); ++i) {
        if (!status[i]) continue;
        if (next[i].x < 0 || next[i].y < 0 || next[i].x >= gray.cols || next[i].y >= gray.rows)
            continue;
        if (!fb_error.empty() && fb_error[i] > fb_tolerance) continue;
        kept_prev.push_back(m_points[i]);
        kept_next.push_back(next[i]);
        dx.push_back(next[i].x - m_points[i].x);
        dy.push_back(next[i].y - m_points[i].y);
    }

    if (static_cast<int>(kept_next.size()) < m_min_features) {
        // Too few survivors to trust a median. One re-seed attempt on the
        // current frame before giving up — a brief occlusion or a glint can
        // kill the old features while the target is still there.
        gray.copyTo(m_previous);
        if (!detectFeatures(gray, true)) {
            clear();
            return false;
        }
        return true;
    }

    const float median_dx = median(dx);
    const float median_dy = median(dy);

    // Median is already robust, but points that disagree strongly with it are
    // on the background or on a passing object, and keeping them would let the
    // gate drift off the target over time.
    std::vector<cv::Point2f> filtered_prev;
    std::vector<cv::Point2f> filtered;
    const float tolerance = std::max(2.0f, 0.25f * std::max(std::fabs(median_dx),
                                                            std::fabs(median_dy)) + 2.0f);
    for (size_t i = 0; i < kept_next.size(); ++i) {
        const float ex = (kept_next[i].x - kept_prev[i].x) - median_dx;
        const float ey = (kept_next[i].y - kept_prev[i].y) - median_dy;
        if (std::sqrt(ex * ex + ey * ey) <= tolerance) {
            filtered_prev.push_back(kept_prev[i]);
            filtered.push_back(kept_next[i]);
        }
    }
    if (filtered.size() < static_cast<size_t>(m_min_features)) {
        filtered_prev = kept_prev;
        filtered = kept_next;
    }

    // Scale from the median ratio of pairwise point separations. Translation
    // cancels out, so this measures the target growing or shrinking rather than
    // moving — without it the gate keeps its designated size and eventually
    // contains as much background as target.
    float scale = 1.0f;
    if (filtered.size() >= 2) {
        std::vector<float> ratios;
        ratios.reserve(filtered.size() * (filtered.size() - 1) / 2);
        for (size_t i = 0; i < filtered.size(); ++i) {
            for (size_t j = i + 1; j < filtered.size(); ++j) {
                const float prev_d = static_cast<float>(cv::norm(filtered_prev[i] - filtered_prev[j]));
                if (prev_d < 1.0f) continue;
                const float next_d = static_cast<float>(cv::norm(filtered[i] - filtered[j]));
                ratios.push_back(next_d / prev_d);
            }
        }
        if (!ratios.empty()) scale = median(ratios);
    }

    m_points = filtered;
    if (!moveBox(median_dx, median_dy, scale, gray.size())) {
        clear();
        return false;
    }

    gray.copyTo(m_previous);

    // Feature count decays as points fail; top back up before it reaches the
    // floor, so a long track does not slowly starve itself.
    if (static_cast<int>(m_points.size()) < m_max_features / 3) detectFeatures(gray, true);
    return true;
}

bool GateTracker::updateTemplate(const cv::Mat &gray)
{
    if (m_template.empty()) {
        clear();
        return false;
    }

    // Search a window centred on the last known position, sized so a target can
    // cross half the gate between frames without escaping it.
    const int margin_x = std::max(16, m_box.width / 2);
    const int margin_y = std::max(16, m_box.height / 2);
    cv::Rect search(m_box.x - margin_x, m_box.y - margin_y,
                    m_box.width + 2 * margin_x, m_box.height + 2 * margin_y);
    search &= cv::Rect(0, 0, gray.cols, gray.rows);
    if (search.width < m_template.cols || search.height < m_template.rows) {
        clear();
        return false;
    }

    const cv::Mat window = gray(search);

    std::vector<float> scales{1.0f};
    if (m_settings.multi_scale) {
        scales.push_back(1.0f - MaxScaleStep);
        scales.push_back(1.0f + MaxScaleStep);
    }

    const auto correlate = [&](const cv::Mat &tmpl, float scale,
                               double *score, cv::Point *location, cv::Size *size) {
        cv::Size target(static_cast<int>(std::lround(tmpl.cols * scale)),
                        static_cast<int>(std::lround(tmpl.rows * scale)));
        if (target.width < 8 || target.height < 8) return false;
        if (target.width > window.cols || target.height > window.rows) return false;

        cv::Mat scaled;
        if (scale == 1.0f) scaled = tmpl;
        else cv::resize(tmpl, scaled, target, 0, 0, cv::INTER_LINEAR);

        cv::Mat response;
        cv::matchTemplate(window, scaled, response, cv::TM_CCOEFF_NORMED);
        double max_value = 0.0;
        cv::Point max_location;
        cv::minMaxLoc(response, nullptr, &max_value, nullptr, &max_location);

        *score = max_value;
        *location = max_location;
        *size = target;
        return true;
    };

    double best_score = -1.0;
    cv::Point best_location;
    cv::Size best_size;
    for (float scale : scales) {
        double score = 0.0;
        cv::Point location;
        cv::Size size;
        if (!correlate(m_template, scale, &score, &location, &size)) continue;
        if (score > best_score) {
            best_score = score;
            best_location = location;
            best_size = size;
        }
    }

    // The adaptive patch has lost it. The designation patch is the only thing
    // left that is guaranteed not to have drifted, so try to recover with it
    // before writing the track off.
    bool recovered = false;
    if (best_score < MatchAcceptScore && !m_anchor.empty()) {
        cv::Mat anchor = m_anchor;
        if (anchor.cols != m_box.width || anchor.rows != m_box.height) {
            const cv::Size target(std::max(8, m_box.width), std::max(8, m_box.height));
            if (target.width <= window.cols && target.height <= window.rows)
                cv::resize(m_anchor, anchor, target, 0, 0, cv::INTER_LINEAR);
        }
        double score = 0.0;
        cv::Point location;
        cv::Size size;
        if (correlate(anchor, 1.0f, &score, &location, &size) && score >= AnchorRecoverScore) {
            best_score = score;
            best_location = location;
            best_size = size;
            recovered = true;
        }
    }

    m_score = best_score;

    if (best_score < MatchAcceptScore && !recovered) {
        // Coast on the last position: a target crossing behind a mast should
        // not cost the operator their designation.
        if (++m_misses > MaxTemplateMisses) {
            clear();
            return false;
        }
        return true;
    }

    m_misses = 0;

    const cv::Rect matched(search.x + best_location.x, search.y + best_location.y,
                           best_size.width, best_size.height);
    const float dx = (matched.x + matched.width * 0.5f) - (m_box.x + m_box.width * 0.5f);
    const float dy = (matched.y + matched.height * 0.5f) - (m_box.y + m_box.height * 0.5f);
    const float scale = m_box.width > 0 ? static_cast<float>(best_size.width) / m_box.width : 1.0f;

    if (!moveBox(dx, dy, scale, gray.size())) {
        clear();
        return false;
    }

    // Only a gate wholly inside the frame yields a patch that still means the
    // designated region; a clipped one would silently shrink the template and
    // corrupt the scale ratio on the next frame.
    const cv::Rect frame(0, 0, gray.cols, gray.rows);
    if ((m_box & frame) == m_box && m_box.width >= 8 && m_box.height >= 8) {
        // Re-extract rather than blend when the size changed, since a blend
        // needs matching dimensions.
        if (recovered || m_box.size() != m_template.size())
            gray(m_box).copyTo(m_template);
        else if (best_score >= TemplateLearnScore)
            cv::addWeighted(m_template, 1.0 - TemplateLearnRate,
                            gray(m_box), TemplateLearnRate, 0.0, m_template);
    }

    // The scale ratio above reads the template size against the box size, so
    // the two must stay equal. They diverge only when moveBox clamps or the
    // patch could not be taken, which is rare enough to pay a resample for.
    if (m_box.size() != m_template.size() && m_box.width >= 8 && m_box.height >= 8)
        cv::resize(m_template, m_template, m_box.size(), 0, 0, cv::INTER_LINEAR);

    return true;
}
