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
    int classId = -1;
    float confidence = 0.0f;
    cv::Rect box;
    // Frames since this track was last matched to a detection. 0 means it was
    // seen in the current frame; anything higher means it is coasting.
    int missedFrames = 0;
    // Consecutive matches so far; a track is only reported once this reaches
    // the confirmation threshold.
    int hitStreak = 0;
    bool confirmed = false;
    // Set for exactly one update, the one in which the track was dropped, so
    // a single disappearance report can be emitted for it.
    bool justLost = false;
};

// Greedy IOU tracker. Deliberately no motion model: YOLO boxes on a slewing
// turret jump too far between frames for a constant-velocity predictor to help,
// and a Kalman filter would need tuning against real footage before it earned
// its place. Association is IOU with a centroid distance gate.
class TargetTracker
{
public:
    void setIouThreshold(double value) { m_iou_threshold = value; }
    void setMaxCentroidDistance(double pixels) { m_max_centroid_distance = pixels; }
    void setConfirmFrames(int frames) { m_confirm_frames = frames; }
    void setMaxMissedFrames(int frames) { m_max_missed_frames = frames; }

    // Feeds one frame of detections and returns the live tracks, including any
    // dropped this frame (flagged justLost) so the caller can report them once.
    const std::vector<TargetTrack>& update(const std::vector<YoloResult>& detections);

    const std::vector<TargetTrack>& tracks() const { return m_tracks; }
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
