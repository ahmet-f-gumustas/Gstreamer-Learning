#include "tracking/byte_tracker.h"
#include "tracking/hungarian.h"

#include <algorithm>
#include <iostream>

namespace edge {

ByteTracker::ByteTracker(const ByteTrackConfig& cfg) : cfg_(cfg) {}

// ─────────────────────────────────────────────────────────────────────────────
float ByteTracker::iouDistance(const STrack& a, const STrack& b) {
    float x1 = std::max(a.x, b.x);
    float y1 = std::max(a.y, b.y);
    float x2 = std::min(a.x + a.w, b.x + b.w);
    float y2 = std::min(a.y + a.h, b.y + b.h);
    float w  = std::max(0.f, x2 - x1);
    float h  = std::max(0.f, y2 - y1);
    float inter = w * h;
    float uni   = a.w * a.h + b.w * b.h - inter;
    float iou   = uni > 0 ? inter / uni : 0.f;
    return 1.f - iou;          // distance form
}

std::vector<float> ByteTracker::buildIouCost(const std::vector<STrack*>& A,
                                             const std::vector<STrack*>& B) {
    std::vector<float> c(A.size() * B.size());
    for (size_t i = 0; i < A.size(); ++i)
        for (size_t j = 0; j < B.size(); ++j)
            c[i * B.size() + j] = iouDistance(*A[i], *B[j]);
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
std::vector<Detection> ByteTracker::update(const std::vector<Detection>& dets) {
    ++frame_id_;

    // Split detections by confidence.
    std::vector<Detection> high_dets, low_dets;
    for (auto& d : dets) {
        if (d.confidence >= cfg_.track_high_thresh)      high_dets.push_back(d);
        else if (d.confidence >= cfg_.track_low_thresh)  low_dets.push_back(d);
    }

    // ── 1) Predict every existing track ──────────────────────────────────────
    auto predict_all = [](std::vector<STrack>& tracks) {
        for (auto& t : tracks) {
            if (!t.kf_initialized) continue;
            cv::Vec4f xyah = t.kf.predict();
            t.xyah_to_tlwh(xyah);
        }
    };
    predict_all(tracked_);
    predict_all(lost_);

    // Pool: tracked + lost  --> first-stage candidates
    std::vector<STrack*> pool;
    for (auto& t : tracked_) pool.push_back(&t);
    for (auto& t : lost_)    pool.push_back(&t);

    // Convert high detections to STrack stubs
    auto detToTrack = [&](const Detection& d) {
        STrack s;
        s.x = d.x; s.y = d.y; s.w = d.w; s.h = d.h;
        s.score = d.confidence;
        s.class_id = d.class_id;
        return s;
    };

    std::vector<STrack> hi_st;
    hi_st.reserve(high_dets.size());
    for (auto& d : high_dets) hi_st.push_back(detToTrack(d));
    std::vector<STrack*> hi_ptr;
    for (auto& s : hi_st) hi_ptr.push_back(&s);

    // ── 2) First association: tracked+lost vs. high detections ───────────────
    auto cost1 = buildIouCost(pool, hi_ptr);
    for (auto& v : cost1) if (v > cfg_.match_thresh) v = Hungarian::INF_COST;
    auto assign1 = Hungarian::solve(cost1, pool.size(), hi_ptr.size());

    std::vector<bool> hi_matched(hi_ptr.size(), false);
    std::vector<bool> pool_matched(pool.size(), false);

    for (size_t i = 0; i < assign1.size(); ++i) {
        int j = assign1[i];
        if (j < 0) continue;
        STrack* track = pool[i];
        STrack& det   = hi_st[j];

        track->x = det.x; track->y = det.y;
        track->w = det.w; track->h = det.h;
        track->score = det.score;
        track->class_id = det.class_id;
        track->frame_id = frame_id_;
        track->time_since_update = 0;
        ++track->tracklet_len;
        if (!track->kf_initialized) {
            track->kf.init(track->tlwh_to_xyah());
            track->kf_initialized = true;
        } else {
            track->kf.update(track->tlwh_to_xyah());
        }
        track->state = TrackState::TRACKED;
        hi_matched[j]    = true;
        pool_matched[i]  = true;
    }

    // ── 3) Second association: unmatched tracked vs. low-conf detections ─────
    std::vector<STrack*> remain_tracked;
    for (size_t i = 0; i < pool.size(); ++i) {
        if (!pool_matched[i] && pool[i]->state == TrackState::TRACKED)
            remain_tracked.push_back(pool[i]);
    }

    std::vector<STrack> lo_st;
    lo_st.reserve(low_dets.size());
    for (auto& d : low_dets) lo_st.push_back(detToTrack(d));
    std::vector<STrack*> lo_ptr;
    for (auto& s : lo_st) lo_ptr.push_back(&s);

    auto cost2 = buildIouCost(remain_tracked, lo_ptr);
    for (auto& v : cost2) if (v > 0.5f) v = Hungarian::INF_COST;
    auto assign2 = Hungarian::solve(cost2, remain_tracked.size(), lo_ptr.size());

    for (size_t i = 0; i < assign2.size(); ++i) {
        int j = assign2[i];
        if (j < 0) continue;
        STrack* track = remain_tracked[i];
        STrack& det   = lo_st[j];
        track->x = det.x; track->y = det.y;
        track->w = det.w; track->h = det.h;
        track->score = det.score;
        track->class_id = det.class_id;
        track->frame_id = frame_id_;
        track->time_since_update = 0;
        if (track->kf_initialized) track->kf.update(track->tlwh_to_xyah());
        track->state = TrackState::TRACKED;
    }

    // ── 4) Mark unmatched tracked as lost ────────────────────────────────────
    for (auto* t : remain_tracked) {
        if (t->time_since_update > 0) {
            t->state = TrackState::LOST;
        }
    }

    // ── 5) New tracks from unmatched high detections ─────────────────────────
    for (size_t j = 0; j < hi_ptr.size(); ++j) {
        if (hi_matched[j]) continue;
        STrack& det = hi_st[j];
        if (det.score < cfg_.new_track_thresh) continue;
        STrack new_t = det;
        new_t.track_id     = next_id_++;
        new_t.state        = TrackState::TRACKED;
        new_t.frame_id     = frame_id_;
        new_t.start_frame  = frame_id_;
        new_t.tracklet_len = 1;
        new_t.kf.init(new_t.tlwh_to_xyah());
        new_t.kf_initialized = true;
        tracked_.push_back(std::move(new_t));
    }

    // ── 6) Move lost->removed, age timers ────────────────────────────────────
    std::vector<STrack> next_tracked, next_lost;
    for (auto& t : tracked_) {
        if (t.state == TrackState::TRACKED) {
            next_tracked.push_back(t);
        } else if (t.state == TrackState::LOST) {
            t.time_since_update = 1;
            next_lost.push_back(t);
        }
    }
    for (auto& t : lost_) {
        ++t.time_since_update;
        if (t.time_since_update > cfg_.track_buffer)
            removed_.push_back(t);
        else
            next_lost.push_back(t);
    }
    tracked_ = std::move(next_tracked);
    lost_    = std::move(next_lost);

    // ── 7) Emit detections with track ids ────────────────────────────────────
    std::vector<Detection> out;
    out.reserve(tracked_.size());
    for (auto& t : tracked_) {
        Detection d;
        d.x = t.x; d.y = t.y; d.w = t.w; d.h = t.h;
        d.confidence = t.score;
        d.class_id   = t.class_id;
        d.track_id   = t.track_id;
        out.push_back(d);
    }
    return out;
}

}  // namespace edge
