// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_capture.h"
#include <algorithm>
namespace rchase2 {
struct CartBottomPose {bool valid{};std::array<float,12> matrix{};unsigned luma{};int lod{};};
struct CartBottomVertex {float x,y,z,u,v;};
// Revision A cart measured in five native poses, including all culled faces.
// The complete command/material signature and original twelve shell faces
// distinguish this cart from scenery that merely shares its object address.
inline CartBottomPose cart_bottom_pose(const NativeFrame& frame){
    CartBottomPose result;
    if(!frame.complete||frame.rejected||frame.width!=496||frame.height!=384||frame.xoffset||frame.yoffset!=128)return result;
    constexpr float bounds[12][4]={
        {-6.8f,-6.8f,-9.4f,9.4f},{-6.8f,-6.8f,9.4f,9.8f},{-6.8f,-6.4f,9.8f,9.8f},
        {-6.4f,6.4f,9.8f,9.8f},{6.4f,6.8f,9.8f,9.8f},{6.8f,6.8f,9.4f,9.8f},
        {6.8f,6.8f,-9.4f,9.4f},{6.8f,6.8f,-9.8f,-9.4f},{6.4f,6.8f,-9.8f,-9.8f},
        {-6.4f,6.4f,-9.8f,-9.8f},{-6.8f,-6.4f,-9.8f,-9.8f},{-6.8f,-6.8f,-9.8f,-9.4f}};
    unsigned count=0;uint32_t signature=2166136261u;
    for(const auto& p:frame.polygons)if(p.object==0x8363ce){
        if(count>=129||p.count<3||p.count>4||p.window!=1||p.focus!=std::array<float,2>{768,768}
            ||p.center!=std::array<float,2>{248,320}||p.viewport!=std::array<int,4>{0,128,496,512})return {};
        for(float value:p.matrix)if(!std::isfinite(value))return {};
        if(!count){
            result.matrix=p.matrix;result.luma=p.luma;result.lod=p.lod;
            // Measured cart matrices are rigid rotations. Their transpose is
            // the inverse; reject scaled/singular transforms rather than guess.
            for(int a=0;a<3;a++)for(int b=0;b<3;b++){
                double product=0;for(int n=0;n<3;n++)product+=double(p.matrix[a*3+n])*p.matrix[b*3+n];
                if(std::abs(product-(a==b?1.:0.))>.002)return {};
            }
        }else if(p.matrix!=result.matrix)return {};
        auto hash=[&](uint32_t value){signature=(signature^value)*16777619u;};
        hash(p.count);hash(p.attr);for(auto value:p.header)hash(value);
        Vec3 lo{1e9,1e9,1e9},hi{-1e9,-1e9,-1e9};
        for(unsigned n=0;n<p.count;n++){
            const auto& v=p.vertices[n];
            if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.u)||!std::isfinite(v.v))return {};
            if(count<12){
                const auto& m=result.matrix;Vec3 q{v.x/768.-m[9],v.y/768.-m[10],v.z-m[11]};
                Vec3 local{q.x*m[0]+q.y*m[1]+q.z*m[2],q.x*m[3]+q.y*m[4]+q.z*m[5],q.x*m[6]+q.y*m[7]+q.z*m[8]};
                lo={std::min(lo.x,local.x),std::min(lo.y,local.y),std::min(lo.z,local.z)};
                hi={std::max(hi.x,local.x),std::max(hi.y,local.y),std::max(hi.z,local.z)};
            }
        }
        if(count<12){
            const double measured[]={lo.x,hi.x,lo.z,hi.z,lo.y,hi.y};
            const double expected[]={bounds[count][0],bounds[count][1],bounds[count][2],bounds[count][3],9.5,12.};
            for(int n=0;n<6;n++)if(std::abs(measured[n]-expected[n])>.003)return {};
        }
        ++count;
    }
    result.valid=count==129&&signature==0x3aa5457cu;
    return result;
}
inline Vec3 cart_bottom_world(Vec3 local,const CartBottomPose& pose,const NativeFrame& frame,const NativeLens& lens,float scale){
    if(!pose.valid||!lens.valid()||!finite(local)||!std::isfinite(scale)||scale<=0)return {NAN,NAN,NAN};
    const auto& m=pose.matrix;
    Vec3 camera{local.x*m[0]+local.y*m[3]+local.z*m[6]+m[9],local.x*m[1]+local.y*m[4]+local.z*m[7]+m[10],local.x*m[2]+local.y*m[5]+local.z*m[8]+m[11]};
    // Linear homogeneous equivalent of native projection then presentation
    // unprojection, retaining signed camera Z for head-down/back viewing.
    return {(768*camera.x+(frame.xoffset+248-lens.cx)*camera.z)/lens.fx*scale,
        (768*camera.y+(lens.cy-(384-320+frame.yoffset))*camera.z)/lens.fy*scale,-camera.z*scale};
}
inline const std::array<CartBottomVertex,96>& cart_bottom_mesh(){
    static const auto mesh=[](){
        std::array<CartBottomVertex,96> vertices{};size_t at=0;
        constexpr std::array<std::array<float,2>,8> ring{{{-6.81f,-9.41f},{-6.41f,-9.81f},{6.41f,-9.81f},{6.81f,-9.41f},
            {6.81f,9.41f},{6.41f,9.81f},{-6.41f,9.81f},{-6.81f,9.41f}}};
        // Keep the physically accepted floor UVs, but recess its surface below
        // the original shell. The eight side faces continue that shell down
        // to the closed base rather than leave a shallow horizontal platform.
        auto vertex=[](float x,float y,float z){return CartBottomVertex{x,y,z,(z+9.8f)*6.3f,x*34.25f};};
        // Original rim Y12 to floor Y1.5 gives10.5units of interior height,
        // 50% more than the accepted seven-unit recessed interior.
        constexpr float floor_y=1.5f,underside_y=1.3f,join_y=9.51f;
        // The long wall edges retain the measured timber grain direction and
        // U endpoints. Native wall bottom V is1024/8=128; continue down from it.
        constexpr std::array<std::array<float,2>,8> wall_u{{{0,3.56f},{7,120.875f},{0,3.56f},{122.875f,5},
            {0,3.56f},{120.875f,7},{0,3.56f},{5,122.875f}}};
        auto wall=[](float x,float y,float z,float u){return CartBottomVertex{x,y,z,u,128+(9.5f-y)*34.25f};};
        for(size_t n=0;n<ring.size();n++){
            const auto& a=ring[n];const auto& b=ring[(n+1)%ring.size()];
            auto top_a=vertex(a[0],floor_y,a[1]),top_b=vertex(b[0],floor_y,b[1]);
            auto low_a=vertex(a[0],underside_y,a[1]),low_b=vertex(b[0],underside_y,b[1]);
            auto join_a=wall(a[0],join_y,a[1],wall_u[n][0]),join_b=wall(b[0],join_y,b[1],wall_u[n][1]);
            auto wall_a=wall(a[0],underside_y,a[1],wall_u[n][0]),wall_b=wall(b[0],underside_y,b[1],wall_u[n][1]);
            for(auto v:{vertex(0,floor_y,0),top_a,top_b,vertex(0,underside_y,0),low_b,low_a,join_a,wall_a,wall_b,join_a,wall_b,join_b})vertices[at++]=v;
        }return vertices;
    }();return mesh;
}
}
