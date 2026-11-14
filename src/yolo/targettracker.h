#ifndef TARGETTRACKER_H
#define TARGETTRACKER_H

#include <opencv2/core.hpp>

#include <vector>

#include "yolo/yolo_detector.h"

// A detection that has survived long enough to be treated as a real target and
// given a stable id. Ids are local to the tracker; the 709 batch number is
// derived from them at report time.
struct TargetTrack {
    int id = -1;
    int class_id = -1;
    float confidence = 0.0f;
    cv::Rect box;
    // Frames since this track was last matched to a detection. 0 means it was
    // seen in the current frame; anything higher means it is coasting.
    int missed_frames = 0;
    // Consecutive matches so far; a track is only reported once this reaches
    // the confirmation threshold.
    int hit_streak = 0;
    bool confirmed = false;
    // Set for exactly one update, the one in which the track was dropped, so
    // a single disappearance report can be emitted for it.
    bool just_lost = false;
};

// Greedy IOU tracker with a centroid distance gate. No motion model on purpose:
// boxes on a slewing turret jump too far for a constant-velocity predictor, and a
// Kalman filter would need tuning on real footage first.
class TargetTracker
{
public:
    void set_iou_threshold(double value) { m_iou_threshold = value; }
    void set_max_centroid_distance(double pixels) { m_max_centroid_distance = pixels; }
    void set_confirm_frames(int frames) { m_confirm_frames = frames; }
    void set_max_missed_frames(int frames) { m_max_missed_frames = frames; }

    // Feeds one frame of detections and returns the live tracks, including any
    // dropped this frame (flagged just_lost) so the caller can report them once.
    const std::vector<TargetTrack> &update(const std::vector<YoloResult> &detections);

    const std::vector<TargetTrack> &get_tracks() const { return m_tracks; }
    void reset();

private:
    std::vector<TargetTrack> m_tracks;
    int m_next_id = 1;

    double m_iou_threshold = 0.3;
    double m_max_centroid_distance = 200.0;
    int m_confirm_frames = 3;
    int m_max_missed_frames = 5;
};

#endif // TARGETTRACKER_H
