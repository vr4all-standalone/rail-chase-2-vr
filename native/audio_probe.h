// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>

namespace rchase2 {
struct AudioProbeRecord {
    double seconds=0;
    uint64_t pushes=0,pushed_frames=0,callbacks=0,requested_frames=0;
    uint64_t missing_frames=0,underrun_callbacks=0,dropped_frames=0;
    uint32_t queue_min=0,queue_max=0,queue_end=0;
    double max_push_gap_seconds=0,max_callback_gap_seconds=0;
};
class AudioProbe {
public:
    // Caller supplies one monotonic clock and serializes access. The SDL
    // callback only touches these scalar fields under SDL's existing lock.
    void begin(double now,uint32_t queued) noexcept {
        record_={};
        record_.queue_min=record_.queue_max=record_.queue_end=queued;
        start_=last_push_=last_callback_=now;
    }
    void push(uint32_t frames,uint32_t before,uint32_t after,double now) noexcept {
        ++record_.pushes;
        record_.pushed_frames+=frames;
        const uint64_t offered=uint64_t(before)+frames;
        if(offered>after)record_.dropped_frames+=offered-after;
        record_.max_push_gap_seconds=std::max(record_.max_push_gap_seconds,now-last_push_);
        last_push_=now;
        observe_queue(before);
        observe_queue(after);
    }
    void callback(uint32_t frames,uint32_t queued,double now) noexcept {
        ++record_.callbacks;
        record_.requested_frames+=frames;
        if(frames>queued) {
            record_.missing_frames+=frames-queued;
            ++record_.underrun_callbacks;
        }
        record_.max_callback_gap_seconds=std::max(record_.max_callback_gap_seconds,now-last_callback_);
        last_callback_=now;
        observe_queue(queued);
        observe_queue(queued>frames?queued-frames:0);
    }
    bool due(double now) const noexcept { return now-start_>=1.0; }
    AudioProbeRecord take(double now) noexcept {
        record_.seconds=std::max(now-start_,0.0);
        // Clip a gap spanning two records to each record's interval, including
        // callbacks continuing while the producer is paused. Resume starts a
        // fresh baseline; previous missing frames are never reported twice.
        record_.max_push_gap_seconds=std::max(record_.max_push_gap_seconds,now-last_push_);
        record_.max_callback_gap_seconds=std::max(record_.max_callback_gap_seconds,now-last_callback_);
        const auto result=record_;
        begin(now,record_.queue_end);
        return result;
    }
private:
    void observe_queue(uint32_t queued) noexcept {
        record_.queue_min=std::min(record_.queue_min,queued);
        record_.queue_max=std::max(record_.queue_max,queued);
        record_.queue_end=queued;
    }
    AudioProbeRecord record_;
    double start_=0,last_push_=0,last_callback_=0;
};
}
