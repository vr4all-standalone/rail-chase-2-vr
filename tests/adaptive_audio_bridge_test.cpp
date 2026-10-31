// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/adaptive_audio_bridge.h"
#include <array>
#include <cassert>
#include <memory>

using namespace rchase2;
namespace {
struct Module {int* unloaded;~Module(){++*unloaded;}};
std::weak_ptr<void> loaded;
Rchase2AudioStats observed;
unsigned creates,destroys,pushes,reads,pauses;
bool create_fails,stats_fails,destroy_while_loaded;
uint32_t last_pause,last_frames;
double last_now;
int token;
void* create(uint32_t rate,uint32_t channels){assert(rate==48000&&channels==2);++creates;return create_fails?nullptr:&token;}
uint32_t push(void* h,const int16_t* data,uint32_t frames,double now){assert(h==&token&&data[0]==12);++pushes;last_frames=frames;last_now=now;return frames;}
uint32_t read(void* h,int16_t* data,uint32_t frames,double now){assert(h==&token);++reads;last_frames=frames;last_now=now;for(unsigned i=0;i<frames*2;i++)data[i]=i<4?45:0;return std::min(frames,2u);}
void pause(void* h,uint32_t paused){assert(h==&token);++pauses;last_pause=paused;}
uint32_t stats(void* h,Rchase2AudioStats* out,uint32_t bytes){assert(h==&token&&bytes==sizeof(*out));*out=observed;return stats_fails?0:1;}
void destroy(void* h){assert(h==&token);++destroys;destroy_while_loaded=!loaded.expired();}
AudioStretchFunctions functions(){return {create,push,read,pause,stats,destroy};}
void reset(){creates=destroys=pushes=reads=pauses=0;create_fails=stats_fails=destroy_while_loaded=false;observed={};observed.size=sizeof(observed);observed.abi=Rchase2AudioAbi;observed.sample_rate=48000;observed.channels=2;}
}
int main(){
    assert(!audio_stretch_requested(nullptr,"1",nullptr));
    assert(!audio_stretch_requested("1",nullptr,nullptr));
    assert(!audio_stretch_requested("1","10",nullptr));
    assert(!audio_stretch_requested("1","0",nullptr));
    assert(!audio_stretch_requested("1","1",""));
    assert(audio_stretch_requested("","1",nullptr));
    const char audio_library[]="audio.dll",xr_library[]="xr.dll";
    assert(audio_stretch_library(audio_library,xr_library)==audio_library);
    assert(audio_stretch_library("",xr_library)==xr_library);
    assert(audio_stretch_library(nullptr,xr_library)==xr_library);
    assert(audio_stretch_library(nullptr,nullptr)==nullptr);
    {
        AudioStretchBridge inactive;std::array<int16_t,8> silence;silence.fill(-1);
        assert(inactive.read(silence.data(),4,0,false)==0);
        for(auto sample:silence)assert(sample==0);
        assert(inactive.push(silence.data(),4,0)==0);
    }
    reset();int unloaded=0;
    {
        auto module=std::make_shared<Module>();module->unloaded=&unloaded;loaded=module;
        AudioStretchBridge bridge;
        assert(bridge.open(functions(),module,48000,2)==AudioStretchOpen::Ready);
        assert(creates==1&&bridge.active());
        module.reset();assert(!loaded.expired()&&unloaded==0);
        const int16_t input[]={12,13,14,15};assert(bridge.push(input,2,4.5)==2&&pushes==1&&last_frames==2&&last_now==4.5);
        std::array<int16_t,8> output;output.fill(-1);
        assert(bridge.read(output.data(),4,5.0,true)==2);
        assert(reads==1&&pauses==1&&last_pause==1&&last_frames==4&&last_now==5.0);
        assert(output[0]==45&&output[3]==45&&output[4]==0&&output[7]==0);
        bridge.read(output.data(),4,6.0,false);assert(last_pause==0);
        Rchase2AudioStats snapshot;assert(bridge.snapshot(snapshot)&&snapshot.abi==Rchase2AudioAbi);
        bridge.reset();assert(destroys==1&&destroy_while_loaded&&loaded.expired()&&unloaded==1&&!bridge.active());
        bridge.reset();assert(destroys==1);
    }
    for(unsigned missing=0;missing<6;missing++){
        reset();auto api=functions();
        if(missing==0)api.create=nullptr;
        if(missing==1)api.push=nullptr;
        if(missing==2)api.read=nullptr;
        if(missing==3)api.pause=nullptr;
        if(missing==4)api.stats=nullptr;
        if(missing==5)api.destroy=nullptr;
        auto module=std::make_shared<int>();AudioStretchBridge bridge;
        assert(bridge.open(api,module,48000,2)==AudioStretchOpen::MissingExports&&!bridge.active()&&creates==0);
    }
    for(unsigned bad=0;bad<6;bad++){
        reset();auto module=std::make_shared<int>();loaded=module;AudioStretchBridge bridge;
        if(bad==0)observed.abi=0;
        if(bad==1)observed.size=sizeof(observed)-1;
        if(bad==2)observed.sample_rate=44100;
        if(bad==3)observed.channels=1;
        if(bad==4)stats_fails=true;
        if(bad==5)observed.state=Rchase2AudioFailed;
        assert(bridge.open(functions(),module,48000,2)==AudioStretchOpen::InvalidStats);
        assert(!bridge.active()&&creates==1&&destroys==1&&destroy_while_loaded);
    }
    reset();{AudioStretchBridge bridge;auto module=std::make_shared<int>();loaded=module;create_fails=true;
        assert(bridge.open(functions(),module,48000,2)==AudioStretchOpen::CreateFailed&&creates==1&&destroys==0);}
    reset();{AudioStretchBridge bridge;auto module=std::make_shared<int>();
        assert(bridge.open(functions(),module,44100,2)==AudioStretchOpen::Unsupported&&creates==0);
        assert(bridge.open(functions(),module,48000,1)==AudioStretchOpen::Unsupported&&creates==0);}
    reset();{AudioStretchBridge bridge;auto module=std::make_shared<int>();loaded=module;
        assert(bridge.open(functions(),module,48000,2)==AudioStretchOpen::Ready);}
    assert(destroys==1&&destroy_while_loaded); // Destruction also joins before unloading.
    AudioStretchProbe probe;Rchase2AudioStats initial{};probe.begin(10,initial);
    assert(!probe.due(10.99)&&probe.due(11));
    auto total=initial;total.native_frames=900;total.output_frames=1000;total.requested_frames=1100;
    total.delivered_frames=1000;total.missing_frames=90;total.warmup_frames=10;total.paused_frames=10;
    total.paused_input_frames=12;total.input_dropped_frames=3;total.output_dropped_frames=4;
    total.callbacks=20;total.underrun_callbacks=2;total.input_blocks=8;total.output_blocks=9;
    total.input_queue_frames=33;total.output_queue_frames=44;total.tempo=.9;total.input_ratio=.91;total.max_input_gap_ms=15;
    const auto first=probe.take(total,11);
    assert(first.seconds==1&&first.delta.native_frames==900&&first.delta.output_frames==1000&&first.delta.requested_frames==1100);
    assert(first.delta.missing_frames==90&&first.delta.warmup_frames==10&&first.delta.paused_frames==10&&first.delta.paused_input_frames==12);
    assert(first.delta.input_dropped_frames==3&&first.delta.output_dropped_frames==4&&first.delta.callbacks==20&&first.delta.underrun_callbacks==2);
    assert(first.delta.input_blocks==8&&first.delta.output_blocks==9&&first.total.input_queue_frames==33&&first.total.tempo==.9);
    total.native_frames+=7;total.requested_frames+=16;total.missing_frames+=16;total.callbacks++;
    const auto second=probe.take(total,12);
    assert(second.delta.native_frames==7&&second.delta.output_frames==0&&second.delta.requested_frames==16&&second.delta.missing_frames==16);
    assert(second.delta.callbacks==1&&second.delta.warmup_frames==0&&second.delta.paused_frames==0&&second.total.output_queue_frames==44);
}
