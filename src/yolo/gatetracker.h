#ifndef GATETRACKER_H
#define GATETRACKER_H

#include <opencv2/core.hpp>

#include <vector>

// Operator-designated region tracker, for closed-loop follow when no detector
// is running. Two exclusive methods, chosen in the setup panel:
//
//   OpticalFlow  Lucas-Kanade on corner features inside the gate. Smooth and
//                cheap, but needs texture: a target on flat water or open sky
//                may not yield enough corners to seed at all.
//   Template     Normalized cross-correlation of the designated patch over a
//                search window. Indifferent to texture and yields an absolute
//                match score, so loss is detected directly rather than inferred
//                from surviving feature count.
//
// Both estimate scale, so the gate follows a target that grows or shrinks.
class GateTracker
{
public:
    enum class Mode { OpticalFlow, Template };

    struct Settings {
        Mode mode = Mode::OpticalFlow;
        // Optical flow: validates each point by tracking it back to the previous
        // frame and rejecting those that do not return to their origin. Roughly
        // doubles the flow cost.
        bool forward_backward_check = true;
        // Template: also correlates at +/-5% size, so the gate tracks scale at
        // about three times the search cost.
        bool multi_scale = true;
    };

    void setSettings(const Settings &settings);
    const Settings& settings() const { return m_settings; }

    void seed(const cv::Mat &gray, const cv::Rect &gate);
    // Returns false once the gate can no longer be trusted; box() is then
    // meaningless and the caller should disengage.
    bool update(const cv::Mat &gray);
    void clear();

    bool active() const { return m_active; }
    cv::Rect box() const { return m_box; }
    int featureCount() const { return static_cast<int>(m_points.size()); }
    // Last correlation peak in template mode, 0 in optical-flow mode.
    double matchScore() const { return m_score; }

    void setMinFeatures(int value) { m_min_features = value; }
    void setMaxFeatures(int value) { m_max_features = value; }

private:
    bool detectFeatures(const cv::Mat &gray, bool core_only);
    bool updateOpticalFlow(const cv::Mat &gray);
    bool updateTemplate(const cv::Mat &gray);
    // Applies a translation and a scale about the gate centre, and reports
    // whether the result is still usable.
    bool moveBox(float dx, float dy, float scale, const cv::Size &frame);

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
