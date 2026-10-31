// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
// The private controller/core are exercised without adding test exports to the
// shipped C ABI; this target compiles the implementation exactly once.
#include "../native/audio_stretch.cpp"
#include <cassert>
#include <cstdio>
#include <vector>
#include <complex>
#include <numbers>
#include <new>
#include <cstdlib>
#include <deque>

static thread_local bool rt_checked;
static thread_local uint64_t rt_allocations;
[[gnu::noinline]] void* operator new(size_t bytes) {
    if(rt_checked)++rt_allocations;
    if(void* data=std::malloc(bytes?bytes:1))return data;
    throw std::bad_alloc();
}
[[gnu::noinline]] void operator delete(void* data) noexcept {std::free(data);}
[[gnu::noinline]] void operator delete(void* data,size_t) noexcept {std::free(data);}
void* operator new[](size_t bytes) {return ::operator new(bytes);}
void operator delete[](void* data) noexcept {::operator delete(data);}
void operator delete[](void* data,size_t) noexcept {::operator delete(data);}
[[gnu::noinline]] void* operator new(size_t bytes,std::align_val_t alignment) {
    if(rt_checked)++rt_allocations;
    const size_t align=size_t(alignment),rounded=(bytes+align-1)/align*align;
#ifdef _WIN32
    if(void* data=_aligned_malloc(rounded,align))return data;
#else
    if(void* data=std::aligned_alloc(align,rounded))return data;
#endif
    throw std::bad_alloc();
}
[[gnu::noinline]] void operator delete(void* data,std::align_val_t) noexcept {
#ifdef _WIN32
    _aligned_free(data);
#else
    std::free(data);
#endif
}
void operator delete(void* data,size_t,std::align_val_t align) noexcept {::operator delete(data,align);}
void* operator new[](size_t bytes,std::align_val_t align) {return ::operator new(bytes,align);}
void operator delete[](void* data,std::align_val_t align) noexcept {::operator delete(data,align);}
void operator delete[](void* data,size_t,std::align_val_t align) noexcept {::operator delete(data,align);}

using namespace rchase2_audio_detail;
static void deterministic_arrival_controller() {
    TempoController controller;
    for(int block=0;block<160;block++)controller.update(960,10.+block*.025,2400);
    assert(std::abs(controller.ratio()-.8)<.005);
    assert(std::abs(controller.tempo()-.8)<.005);
    for(int block=160;block<240;block++)controller.update(960,10.+block*.025,0);
    assert(controller.tempo()>.74 && controller.tempo()<.795);
    for(int block=240;block<320;block++)controller.update(960,10.+block*.025,4800);
    assert(controller.tempo()>.805 && controller.tempo()<.86);
    controller.reset();
    for(int block=0;block<160;block++) {
        controller.update(480,100.+block*.025,2400);
        controller.update(480,100.+block*.025,2400);
    }
    assert(std::abs(controller.ratio()-.8)<.005);
    assert(std::abs(controller.tempo()-.8)<.005);
}
static void c_boundary_and_pause_accounting() {
    assert(!rchase2_audio_create(44100,2));
    assert(!rchase2_audio_create(48000,1));
    assert(!rchase2_audio_create(0,0));
    std::array<int16_t,1024> output;output.fill(123);
    assert(rchase2_audio_read(nullptr,output.data(),512,100.)==0);
    for(auto value:output)assert(value==0);
    assert(rchase2_audio_stats(nullptr,nullptr,0)==0);
    rchase2_audio_pause(nullptr,1);rchase2_audio_destroy(nullptr);
    void* handle=rchase2_audio_create(48000,2);assert(handle);
    rt_allocations=0;rt_checked=true;
    assert(rchase2_audio_read(handle,output.data(),512,100.)==0);
    rchase2_audio_pause(handle,1);
    assert(rchase2_audio_read(handle,output.data(),512,100.1)==0);
    rt_checked=false;assert(rt_allocations==0);
    std::array<int16_t,1920> input{};
    assert(rchase2_audio_push(handle,input.data(),960,100.1)==0);
    rchase2_audio_pause(handle,0);
    assert(rchase2_audio_read(handle,output.data(),512,100.2)==0);
    Rchase2AudioStats stats;
    assert(rchase2_audio_stats(handle,&stats,sizeof(stats))==1);
    assert(stats.size==sizeof(stats)&&stats.abi==Rchase2AudioAbi);
    assert(stats.sample_rate==48000&&stats.channels==2);
    assert(stats.requested_frames==1536 && stats.delivered_frames==0);
    assert(stats.missing_frames==1024 && stats.paused_frames==512);
    assert(stats.warmup_frames<=stats.missing_frames);
    assert(stats.native_frames==960 && stats.paused_input_frames==960);
    assert(stats.input_dropped_frames==0);
    assert(rchase2_audio_stats(handle,&stats,sizeof(stats)-1)==0);
    rchase2_audio_destroy(handle);
}
static void concurrent_stats_partition() {
    void* handle=rchase2_audio_create(48000,2);assert(handle);
    std::atomic<bool> start{false},done{false};
    std::thread consumer([&] {
        std::array<int16_t,1024> output;
        while(!start.load(std::memory_order_acquire))std::this_thread::yield();
        rt_allocations=0;rt_checked=true;
        for(unsigned i=0;i<100000;i++) {
            rchase2_audio_pause(handle,(i%5)==0);
            rchase2_audio_read(handle,output.data(),512,100.+i*.01);
        }
        rt_checked=false;assert(rt_allocations==0);
        done.store(true,std::memory_order_release);
    });
    unsigned snapshots=0;
    start.store(true,std::memory_order_release);
    do {
        Rchase2AudioStats stats;
        if(rchase2_audio_stats(handle,&stats,sizeof(stats))) {
            ++snapshots;
            assert(stats.requested_frames==stats.delivered_frames+stats.missing_frames+stats.paused_frames);
            assert(stats.warmup_frames<=stats.missing_frames);
        }
    } while(!done.load(std::memory_order_acquire));
    consumer.join();
    Rchase2AudioStats final;assert(rchase2_audio_stats(handle,&final,sizeof(final))==1);
    assert(snapshots>0 && final.callbacks==100000 && final.requested_frames==51200000);
    assert(final.requested_frames==final.delivered_frames+final.missing_frames+final.paused_frames);
    rchase2_audio_destroy(handle);
}
static std::array<int16_t,1920> tone(uint64_t first) {
    std::array<int16_t,1920> result;
    for(unsigned i=0;i<960;i++) {
        const auto value=int16_t(std::lrint(12000.*std::sin(2.*std::numbers::pi*440.*double(first+i)/48000.)));
        result[i*2]=value;result[i*2+1]=-value;
    }
    return result;
}
static double dominant_pitch(const std::vector<int16_t>& stereo) {
    constexpr size_t n=32768;
    assert(stereo.size()>=n*2);
    std::vector<std::complex<double>> spectrum(n);
    const size_t begin=stereo.size()/2-n;
    for(size_t i=0;i<n;i++)spectrum[i]=double(stereo[(begin+i)*2])*(.5-.5*std::cos(2.*std::numbers::pi*i/(n-1)));
    for(size_t i=1,j=0;i<n;i++) {
        size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;
        if(i<j)std::swap(spectrum[i],spectrum[j]);
    }
    for(size_t length=2;length<=n;length*=2) {
        const auto step=std::polar(1.,-2.*std::numbers::pi/length);
        for(size_t base=0;base<n;base+=length) {
            std::complex<double> phase=1;
            for(size_t i=0;i<length/2;i++) {
                const auto a=spectrum[base+i],b=phase*spectrum[base+i+length/2];
                spectrum[base+i]=a+b;spectrum[base+i+length/2]=a-b;phase*=step;
            }
        }
    }
    size_t peak=1;for(size_t i=2;i<n/2;i++)if(std::norm(spectrum[i])>std::norm(spectrum[peak]))peak=i;
    return double(peak)*48000./n;
}
static void assert_tone(const std::vector<int16_t>& samples,double ratio,const char* label) {
    int peak=0,antiphase_error=0;
    for(size_t i=0;i+1<samples.size();i+=2) {
        peak=std::max(peak,std::abs(int(samples[i])));
        antiphase_error=std::max(antiphase_error,std::abs(int(samples[i])+samples[i+1]));
    }
    const double pitch=dominant_pitch(samples);
    std::printf("%s ratio=%.2f pitch=%.3fHz peak=%d stereo_error=%d\n",label,ratio,pitch,peak,antiphase_error);
    assert(std::abs(pitch-440.)<2.0);
    assert(peak>10000 && peak<32767);
    assert(antiphase_error<=2);
}
static void deterministic_dsp_clock(double ratio) {
    StretchCore core;
    std::deque<std::array<int16_t,2>> queue;
    std::vector<int16_t> heard;
    std::array<float,4096> converted;
    double next_input=0,next_read=0,first_delivery=-1;uint64_t native=0,steady_missing=0;
    bool running=false;
    while(std::min(next_input,next_read)<6.) {
        if(next_input<=next_read) {
            const auto input=tone(native);native+=960;
            core.feed(input.data(),960,100.+next_input,uint32_t(queue.size()));
            uint32_t count;
            while((count=core.receive(converted.data(),2048))!=0)for(uint32_t i=0;i<count;i++)
                queue.push_back({int16_t(std::lrint(std::clamp(converted[i*2],-1.f,.999969f)*32768.f)),int16_t(std::lrint(std::clamp(converted[i*2+1],-1.f,.999969f)*32768.f))});
            next_input+=.02/ratio;
        } else {
            if(!running && queue.size()>=2400)running=true;
            uint32_t delivered=0;
            if(running)while(delivered<512 && !queue.empty()) {
                const auto sample=queue.front();queue.pop_front();++delivered;
                if(next_read>=1.){heard.push_back(sample[0]);heard.push_back(sample[1]);}
            }
            if(delivered && first_delivery<0)first_delivery=next_read;
            if(next_read>=1.)steady_missing+=512-delivered;
            next_read+=512./48000.;
        }
    }
    std::printf("deterministic ratio=%.2f latency_setting=%d first_delivery_ms=%.3f steady_missing=%llu queue=%zu tempo=%.4f\n",ratio,core.initial_latency(),first_delivery*1000.,(unsigned long long)steady_missing,queue.size(),core.tempo());
    assert(first_delivery>=0 && first_delivery<.25);
    assert(steady_missing==0);
    assert(std::abs(core.ratio()-ratio)<.01);
    assert_tone(heard,ratio,"deterministic");
}
// Exercise the same streaming core through changing native capacity, with
// callback phase varied independently of the native 960-frame block clock.
static void sustained_rate_change_clock(double callback_phase,bool jitter) {
    StretchCore core;std::deque<std::array<int16_t,2>> queue;
    std::array<float,4096> converted;std::vector<int16_t> heard;
    double next_input=0,next_read=callback_phase,nominal=0,previous=0;
    double min_tempo=2,max_tempo=0;uint64_t native=0,missing=0;
    unsigned block=0;size_t max_queue=0;bool running=false;
    while(next_read<12.) {
        if(next_input<=next_read) {
            const auto input=tone(native);native+=960;
            core.feed(input.data(),960,100.+next_input,uint32_t(queue.size()));
            uint32_t count;
            while((count=core.receive(converted.data(),2048))!=0)for(uint32_t i=0;i<count;i++)
                queue.push_back({int16_t(std::lrint(std::clamp(converted[i*2],-1.f,.999969f)*32768.f)),int16_t(std::lrint(std::clamp(converted[i*2+1],-1.f,.999969f)*32768.f))});
            max_queue=std::max(max_queue,queue.size());
            if(next_input>=1.){min_tempo=std::min(min_tempo,core.tempo());max_tempo=std::max(max_tempo,core.tempo());}
            const double ratio=jitter?.9:(nominal<3.?1.:nominal<6.?.8:1.);
            nominal+=.02/ratio;++block;
            next_input=jitter?std::max(previous,nominal+.008*std::sin(block*2.1)):nominal;
            previous=next_input;
        } else {
            if(!running && queue.size()>=PrefillFrames)running=true;
            uint32_t delivered=0;
            if(running)while(delivered<512 && !queue.empty()) {
                const auto sample=queue.front();queue.pop_front();++delivered;
                if(next_read>=1.){heard.push_back(sample[0]);heard.push_back(sample[1]);}
            }
            if(next_read>=1.)missing+=512-delivered;
            next_read+=512./48000.;
        }
    }
    std::printf("sustained %s phase_ms=%.3f missing=%llu max_queue=%zu tempo_range=%.4f..%.4f\n",jitter?"jitter":"1-.8-1",callback_phase*1000.,(unsigned long long)missing,max_queue,min_tempo,max_tempo);
    assert(missing==0);assert(max_queue<8192);
    if(jitter){assert(min_tempo>.85 && max_tempo<.95);}
    assert_tone(heard,0.,jitter?"jitter":"rate-change");
}
static void actual_worker_clock(double ratio) {
    void* handle=rchase2_audio_create(48000,2);assert(handle);
    using Clock=std::chrono::steady_clock;
    const auto start=Clock::now();double next_input=0,next_read=0,first_delivery=-1;
    uint64_t native=0,steady_missing=0;std::vector<int16_t> heard;
    std::array<int16_t,1024> output;
    while(std::min(next_input,next_read)<3.) {
        const double at=std::min(next_input,next_read);
        std::this_thread::sleep_until(start+std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(at)));
        if(next_input<=next_read) {
            const auto input=tone(native);native+=960;
            assert(rchase2_audio_push(handle,input.data(),960,1000.+at)==960);
            next_input+=.02/ratio;
        } else {
            rt_allocations=0;rt_checked=true;
            const auto delivered=rchase2_audio_read(handle,output.data(),512,1000.+at);
            rt_checked=false;assert(rt_allocations==0);
            if(delivered && first_delivery<0)first_delivery=at;
            if(at>=1.) {steady_missing+=512-delivered;heard.insert(heard.end(),output.begin(),output.end());}
            next_read+=512./48000.;
        }
    }
    Rchase2AudioStats stats;assert(rchase2_audio_stats(handle,&stats,sizeof(stats))==1);
    std::printf("worker ratio=%.2f first_delivery_ms=%.3f steady_missing=%llu input_ratio=%.4f tempo=%.4f queue=%u missing=%llu warmup=%llu\n",ratio,first_delivery*1000.,(unsigned long long)steady_missing,stats.input_ratio,stats.tempo,stats.output_queue_frames,(unsigned long long)stats.missing_frames,(unsigned long long)stats.warmup_frames);
    assert(first_delivery>=0 && first_delivery<.25);
    assert(stats.state==Rchase2AudioRunning);
    assert(stats.requested_frames==stats.delivered_frames+stats.missing_frames+stats.paused_frames);
    assert(stats.warmup_frames<=stats.missing_frames && stats.native_frames==native);
    assert(stats.input_dropped_frames==0 && stats.output_dropped_frames==0);
    assert(steady_missing==0);
    assert_tone(heard,ratio,"worker");
    rchase2_audio_destroy(handle);
}
static void bounded_warmup_and_no_stale_resume() {
    void* handle=rchase2_audio_create(48000,2);assert(handle);
    const auto input=tone(0);std::array<int16_t,1024> output;
    assert(rchase2_audio_push(handle,input.data(),960,100.)==960);
    rchase2_audio_read(handle,output.data(),512,100.01);
    Rchase2AudioStats before,after;rchase2_audio_stats(handle,&before,sizeof(before));
    rchase2_audio_read(handle,output.data(),512,100.3);
    rchase2_audio_stats(handle,&after,sizeof(after));
    assert(after.state==Rchase2AudioFailed && after.warmup_frames==before.warmup_frames);
    rchase2_audio_destroy(handle);
    handle=rchase2_audio_create(48000,2);assert(handle);
    for(int i=0;i<12;i++) {
        assert(rchase2_audio_push(handle,input.data(),960,100.+i*.02)==960);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    rchase2_audio_stats(handle,&before,sizeof(before));assert(before.state==Rchase2AudioRunning);
    // There is deliberately no read between pause/resume: a worker may observe
    // only the final control value, so a generation is required, not just bool.
    rt_allocations=0;rt_checked=true;
    rchase2_audio_pause(handle,1);rchase2_audio_pause(handle,0);
    assert(rchase2_audio_read(handle,output.data(),512,100.3)==0);
    rt_checked=false;assert(rt_allocations==0);
    for(auto value:output)assert(value==0);
    rchase2_audio_destroy(handle);
}
// Starvation after nonzero playback must become silence, and pause/resume
// must reject every sample from the previous DSP generation.
static void starvation_silence_and_resume_generation() {
    void* handle=rchase2_audio_create(48000,2);assert(handle);
    std::array<int16_t,1920> input;input.fill(-12000);
    std::array<int16_t,4096> output;
    for(unsigned i=0;i<12;i++) {
        assert(rchase2_audio_push(handle,input.data(),960,100.+i*.02)==960);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bool heard=false,silent=false;
    for(unsigned i=0;i<32;i++) {
        output.fill(321);
        const auto delivered=rchase2_audio_read(handle,output.data(),2048,100.3+i*.04);
        heard|=delivered!=0;silent|=delivered==0;
        for(size_t j=size_t(delivered)*2;j<output.size();j++)assert(output[j]==0);
    }
    assert(heard&&silent);
    // Neither the worker nor the consumer need observe the paused boolean:
    // both transitions increment epochs, flushing old queued work.
    rchase2_audio_pause(handle,1);rchase2_audio_pause(handle,0);
    assert(rchase2_audio_read(handle,output.data(),2048,102.)==0);
    input.fill(12000);
    for(unsigned i=0;i<12;i++) {
        assert(rchase2_audio_push(handle,input.data(),960,102.+i*.02)==960);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto delivered=rchase2_audio_read(handle,output.data(),2048,102.3);
    assert(delivered>0);
    for(unsigned i=0;i<delivered*2;i++)assert(output[i]>=0);
    rchase2_audio_destroy(handle);
}
static void overload_is_bounded_and_pause_discards_the_backlog() {
    void* handle=rchase2_audio_create(48000,2);assert(handle);
    auto input=tone(0);std::array<int16_t,1024> output;
    for(unsigned i=0;i<2000;i++)rchase2_audio_push(handle,input.data(),960,100.+i*.00001);
    Rchase2AudioStats stats;assert(rchase2_audio_stats(handle,&stats,sizeof(stats))==1);
    assert(stats.native_frames==1920000&&stats.input_dropped_frames>0);
    assert(stats.input_queue_frames<=65536&&stats.output_queue_frames<=16384);
    rchase2_audio_pause(handle,1);
    assert(rchase2_audio_read(handle,output.data(),512,101.)==0);
    for(auto sample:output)assert(sample==0);
    rchase2_audio_pause(handle,0);
    assert(rchase2_audio_read(handle,output.data(),512,101.01)==0);
    for(auto sample:output)assert(sample==0);
    rchase2_audio_destroy(handle);
}
int main() {
    starvation_silence_and_resume_generation();
    overload_is_bounded_and_pause_discards_the_backlog();
    deterministic_arrival_controller();
    c_boundary_and_pause_accounting();
    concurrent_stats_partition();
    bounded_warmup_and_no_stale_resume();
    for(double ratio:{.7,.74,.8,.82,.9,1.})deterministic_dsp_clock(ratio);
    for(double phase:{0.,.0028,.0052,.0076})sustained_rate_change_clock(phase,false);
    sustained_rate_change_clock(0.,true);
    for(double ratio:{.74,.82,1.})actual_worker_clock(ratio);
    std::puts("Adaptive audio controller, DSP and worker regressions passed");
}
