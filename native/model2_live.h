// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "host_bridge.h"
#include "raster_policy.h"
#include "model2_trace.h"
#include "frame_pool.h"
#include <iomanip>
#include <string_view>
namespace rchase2 {
inline SceneCapture& live_capture(){static SceneCapture capture;return capture;}
inline bool& live_active(){static bool active{};return active;}
inline bool& live_pending(){static bool pending{};return pending;}
inline std::unique_ptr<NativeFrame>& live_raster(){static std::unique_ptr<NativeFrame> frame;return frame;}
inline uint64_t& live_sequence(){static uint64_t sequence{};return sequence;}
inline NativeFramePool& live_frame_pool(){static NativeFramePool pool;return pool;}
inline bool live_reuse_frames(){static const bool enabled=[](){const char* value=std::getenv("RCHASE2_FRAME_POOL");return !value||std::string_view(value)!="0";}();return enabled;}
inline void live_begin(uint64_t sequence,double time) {
    live_sequence()=sequence;
    live_active()=vr_requested();
    if(live_active()){live_capture().begin(sequence,time,live_reuse_frames()?live_frame_pool().acquire():NativeFrame{});live_pending()=false;}
}
inline bool live_skip_cpu_raster(bool direct,bool full_frame) {
    auto& runtime=shared_runtime();
    auto published=runtime.scene.load();
    const auto* frame=live_raster()?live_raster().get():published.get();
    RasterContext context;context.vr=vr_requested();
    const char* requested=std::getenv("RCHASE2_FAST_RASTER");
    context.requested=requested&&std::string_view(requested)=="1";
    context.reference=std::getenv("RCHASE2_CPU_RASTER")!=nullptr;
    context.capture=std::getenv("RCHASE2_SCENE_DIR")!=nullptr;
    context.host_ready=runtime.host_ready;context.quit=runtime.quit_requested;
    context.direct=direct;context.full_frame=full_frame;context.sequence=live_sequence();
    context.state=runtime.native_snapshot().state;
    const bool skip=skip_cpu_raster(context,frame);
    static bool announced=false;
    if(skip&&!announced)if(const char* path=std::getenv("RCHASE2_XR_LOG")) {
        std::ofstream out(path,std::ios::app);
        out<<"Guarded CPU world raster bypass active at native frame "<<context.sequence<<'\n';announced=true;
    }
    if(const char* path=std::getenv("RCHASE2_RASTER_LOG")) {
        static std::ofstream out(path);static bool header=false;
        if(!header){out<<"sequence\ttime\tmode\tskip\tpolygons\n";header=true;}
        out<<context.sequence<<'\t'<<std::setprecision(17)<<(frame?frame->time:0)<<'\t'<<context.state.mode<<'\t'<<skip<<'\t'<<(frame?frame->polygons.size():0)<<'\n';
    }
    return skip;
}
// Bounded, opt-in producer capture for reference/optimized fidelity proofs.
// Unlike RCHASE2_SCENE_DIR, this records the actual optimized path. It is never
// enabled by the app. It owns no game state and cannot drive cabinet inputs.
inline void live_proof(const NativeFrame& frame) {
    const char* root=std::getenv("RCHASE2_LIVE_PROOF_DIR");
    static double next=5;static unsigned count{};
    if(!root||count>=12||frame.time<next||!trace_pending_raster().empty())return;
    write_trace(frame,root);
    const std::vector<uint32_t>* rasters[]={&frame.background,&frame.foreground,&frame.native};
    const char* names[]={"background","foreground","native"};
    for(int i=0;i<3;i++)trace_raster(names[i],frame.width,frame.height,[&](int x,int y){return rasters[i]->at(size_t(y)*frame.width+x);});
    commit_trace_rasters();trace_pending_raster().clear();
    ++count;next=frame.time+5;
}
template<class Pixel> void copy_pixels(std::vector<uint32_t>& out,int width,int height,Pixel pixel) {
    out.resize(size_t(width)*height);
    for(int y=0;y<height;y++)for(int x=0;x<width;x++)out[size_t(y)*width+x]=uint32_t(pixel(x,y));
}
}
