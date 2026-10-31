// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_capture.h"
#include <algorithm>
#include <chrono>
namespace rchase2 {
// Original boot/credits rasters are ready to present before the game permits
// Start. Keep that visual decision separate from native input eligibility.
inline bool native_frame_presentable(const NativeFrame* frame){
    if(!frame||!frame->complete||frame->width<2||frame->height<2||frame->width>2048||frame->height>2048)return false;
    const size_t pixels=size_t(frame->width)*frame->height;
    return frame->native.size()==pixels||frame->background.size()==pixels||frame->foreground.size()==pixels||!frame->polygons.empty();
}
class LaunchPolicy {
    std::chrono::nanoseconds remaining_{std::chrono::seconds(1)};
public:
    bool active()const{return remaining_.count()>0;}
    void observe(const NativeFrame* frame){
        if(!active()||!native_frame_presentable(frame))return;
        const size_t pixels=size_t(frame->width)*frame->height;
        for(const auto* raster:{&frame->native,&frame->background,&frame->foreground})
            if(raster->size()==pixels&&std::any_of(raster->begin(),raster->end(),[](uint32_t pixel){return (pixel&0xffffffu)!=0;})){
                remaining_={};return;
            }
        for(const auto& polygon:frame->polygons)
            if(!polygon.culled&&polygon.count>=3&&polygon.count<=4&&((polygon.header[0]>>13)&3)!=1){remaining_={};return;}
    }
    // The host advances only focused, presented frames; no minimum hold masks
    // original content, and even a delayed frame cannot extend the hard cap.
    void presented(std::chrono::nanoseconds period){if(period.count()>0)remaining_-=std::min(remaining_,period);}
};
}
