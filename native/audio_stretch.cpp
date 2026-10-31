// SPDX-License-Identifier: GPL-2.0-or-later
#include "audio_stretch_api.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>
#include <memory>
#include <limits>
#include "SoundTouch.h"

namespace rchase2_audio_detail {
class TempoController {
    bool started_=false,estimated_=false;
    double window_start_=0,last_=0,ratio_=1,tempo_=1;
    uint64_t window_frames_=0,previous_frames_=0;
    double previous_elapsed_=0;
public:
    void reset() noexcept {*this=TempoController{};}
    double update(uint32_t frames,double now,uint32_t queued) noexcept {
        if(!std::isfinite(now))return tempo_;
        if(!started_ || now<last_) {
            started_=true;estimated_=false;window_start_=last_=now;window_frames_=0;
            previous_frames_=0;previous_elapsed_=0;
            return tempo_;
        }
        const double dt=now-last_;last_=now;window_frames_+=frames;
        const double elapsed=now-window_start_;
        bool first_estimate=false;
        // Aggregate arrivals before the EMA so back-to-back blocks and bursty
        // producer scheduling do not become implausible instantaneous rates.
        if(elapsed>=.08) {
            const double measured=std::clamp(double(window_frames_)/(48000.*elapsed),.65,1.05);
            first_estimate=!estimated_;
            // A sustained change must cross two complete arrival windows.
            // Ordinary jitter retains the slow EMA; after a genuine capacity
            // step, adopting the latest window avoids draining the reservoir
            // while the 350ms filter catches up. This is symmetric on recovery.
            const double combined=double(window_frames_+previous_frames_)/(48000.*(elapsed+previous_elapsed_));
            const bool sustained=previous_elapsed_>0 && std::abs(combined-ratio_)>.08;
            ratio_=sustained?measured:estimated_?ratio_+(1.-std::exp(-elapsed/.35))*(measured-ratio_):measured;
            previous_frames_=window_frames_;previous_elapsed_=elapsed;
            estimated_=true;window_start_=now;window_frames_=0;
        }
        // A small reservoir correction compensates slow estimator error; it
        // never replaces the measured native production-rate estimate.
        const double feedback=std::clamp((double(queued)-2400.)/48000.*.6,-.03,.03);
        const double desired=std::clamp(ratio_+feedback,.60,1.04);
        if(first_estimate)tempo_=desired;
        else tempo_+=std::clamp(desired-tempo_,-dt*2.,dt*2.);
        return tempo_;
    }
    double ratio() const noexcept {return ratio_;}
    double tempo() const noexcept {return tempo_;}
};

constexpr uint32_t SampleRate=48000,Channels=2,BlockFrames=2048;
constexpr uint32_t InputSlots=32,OutputCapacity=16384,PrefillFrames=2400;
constexpr uint64_t NoEpoch=std::numeric_limits<uint64_t>::max();
static_assert(std::atomic<uint64_t>::is_always_lock_free);
static_assert(std::atomic<uint32_t>::is_always_lock_free);
static_assert(std::atomic<double>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);

// Only the worker constructs or calls this object in the shipped path. Tests
// can pump the same core on an artificial arrival clock without a live thread.
class StretchCore {
    soundtouch::SoundTouch sound_;
    TempoController controller_;
    std::array<float,BlockFrames*Channels> converted_{};
public:
    StretchCore() {
        sound_.setSampleRate(SampleRate);sound_.setChannels(Channels);
        sound_.setRate(1);sound_.setPitch(1);sound_.setTempo(1);
        sound_.setSetting(SETTING_SEQUENCE_MS,32);
        sound_.setSetting(SETTING_SEEKWINDOW_MS,8);
        sound_.setSetting(SETTING_OVERLAP_MS,8);
        sound_.setSetting(SETTING_USE_QUICKSEEK,0);
    }
    void clear() {sound_.clear();controller_.reset();}
    void feed(const int16_t* samples,uint32_t frames,double now,uint32_t queued) {
        const double tempo=controller_.update(frames,now,queued);
        sound_.setTempo(tempo);
        for(size_t i=0;i<size_t(frames)*Channels;i++)converted_[i]=samples[i]/32768.f;
        sound_.putSamples(converted_.data(),frames);
    }
    uint32_t receive(float* samples,uint32_t frames) {return sound_.receiveSamples(samples,frames);}
    uint32_t available() const {return sound_.numSamples();}
    double ratio() const noexcept {return controller_.ratio();}
    double tempo() const noexcept {return controller_.tempo();}
    int initial_latency() const {return sound_.getSetting(SETTING_INITIAL_LATENCY);}
};

struct InputBlock {
    uint32_t frames=0;double now=0;uint64_t epoch=0;
    std::array<int16_t,BlockFrames*Channels> samples;
};
struct OutputSample {int16_t left=0,right=0;uint64_t epoch=0;};

struct Counters {
    std::atomic<uint64_t> native{0},output{0},requested{0},delivered{0},missing{0},warmup{0},paused{0};
    std::atomic<uint64_t> paused_input{0},input_dropped{0},output_dropped{0};
    std::atomic<uint64_t> callbacks{0},underrun_callbacks{0},input_blocks{0},output_blocks{0};
    std::atomic<double> tempo{1},ratio{1},max_input_gap{0},max_output_gap{0};
};

class AudioWorker {
    std::array<InputBlock,InputSlots> input_;
    std::array<OutputSample,OutputCapacity> output_;
    alignas(64) std::atomic<uint64_t> input_head_{0},input_tail_{0};
    alignas(64) std::atomic<uint64_t> output_head_{0},output_tail_{0};
    std::atomic<uint32_t> input_frames_{0};
    // Every pause transition increments the generation, including a pause and
    // resume that both occur between worker polls. No ring index is reset.
    std::atomic<uint64_t> control_{0},running_epoch_{NoEpoch},timeout_epoch_{NoEpoch};
    std::atomic<uint64_t> first_input_epoch_{NoEpoch};
    std::atomic<double> first_input_now_{0},latest_push_now_{0},latest_read_now_{0};
    std::atomic<bool> stop_{false},ready_{false},failed_{false};
    std::thread worker_;
    uint64_t producer_epoch_=NoEpoch;
    double previous_push_=0;
    Counters counters_;
    // Single SDL consumer: a bounded reader can reject a concurrently changing
    // callback partition without delaying the real-time writer.
    std::atomic<uint64_t> callback_sequence_{0};

    static int16_t pcm(float value) noexcept {
        const float bounded=std::clamp(value,-1.f,32767.f/32768.f);
        return static_cast<int16_t>(std::lrint(bounded*32768.f));
    }
    uint32_t generation_queued(uint64_t begin) const noexcept {
        const uint64_t head=output_head_.load(std::memory_order_relaxed);
        const uint64_t tail=std::max(begin,output_tail_.load(std::memory_order_acquire));
        return uint32_t(head-tail);
    }
    void work() noexcept {
        try {
            StretchCore core;
            std::array<float,BlockFrames*Channels> samples;
            uint64_t epoch=NoEpoch,output_begin=0;
            double previous_output=0;bool have_output=false;
            while(!stop_.load(std::memory_order_acquire)) {
                const uint64_t control=control_.load(std::memory_order_acquire);
                const uint64_t wanted=control>>1;
                const bool paused=(control&1)!=0;
                if(wanted!=epoch) {
                    core.clear();epoch=wanted;output_begin=output_head_.load(std::memory_order_relaxed);
                    have_output=false;running_epoch_.store(NoEpoch,std::memory_order_release);
                    counters_.tempo.store(1,std::memory_order_relaxed);counters_.ratio.store(1,std::memory_order_relaxed);
                }
                ready_.store(true,std::memory_order_release);
                bool did_work=false;
                if(!paused) {
                    // Do not remove DSP output unless the preallocated sample
                    // ring can accept it. Backpressure never blocks push/read.
                    const uint64_t head=output_head_.load(std::memory_order_relaxed);
                    const uint64_t tail=output_tail_.load(std::memory_order_acquire);
                    const uint32_t room=OutputCapacity-uint32_t(head-tail);
                    if(room && core.available()) {
                        const uint32_t count=core.receive(samples.data(),std::min(room,BlockFrames));
                        for(uint32_t i=0;i<count;i++) {
                            auto& sample=output_[(head+i)&(OutputCapacity-1)];
                            sample.left=pcm(samples[i*2]);sample.right=pcm(samples[i*2+1]);sample.epoch=epoch;
                        }
                        output_head_.store(head+count,std::memory_order_release);
                        counters_.output.fetch_add(count,std::memory_order_relaxed);
                        counters_.output_blocks.fetch_add(count!=0,std::memory_order_relaxed);
                        const double now=std::max(latest_push_now_.load(std::memory_order_relaxed),latest_read_now_.load(std::memory_order_relaxed));
                        if(count && have_output)counters_.max_output_gap.store(std::max(counters_.max_output_gap.load(std::memory_order_relaxed),now-previous_output),std::memory_order_relaxed);
                        if(count){previous_output=now;have_output=true;}
                        if(generation_queued(output_begin)>=PrefillFrames)
                            running_epoch_.store(epoch,std::memory_order_release);
                        did_work=count!=0;
                    }
                }
                const uint64_t tail=input_tail_.load(std::memory_order_relaxed);
                const uint64_t head=input_head_.load(std::memory_order_acquire);
                const bool room=output_head_.load(std::memory_order_relaxed)-output_tail_.load(std::memory_order_acquire)<OutputCapacity;
                if(tail!=head && (paused || room)) {
                    const auto& block=input_[tail&(InputSlots-1)];
                    if(paused || block.epoch!=epoch)
                        counters_.paused_input.fetch_add(block.frames,std::memory_order_relaxed);
                    else {
                        core.feed(block.samples.data(),block.frames,block.now,generation_queued(output_begin));
                        counters_.tempo.store(core.tempo(),std::memory_order_relaxed);
                        counters_.ratio.store(core.ratio(),std::memory_order_relaxed);
                    }
                    input_frames_.fetch_sub(block.frames,std::memory_order_relaxed);
                    input_tail_.store(tail+1,std::memory_order_release);did_work=true;
                }
                if(!did_work)std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        } catch(...) {
            failed_.store(true,std::memory_order_release);ready_.store(true,std::memory_order_release);
        }
    }
public:
    AudioWorker() {worker_=std::thread([this]{work();});}
    ~AudioWorker() {stop_.store(true,std::memory_order_release);if(worker_.joinable())worker_.join();}
    bool ready() const noexcept {return ready_.load(std::memory_order_acquire);}
    bool failed() const noexcept {return failed_.load(std::memory_order_acquire);}
    uint32_t push(const int16_t* data,uint32_t frames,double now) noexcept {
        if(!data || !frames)return 0;
        counters_.native.fetch_add(frames,std::memory_order_relaxed);
        const uint64_t control=control_.load(std::memory_order_acquire),epoch=control>>1;
        if(control&1) {counters_.paused_input.fetch_add(frames,std::memory_order_relaxed);return 0;}
        if(failed() || !std::isfinite(now)) {counters_.input_dropped.fetch_add(frames,std::memory_order_relaxed);return 0;}
        latest_push_now_.store(now,std::memory_order_relaxed);
        if(producer_epoch_!=epoch) {
            producer_epoch_=epoch;previous_push_=now;
            first_input_now_.store(now,std::memory_order_relaxed);first_input_epoch_.store(epoch,std::memory_order_release);
        } else {
            counters_.max_input_gap.store(std::max(counters_.max_input_gap.load(std::memory_order_relaxed),now-previous_push_),std::memory_order_relaxed);
            previous_push_=now;
        }
        uint32_t accepted=0;
        while(accepted<frames) {
            const uint64_t head=input_head_.load(std::memory_order_relaxed);
            if(head-input_tail_.load(std::memory_order_acquire)==InputSlots)break;
            const uint32_t count=std::min(BlockFrames,frames-accepted);
            auto& block=input_[head&(InputSlots-1)];block.frames=count;block.now=now;block.epoch=epoch;
            std::memcpy(block.samples.data(),data+size_t(accepted)*2,size_t(count)*2*sizeof(int16_t));
            input_frames_.fetch_add(count,std::memory_order_relaxed);
            input_head_.store(head+1,std::memory_order_release);
            counters_.input_blocks.fetch_add(1,std::memory_order_relaxed);accepted+=count;
        }
        counters_.input_dropped.fetch_add(frames-accepted,std::memory_order_relaxed);
        return accepted;
    }
    uint32_t read(int16_t* data,uint32_t frames,double now) noexcept {
        if(!data || !frames)return 0;
        callback_sequence_.fetch_add(1);
        std::fill_n(data,size_t(frames)*2,0);
        if(std::isfinite(now))latest_read_now_.store(now,std::memory_order_relaxed);
        counters_.callbacks.fetch_add(1);
        const uint64_t control=control_.load(std::memory_order_acquire),epoch=control>>1;
        const uint64_t head=output_head_.load(std::memory_order_acquire);
        uint64_t tail=output_tail_.load(std::memory_order_relaxed);
        uint64_t discarded=0;uint32_t delivered=0;
        if(control&1) {
            discarded=head-tail;output_tail_.store(head,std::memory_order_release);
            counters_.output_dropped.fetch_add(discarded,std::memory_order_relaxed);
            counters_.paused.fetch_add(frames);
            counters_.requested.fetch_add(frames);
            callback_sequence_.fetch_add(1);return 0;
        }
        const bool running=running_epoch_.load(std::memory_order_acquire)==epoch;
        while(tail<head && delivered<frames) {
            const auto& sample=output_[tail&(OutputCapacity-1)];
            if(sample.epoch!=epoch) {++tail;++discarded;continue;}
            if(!running)break; // Retain this generation's prefill.
            data[delivered*2]=sample.left;data[delivered*2+1]=sample.right;++tail;++delivered;
        }
        output_tail_.store(tail,std::memory_order_release);
        counters_.output_dropped.fetch_add(discarded,std::memory_order_relaxed);
        counters_.delivered.fetch_add(delivered);
        const uint32_t missing=frames-delivered;
        if(missing) {
            counters_.missing.fetch_add(missing);
            counters_.underrun_callbacks.fetch_add(1);
            bool warming=!running && !failed() && timeout_epoch_.load(std::memory_order_relaxed)!=epoch;
            if(warming && first_input_epoch_.load(std::memory_order_acquire)==epoch &&
               std::isfinite(now) && now-first_input_now_.load(std::memory_order_relaxed)>.25) {
                timeout_epoch_.store(epoch,std::memory_order_relaxed);warming=false;
            }
            if(warming)counters_.warmup.fetch_add(missing);
        }
        counters_.requested.fetch_add(frames);
        callback_sequence_.fetch_add(1);return delivered;
    }
    void pause(bool paused) noexcept {
        uint64_t value=control_.load(std::memory_order_relaxed);
        while((value&1)!=uint64_t(paused) && !control_.compare_exchange_weak(value,((value>>1)+1)*2+uint64_t(paused),std::memory_order_release,std::memory_order_relaxed)) {}
    }
    bool stats(Rchase2AudioStats& result) const noexcept {
        const uint64_t sequence=callback_sequence_.load();
        if(sequence&1)return false;
        Rchase2AudioStats s;s.size=sizeof(s);s.abi=Rchase2AudioAbi;s.sample_rate=SampleRate;s.channels=Channels;
        const uint64_t control=control_.load(std::memory_order_acquire),epoch=control>>1;
        s.state=(control&1)?Rchase2AudioPaused:failed()?Rchase2AudioFailed:
            running_epoch_.load(std::memory_order_acquire)==epoch?Rchase2AudioRunning:
            timeout_epoch_.load(std::memory_order_relaxed)==epoch?Rchase2AudioFailed:Rchase2AudioWarmup;
        s.input_queue_frames=input_frames_.load(std::memory_order_relaxed);
        const uint64_t output_tail=output_tail_.load(std::memory_order_acquire);
        s.output_queue_frames=uint32_t(output_head_.load(std::memory_order_acquire)-output_tail);
        s.native_frames=counters_.native.load();s.output_frames=counters_.output.load();s.requested_frames=counters_.requested.load();s.delivered_frames=counters_.delivered.load();
        s.missing_frames=counters_.missing.load();s.warmup_frames=counters_.warmup.load();s.paused_frames=counters_.paused.load();s.paused_input_frames=counters_.paused_input.load();
        s.input_dropped_frames=counters_.input_dropped.load();s.output_dropped_frames=counters_.output_dropped.load();s.callbacks=counters_.callbacks.load();s.underrun_callbacks=counters_.underrun_callbacks.load();
        s.input_blocks=counters_.input_blocks.load();s.output_blocks=counters_.output_blocks.load();s.tempo=counters_.tempo.load();s.input_ratio=counters_.ratio.load();
        s.max_input_gap_ms=counters_.max_input_gap.load()*1000.;s.max_output_gap_ms=counters_.max_output_gap.load()*1000.;
        // The two sequentially consistent sequence observations surround the
        // atomic callback counters. No retry, wait, or reader work enters read().
        if(callback_sequence_.load()!=sequence)return false;
        result=s;return true;
    }
};
}

extern "C" void* rchase2_audio_create(uint32_t rate,uint32_t channels) {
    try {
        if(rate!=48000 || channels!=2)return nullptr;
        auto worker=std::make_unique<rchase2_audio_detail::AudioWorker>();
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(!worker->ready() && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if(!worker->ready() || worker->failed())return nullptr;
        return worker.release();
    } catch(...) {return nullptr;}
}
extern "C" uint32_t rchase2_audio_push(void* handle,const int16_t* data,uint32_t frames,double now) {
    try {return handle?static_cast<rchase2_audio_detail::AudioWorker*>(handle)->push(data,frames,now):0;}catch(...) {return 0;}
}
extern "C" uint32_t rchase2_audio_read(void* handle,int16_t* data,uint32_t frames,double now) {
    try {
        if(handle)return static_cast<rchase2_audio_detail::AudioWorker*>(handle)->read(data,frames,now);
    }catch(...) {}
    if(data)std::fill_n(data,size_t(frames)*2,0);
    return 0;
}
extern "C" void rchase2_audio_pause(void* handle,uint32_t paused) {
    try {if(handle)static_cast<rchase2_audio_detail::AudioWorker*>(handle)->pause(paused!=0);}catch(...) {}
}
extern "C" uint32_t rchase2_audio_stats(void* handle,Rchase2AudioStats* output,uint32_t bytes) {
    try {
        if(!handle || !output || bytes<sizeof(*output))return 0;
        return static_cast<rchase2_audio_detail::AudioWorker*>(handle)->stats(*output)?1u:0u;
    }catch(...) {return 0;}
}
extern "C" void rchase2_audio_destroy(void* handle) {
    try {delete static_cast<rchase2_audio_detail::AudioWorker*>(handle);}catch(...) {}
}
