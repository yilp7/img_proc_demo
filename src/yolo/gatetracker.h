#ifndef GATETRACKER_H
#define GATETRACKER_H

#include <opencv2/core.hpp>

#include <vector>

// Tracker of an operator-designated region, for closed-loop follow without a
// detector. OPTICAL_FLOW follows corner features and needs texture; TEMPLATE
// correlates the designated patch and reports a match score. Both estimate scale.
class GateTracker
{
public:
    enum class Mode { OPTICAL_FLOW, TEMPLATE };

    struct Settings {
        Mode mode = Mode::OPTICAL_FLOW;
        // Optical flow: validates each point by tracking it back to the previous
        // frame and rejecting those that do not return to their origin. Roughly
        // doubles the flow cost.
        bool forward_backward_check = true;
        // Template: also correlates at +/-5% size, so the gate tracks scale at
        // about three times the search cost.
        bool multi_scale = true;
    };

    void set_settings(const Settings &settings);
    const Settings &get_settings() const { return m_settings; }

    void seed(const cv::Mat &gray, const cv::Rect &gate);
    // Returns false once the gate can no longer be trusted; get_box() is then
    // meaningless and the caller should disengage.
    bool update(const cv::Mat &gray);
    void clear();

    bool is_active() const { return m_active; }
    cv::Rect get_box() const { return m_box; }
    int get_feature_count() const { return static_cast<int>(m_points.size()); }
    // Last correlation peak in template mode, 0 in optical-flow mode.
    double get_match_score() const { return m_score; }

    void set_min_features(int value) { m_min_features = value; }
    void set_max_features(int value) { m_max_features = value; }

private:
    bool detect_features(const cv::Mat &gray, bool core_only);
    bool update_optical_flow(const cv::Mat &gray);
    bool update_template(const cv::Mat &gray);
    // Applies a translation and a scale about the gate centre, and reports
    // whether the result is still usable.
    bool move_box(float dx, float dy, float scale, const cv::Size &frame);

    Settings m_settings;

    cv::Mat m_previous;                  // optical flow only
    cv::Size m_frame_size;
    std::vector<cv::Point2f> m_points;
    cv::Rect m_box;
    cv::Size m_seed_size;
    bool m_active = false;
    double m_score = 0.0;

    // Template mode. The adaptive patch absorbs slow appearance change; the
    // anchor is the designation patch, kept untouched so a track that has
    // drifted or been occluded has something honest to recover against.
    cv::Mat m_template;
    cv::Mat m_anchor;
    int m_misses = 0;

    int m_min_features = 6;
    int m_max_features = 60;
};

#endif // GATETRACKER_H
