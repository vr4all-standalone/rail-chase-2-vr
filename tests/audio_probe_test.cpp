// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/audio_probe.h"
#include <cassert>
#include <cmath>
#include <type_traits>

using namespace rchase2;
static bool near(double a, double b) { return std::abs(a-b)<0.000001; }

static void counts_exact_frames_instead_of_whole_callbacks() {
    AudioProbe probe;
    probe.begin(0.0,0);
    probe.push(960,0,960,0.005);
    probe.callback(512,960,0.010);
    probe.callback(512,448,0.020);
    assert(!probe.due(0.999));
    assert(probe.due(1.000));
    probe.push(960,0,960,1.005);
    const auto record=probe.take(1.005);
    assert(near(record.seconds,1.005));
    assert(record.pushes==2 && record.pushed_frames==1920);
    assert(record.callbacks==2 && record.requested_frames==1024);
    assert(record.missing_frames==64 && record.underrun_callbacks==1);
    assert(record.dropped_frames==0);
    assert(record.queue_min==0 && record.queue_max==960 && record.queue_end==960);
    assert(near(record.max_push_gap_seconds,1.0));
    assert(near(record.max_callback_gap_seconds,0.985));
}

static void a_pause_is_reported_once_without_contaminating_the_next_interval() {
    AudioProbe probe;
    probe.begin(0.0,0);
    probe.push(960,0,960,0.0);
    probe.callback(512,960,0.010);
    probe.callback(512,448,0.020);
    probe.callback(512,0,5.0);
    probe.push(960,0,960,5.0);
    const auto paused=probe.take(5.0);
    assert(near(paused.seconds,5.0));
    assert(paused.missing_frames==576 && paused.underrun_callbacks==2);
    assert(near(paused.max_push_gap_seconds,5.0));
    assert(!probe.due(5.999) && probe.due(6.0));
    probe.callback(512,960,5.010);
    probe.push(960,448,1408,5.020);
    const auto normal=probe.take(6.0);
    assert(near(normal.seconds,1.0));
    assert(normal.pushes==1 && normal.pushed_frames==960);
    assert(normal.callbacks==1 && normal.requested_frames==512);
    assert(normal.missing_frames==0 && normal.underrun_callbacks==0);
    assert(normal.queue_min==448 && normal.queue_max==1408 && normal.queue_end==1408);
    assert(near(normal.max_push_gap_seconds,0.980));
    assert(near(normal.max_callback_gap_seconds,0.990));
}

static void queue_trimming_is_separate_from_starvation() {
    AudioProbe probe;
    probe.begin(10.0,2400);
    probe.push(960,2400,1920,10.01);
    probe.callback(512,1920,10.02);
    const auto record=probe.take(11.0);
    assert(record.dropped_frames==1440);
    assert(record.missing_frames==0 && record.underrun_callbacks==0);
    assert(record.queue_min==1408 && record.queue_max==2400);
    assert(record.queue_end==1408);
    const auto empty=probe.take(12.0);
    assert(empty.pushes==0 && empty.pushed_frames==0 && empty.callbacks==0);
    assert(empty.requested_frames==0 && empty.missing_frames==0 && empty.dropped_frames==0);
    assert(empty.queue_min==1408 && empty.queue_max==1408);
    assert(near(empty.max_push_gap_seconds,1.0));
    assert(near(empty.max_callback_gap_seconds,1.0));
}

int main() {
    static_assert(std::is_trivially_copyable_v<AudioProbe>);
    static_assert(noexcept(AudioProbe{}.callback(512,0,0.0)));
    counts_exact_frames_instead_of_whole_callbacks();
    a_pause_is_reported_once_without_contaminating_the_next_interval();
    queue_trimming_is_separate_from_starvation();
}
