// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_capture.h"
#include <map>
namespace rchase2 {
struct NativeReticleSource {
    std::vector<size_t> polygons,duplicates;
    double center_x{},center_y{};
    unsigned window{};
    bool valid()const{return !polygons.empty()&&std::isfinite(center_x)&&std::isfinite(center_y);}
};
using NativeReticles=std::array<NativeReticleSource,2>;
// Revision A source lists independently measured in five dual-player gameplay
// frames (35..55 native seconds). Require the complete native command/material
// signature; a matching depth, window or color alone must never hide geometry.
inline NativeReticles native_reticles(const NativeFrame& frame){
    NativeReticles result;
    if(!frame.complete||frame.rejected||frame.width!=496||frame.height!=384||frame.xoffset!=0||frame.yoffset!=128)return result;
    constexpr uint32_t objects[]={0x835f85,0x835f24};
    constexpr unsigned colors[]={7936,256}; // Native P1 blue / P2 warm-color material indices.
    struct Group {NativeReticleSource source;bool invalid{};std::array<unsigned,5> parts{};std::array<float,2> origin{};};
    std::array<std::map<unsigned,Group>,2> groups;
    for(size_t index=0;index<frame.polygons.size();index++){
        const auto& p=frame.polygons[index];
        int player=p.object==objects[0]?0:p.object==objects[1]?1:-1;if(player<0)continue;
        auto& group=groups[player][p.window];auto& source=group.source;auto& bad=group.invalid;const auto& m=p.matrix;
        if(source.polygons.empty()){
            group.origin={m[9],m[10]};source.window=p.window;
            source.center_x=248.+768.*m[9]/6.;source.center_y=192.-768.*m[10]/6.;
        }
        source.polygons.push_back(index);
        const bool bracket=p.header==std::array<unsigned,4>{16384,0,3528,colors[player]};
        const bool textured=p.header==std::array<unsigned,4>{24585,1,3200,384};
        int part=-1;
        if(bracket&&p.attr==1282&&p.count==3&&!p.culled)part=0;
        if(bracket&&p.attr==5378&&p.count==3&&!p.culled)part=1;
        if(bracket&&p.attr==1025&&p.count==4&&p.culled)part=2;
        if(textured&&p.attr==1025&&p.count==4&&p.culled)part=3;
        if(textured&&p.attr==1537&&p.count==4&&!p.culled)part=4;
        if(part<0)bad=true;else ++group.parts[part];
        if(p.luma!=255||p.focus!=std::array<float,2>{768,768}
            ||p.center!=std::array<float,2>{248,320}||p.viewport!=std::array<int,4>{0,128,496,512})bad=true;
        constexpr std::array<float,9> basis{.4f,0,0,0,.4f,0,0,0,1};
        for(size_t n=0;n<m.size();n++)if(!std::isfinite(m[n]))bad=true;
        for(size_t n=0;n<basis.size();n++)if(std::abs(m[n]-basis[n])>1e-6f)bad=true;
        if(std::abs(m[11]-6)>1e-6f||m[9]!=group.origin[0]||m[10]!=group.origin[1])bad=true;
        for(unsigned n=0;n<p.count&&n<4;n++){
            const auto& v=p.vertices[n];
            const float z=part==4||(part==3&&n>=2)?6.0098877f:5.99377441f;
            if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.u)||!std::isfinite(v.v)
                ||std::abs(v.z-z)>.00013f)bad=true;
        }
    }
    auto same=[](const NativePolygon& a,const NativePolygon& b){
        if(a.count!=b.count||a.attr!=b.attr||a.sort!=b.sort||a.luma!=b.luma||a.lod!=b.lod
            ||a.header!=b.header||a.focus!=b.focus||a.center!=b.center||a.viewport!=b.viewport
            ||a.matrix!=b.matrix||a.object!=b.object||a.culled!=b.culled)return false;
        for(unsigned n=0;n<a.count&&n<4;n++){
            const auto& x=a.vertices[n];const auto& y=b.vertices[n];
            if(x.x!=y.x||x.y!=y.y||x.z!=y.z||x.u!=y.u||x.v!=y.v)return false;
        }
        return true;
    };
    // Window indices are submission counters. Select the highest complete RC2
    // source only if every other group is an exact equivalent; no GN tolerance.
    for(unsigned player=0;player<2;player++){
        if(groups[player].empty())continue;
        const auto& top=groups[player].rbegin()->second;bool invalid=false;
        for(const auto& [window,group]:groups[player]){
            if(group.invalid||group.parts!=std::array<unsigned,5>{3,1,3,1,1}){invalid=true;break;}
            if(group.source.polygons.size()!=top.source.polygons.size()){invalid=true;break;}
            for(size_t n=0;n<group.source.polygons.size();n++)
                if(!same(frame.polygons[group.source.polygons[n]],frame.polygons[top.source.polygons[n]]))invalid=true;
        }
        if(invalid)continue;
        auto& source=result[player];source=top.source;
        for(const auto& [window,group]:groups[player])
            source.duplicates.insert(source.duplicates.end(),group.source.polygons.begin(),group.source.polygons.end());
    }
    return result;
}
}
