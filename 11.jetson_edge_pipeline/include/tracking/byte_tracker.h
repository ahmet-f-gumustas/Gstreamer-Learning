#ifndef JETSON_EDGE_BYTE_TRACKER_H
#define JETSON_EDGE_BYTE_TRACKER_H

#include "inference/tensorrt_engine.h"  // for Detection
#include "tracking/kalman_filter.h"

#include <vector>
#include <memory>

namespace edge {

struct ByteTrackConfig {
    float track_high_thresh = 0.6f;  // first association
    float track_low_thresh  = 0.1f;  // second association (low-conf dets)
    float new_track_thresh  = 0.7f;  // confidence to start a new track
    float match_thresh      = 0.8f;  // IoU distance threshold
    int   track_buffer      = 30;    // frames before a lost track is removed
    int   frame_rate        = 30;
};

enum class TrackState { NEW, TRACKED, LOST, REMOVED };

struct STrack {
    int          track_id   = -1;
    TrackState   state      = TrackState::NEW;
    int          class_id   = 0;
    float        score      = 0.f;

    // Bounding box (tlwh)
    float        x = 0, y = 0, w = 0, h = 0;

    // Kalman state
    KalmanFilter kf;
    bool         kf_initialized = false;

    // Frame book-keeping
    int          start_frame      = 0;
    int          frame_id         = 0;
    int          tracklet_len     = 0;
    int          time_since_update = 0;

    cv::Vec4f tlwh_to_xyah() const {
        return { x + w * 0.5f, y + h * 0.5f, w / h, h };
    }
    void xyah_to_tlwh(const cv::Vec4f& xyah) {
        h = xyah[3];
        w = xyah[2] * h;
        x = xyah[0] - w * 0.5f;
        y = xyah[1] - h * 0.5f;
    }
};

// Pure-C++ ByteTrack (https://arxiv.org/abs/2110.06864) — no DeepStream dep.
class ByteTracker {
public:
    explicit ByteTracker(const ByteTrackConfig& cfg = {});

    // Feed one frame's detections, get tracks back with track_id filled in.
    std::vector<Detection> update(const std::vector<Detection>& dets);

    int currentFrame() const { return frame_id_; }
    int activeTracks() const { return static_cast<int>(tracked_.size()); }

private:
    ByteTrackConfig cfg_;
    int frame_id_   = 0;
    int next_id_    = 1;

    std::vector<STrack> tracked_;   // currently confirmed
    std::vector<STrack> lost_;      // recently lost, awaiting comeback
    std::vector<STrack> removed_;   // permanently dead

    // IoU distance matrix used by association
    static float iouDistance(const STrack& a, const STrack& b);
    static std::vector<float> buildIouCost(const std::vector<STrack*>& a,
                                            const std::vector<STrack*>& b);
};

}  // namespace edge

#endif
