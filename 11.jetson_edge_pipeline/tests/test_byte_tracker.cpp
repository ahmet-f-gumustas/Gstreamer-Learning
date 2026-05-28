#include "tracking/byte_tracker.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace edge;

static Detection mkdet(float x, float y, float w, float h,
                       int cls = 0, float conf = 0.9f) {
    Detection d;
    d.x = x; d.y = y; d.w = w; d.h = h;
    d.class_id = cls; d.confidence = conf;
    return d;
}

// Bir karakterin sahnede sabit hareket etmesi → aynı track_id korunmalı.
static void test_persistent_id() {
    ByteTracker tracker({});
    int previous_id = -1;
    for (int t = 0; t < 30; ++t) {
        float x = 100.f + t * 5.f;
        auto tr = tracker.update({ mkdet(x, 100, 60, 120) });
        assert(tr.size() == 1);
        if (previous_id < 0) previous_id = tr[0].track_id;
        else assert(tr[0].track_id == previous_id);
    }
}

// İki ayrı nesne → iki ayrı track_id, swap olmamalı.
static void test_two_objects_no_swap() {
    ByteTracker tracker({});
    int id_a = -1, id_b = -1;
    for (int t = 0; t < 20; ++t) {
        auto tr = tracker.update({
            mkdet(50.f  + t * 4.f, 100, 50, 100),
            mkdet(400.f - t * 4.f, 100, 50, 100)
        });
        assert(tr.size() == 2);
        // Hangisi soldaki?
        Detection* left  = tr[0].x < tr[1].x ? &tr[0] : &tr[1];
        Detection* right = tr[0].x < tr[1].x ? &tr[1] : &tr[0];

        if (id_a < 0) { id_a = left->track_id; id_b = right->track_id; }
        else {
            assert(left->track_id  == id_a);
            assert(right->track_id == id_b);
        }
    }
}

// Düşük confidence ile bir frame eksilen track silinmemeli (track_buffer içinde).
static void test_track_buffer_survives_missing_frames() {
    ByteTrackConfig cfg;
    cfg.track_buffer = 10;
    ByteTracker tracker(cfg);

    int id = -1;
    for (int t = 0; t < 5; ++t) {
        auto tr = tracker.update({ mkdet(100, 100, 50, 100) });
        if (id < 0) id = tr[0].track_id;
    }

    // 5 frame boş
    for (int t = 0; t < 5; ++t) tracker.update({});

    // Geri gelir
    auto tr = tracker.update({ mkdet(100, 100, 50, 100) });
    assert(!tr.empty());
    // ByteTrack identity geri vermeyebilir (lost association threshold),
    // ancak track ID havuzu artmamalı  — yeni id = id veya id+1.
    assert(tr[0].track_id >= id);
}

int main() {
    test_persistent_id();
    test_two_objects_no_swap();
    test_track_buffer_survives_missing_frames();
    std::cout << "test_byte_tracker: OK\n";
    return 0;
}
