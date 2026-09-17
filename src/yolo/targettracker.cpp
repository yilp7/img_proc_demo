#include "yolo/targettracker.h"

#include <algorithm>
#include <cmath>

namespace {

double iou(const cv::Rect &a, const cv::Rect &b)
{
    const int inter = (a & b).area();
    if (inter <= 0) return 0.0;
    const int uni = a.area() + b.area() - inter;
    return uni > 0 ? static_cast<double>(inter) / static_cast<double>(uni) : 0.0;
}

double centroidDistance(const cv::Rect &a, const cv::Rect &b)
{
    const double dx = (a.x + a.width / 2.0) - (b.x + b.width / 2.0);
    const double dy = (a.y + a.height / 2.0) - (b.y + b.height / 2.0);
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

const std::vector<TargetTrack>& TargetTracker::update(const std::vector<YoloResult>& detections)
{
    // Tracks dropped in the previous update were already reported; clear the
    // flag before it can be seen twice.
    m_tracks.erase(std::remove_if(m_tracks.begin(), m_tracks.end(),
                                  [](const TargetTrack &t) { return t.justLost; }),
                   m_tracks.end());

    std::vector<bool> detection_used(detections.size(), false);
    std::vector<bool> track_matched(m_tracks.size(), false);

    // Greedy: repeatedly take the best remaining pair. O(n*m) per pick and
    // O(n^2*m) overall, which is irrelevant at YOLO detection counts and much
    // easier to reason about than Hungarian assignment.
    while (true) {
        double best_score = m_iou_threshold;
        int best_track = -1;
        int best_detection = -1;

        for (size_t t = 0; t < m_tracks.size(); ++t) {
            if (track_matched[t]) continue;
            for (size_t d = 0; d < detections.size(); ++d) {
                if (detection_used[d]) continue;
                // Class must agree: a ship re-labelled as a buoy is a new
                // target, not the same one with a different name.
                if (m_tracks[t].classId != detections[d].classId) continue;
                if (centroidDistance(m_tracks[t].box, detections[d].box) > m_max_centroid_distance)
                    continue;
                const double score = iou(m_tracks[t].box, detections[d].box);
                if (score > best_score) {
                    best_score = score;
                    best_track = static_cast<int>(t);
                    best_detection = static_cast<int>(d);
                }
            }
        }

        if (best_track < 0) break;
        track_matched[best_track] = true;
        detection_used[best_detection] = true;

        TargetTrack &track = m_tracks[best_track];
        track.box = detections[best_detection].box;
        track.confidence = detections[best_detection].confidence;
        track.missedFrames = 0;
        ++track.hitStreak;
        if (track.hitStreak >= m_confirm_frames) track.confirmed = true;
    }

    for (size_t t = 0; t < m_tracks.size(); ++t) {
        if (track_matched[t]) continue;
        ++m_tracks[t].missedFrames;
        m_tracks[t].hitStreak = 0;
    }

    for (size_t d = 0; d < detections.size(); ++d) {
        if (detection_used[d]) continue;
        TargetTrack track;
        track.id = m_next_id++;
        track.classId = detections[d].classId;
        track.confidence = detections[d].confidence;
        track.box = detections[d].box;
        track.hitStreak = 1;
        track.confirmed = m_confirm_frames <= 1;
        m_tracks.push_back(track);
    }

    // Expired tracks stay in the vector one more update carrying justLost, so
    // the caller gets exactly one chance to emit a disappearance for them.
    // Only confirmed tracks are worth announcing — an unconfirmed one was
    // never reported as appearing.
    for (TargetTrack &track : m_tracks) {
        if (track.missedFrames > m_max_missed_frames) track.justLost = true;
    }
    m_tracks.erase(std::remove_if(m_tracks.begin(), m_tracks.end(),
                                  [](const TargetTrack &t) {
                                      return t.justLost && !t.confirmed;
                                  }),
                   m_tracks.end());

    return m_tracks;
}

void TargetTracker::reset()
{
    m_tracks.clear();
    // Ids deliberately keep advancing: a target reacquired after a route change
    // is not the same target, and reusing an id would make the platform think
    // an old batch had come back.
}
