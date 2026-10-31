// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_scene.h"
#include "game_state.h"
namespace rchase2 {
struct RasterContext {
    bool vr{},requested{},host_ready{},full_frame{},reference{},capture{},direct{},quit{};
    uint64_t sequence{};
    GameState state;
};
inline bool skip_cpu_raster(const RasterContext& c,const NativeFrame* f) {
    if(!c.vr||!c.requested||!c.host_ready||!c.full_frame||c.reference||c.capture||c.direct||c.quit)return false;
    // Mode 11 continuation and service retain the original renderer until
    // independently exercised. The recognized Revision A mode 9 is gameplay.
    if(!c.state.valid||!c.state.playing||c.state.mode!=9||c.state.service)return false;
    if(!f||f->sequence!=c.sequence||f->rejected||f->width<2||f->height<2||f->width>2048||f->height>2048||
       f->background.size()!=size_t(f->width)*f->height||!Model2Resources(*f).valid())return false;
    NativeFrame candidate;candidate.width=f->width;candidate.height=f->height;
    candidate.xoffset=f->xoffset;candidate.yoffset=f->yoffset;candidate.lens=f->lens;
    for(const auto& p:f->polygons) {
        if(p.window!=1||panel_polygon(p))continue;
        candidate.polygons.assign(1,p);
        for(const auto& t:scene_triangles(candidate)) {
            const auto& a=t.vertices[0];const auto& b=t.vertices[1];const auto& d=t.vertices[2];
            const double area=(b.pixel_x-a.pixel_x)*(d.pixel_y-a.pixel_y)-(b.pixel_y-a.pixel_y)*(d.pixel_x-a.pixel_x);
            if(std::isfinite(area)&&std::abs(area)>1e-5)return true;
        }
    }
    return false;
}
}
