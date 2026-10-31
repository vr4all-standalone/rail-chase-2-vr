// SPDX-License-Identifier: GPL-2.0-or-later
// Scanline setup follows MAME poly.h/model2_v.cpp (BSD-3-Clause; see
// model2_scene.h). Keep this float operation order: native texture coordinates
// are quantized after interpolation, so algebraically equivalent fans differ.
#pragma once
#include "model2_capture.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace rchase2 {
struct NativeRasterVertex {float x{},y{};std::array<float,3> q{};};
struct NativeScanline {
    std::array<float,3> start{},step{};int start_x{},stop_x{};
    bool valid()const{return start_x<stop_x;}
};
struct NativePolygonRaster {
    int first_y{};
    std::vector<NativeScanline> rows;
    std::vector<NativeRasterVertex> wider_vertices;
};
inline int native_round(float value) {
    const float base=std::floor(value);
    if(base<=float(std::numeric_limits<int>::min()))return std::numeric_limits<int>::min();
    if(base>=float(std::numeric_limits<int>::max()))return std::numeric_limits<int>::max();
    return int(base)+(value-base>.5f);
}
inline std::vector<NativeRasterVertex> native_raster_vertices(const NativeFrame& frame,const NativePolygon& p,bool wider=false) {
    std::vector<NativeVertex> vertices(p.vertices.begin(),p.vertices.begin()+std::min(p.count,4u));
    auto clip=[&](float nx,float ny,float nz,float distance) {
        std::vector<NativeVertex> next;
        for(size_t j=0;j<vertices.size();j++) {
            const auto& a=vertices[j];const auto& b=vertices[(j+1)%vertices.size()];
            const float da=a.x*nx+a.y*ny+a.z*nz,db=b.x*nx+b.y*ny+b.z*nz;
            if(da>=distance)next.push_back(a);
            if((da>=distance)!=(db>=distance)) {
                const float t=(distance-da)/(db-da);
                next.push_back({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,
                    a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t});
            }
        }
        vertices=std::move(next);
    };
    if(wider)clip(0,0,1,.0001f);
    else {
        const float distances[]={p.center[0]-p.viewport[0],p.viewport[2]-p.center[0],
            p.viewport[3]-p.center[1],p.center[1]-p.viewport[1]};
        for(int k=0;k<4;k++) {
            const float length=std::hypot(1.f,distances[k]);
            clip(k==0?1/length:k==1?-1/length:0,k==2?-1/length:k==3?1/length:0,distances[k]/length,0);
        }
    }
    std::vector<NativeRasterVertex> result;
    for(const auto& v:vertices) {
        const float z=v.z+std::numeric_limits<float>::min(),q=1.f/z;
        result.push_back({frame.xoffset+p.center[0]+v.x/z,384-p.center[1]+frame.yoffset-v.y/z,
            {q,v.u*q*.125f,v.v*q*.125f}});
    }
    return result;
}
inline NativePolygonRaster native_polygon_raster(const NativeFrame& frame,const NativePolygon& p) {
    NativePolygonRaster result;
    if(p.culled||p.count<3||p.count>4||((p.header[0]>>13)&3)==1)return result;
    result.wider_vertices=native_raster_vertices(frame,p,true);
    auto v=native_raster_vertices(frame,p);
    if(v.size()<3||v.size()>8)return result;
    int low=0,high=0;
    for(int i=1;i<int(v.size());i++) {
        if(v[i].y<v[low].y)low=i;
        else if(v[i].y>v[high].y)high=i;
    }
    const int top=std::max(0,384-p.viewport[3]+frame.yoffset),bottom=std::min(frame.height-1,384-p.viewport[1]+frame.yoffset);
    const int left=std::max(0,p.viewport[0]+frame.xoffset),right=std::min(frame.width-1,p.viewport[2]+frame.xoffset);
    const int first=std::max(native_round(v[low].y),top),last=std::min(native_round(v[high].y),bottom+1);
    if(first>=last||left>right)return result;
    result.first_y=first;result.rows.resize(last-first);
    if(v.size()==3) {
        auto a=v[0],b=v[1],c=v[2];
        if(b.y<a.y)std::swap(a,b);
        if(c.y<b.y){std::swap(b,c);if(b.y<a.y)std::swap(a,b);}
        const float ab=b.y==a.y?0:(b.x-a.x)/(b.y-a.y),ac=c.y==a.y?0:(c.x-a.x)/(c.y-a.y),bc=c.y==b.y?0:(c.x-b.x)/(c.y-b.y);
        const float a00=b.y-c.y,a01=c.x-b.x,a02=b.x*c.y-c.x*b.y;
        const float a10=c.y-a.y,a11=a.x-c.x,a12=c.x*a.y-a.x*c.y;
        const float a20=a.y-b.y,a21=b.x-a.x,a22=a.x*b.y-b.x*a.y,det=a02+a12+a22;
        std::array<float,3> start{},dx{},dy{};
        for(int k=0;k<3;k++) {
            if(std::abs(det)<.00001f)start[k]=a.q[k];
            else {
                const float inverse=1.f/det;
                dx[k]=inverse*(a.q[k]*a00+b.q[k]*a10+c.q[k]*a20);
                dy[k]=inverse*(a.q[k]*a01+b.q[k]*a11+c.q[k]*a21);
                start[k]=inverse*(a.q[k]*a02+b.q[k]*a12+c.q[k]*a22);
            }
        }
        for(int y=first;y<last;y++) {
            const float fy=float(y)+.5f,sx=a.x+(fy-a.y)*ac,ex=fy<b.y?a.x+(fy-a.y)*ab:b.x+(fy-b.y)*bc;
            int x0=native_round(sx),x1=native_round(ex);if(x0>x1)std::swap(x0,x1);
            auto& row=result.rows[y-first];row.start_x=std::max(x0,left);row.stop_x=std::min(x1,right+1);row.step=dx;
            for(int k=0;k<3;k++)row.start[k]=start[k]+(float(row.start_x)+.5f)*dx[k]+fy*dy[k];
        }
    } else {
        struct Edge {NativeRasterVertex a,b;float dx;std::array<float,3> dq;};
        auto edges=[&](int direction) {
            std::vector<Edge> out;
            for(int i=low;i!=high;) {
                const int j=(i+direction+int(v.size()))%int(v.size());
                if(v[i].y!=v[j].y) {
                    const float inverse=1.f/(v[j].y-v[i].y);
                    Edge edge{v[i],v[j],(v[j].x-v[i].x)*inverse,{}};
                    for(int k=0;k<3;k++)edge.dq[k]=(v[j].q[k]-v[i].q[k])*inverse;
                    out.push_back(edge);
                }
                i=j;
            }
            return out;
        };
        auto le=edges(1),re=edges(-1);if(le.empty()||re.empty())return result;
        const bool shared=le[0].a.x==re[0].a.x&&le[0].a.y==re[0].a.y;
        if((shared&&le[0].dx>re[0].dx)||(!shared&&le[0].a.x>re[0].a.x))std::swap(le,re);
        size_t li=0,ri=0;
        for(int y=first;y<last;y++) {
            const float fy=float(y)+.5f;
            while(li+1<le.size()&&fy>le[li].b.y&&fy<v[high].y)li++;
            while(ri+1<re.size()&&fy>re[ri].b.y&&fy<v[high].y)ri++;
            const auto& l=le[li];const auto& r=re[ri];
            const float ldy=fy-l.a.y,rdy=fy-r.a.y,sx=l.a.x+ldy*l.dx,ex=r.a.x+rdy*r.dx,inverse=1.f/(ex-sx);
            int x0=native_round(sx),x1=native_round(ex);if(x0>x1)std::swap(x0,x1);
            auto& row=result.rows[y-first];row.start_x=std::max(x0,left);row.stop_x=std::min(x1,right+1);
            for(int k=0;k<3;k++) {
                const float lq=l.a.q[k]+ldy*l.dq[k],rq=r.a.q[k]+rdy*r.dq[k];
                row.step[k]=(rq-lq)*inverse;
                row.start[k]=lq+(float(x0)+.5f-sx)*row.step[k];
                if(x0<left)row.start[k]+=(left-x0)*row.step[k];
            }
        }
    }
    return result;
}
inline std::optional<std::array<float,3>> native_polygon_q(const NativePolygonRaster& raster,float x,float y) {
    x=std::floor(x)+.5f;y=std::floor(y)+.5f;
    const int row=int(std::floor(y))-raster.first_y;
    if(row>=0&&row<int(raster.rows.size())&&raster.rows[row].valid()) {
        const auto& r=raster.rows[row];std::array<float,3> q;
        for(int k=0;k<3;k++)q[k]=r.start[k]+(x-(float(r.start_x)+.5f))*r.step[k];
        if(q[0]>0)return q;
    }
    // The native display has no samples beyond its viewport. Keep revealed
    // world surfaces continuous there using the original near-clipped ring.
    float lx=std::numeric_limits<float>::infinity(),rx=-lx;
    std::array<float,3> lq{},rq{};
    const auto& v=raster.wider_vertices;
    for(size_t j=0;j<v.size();j++) {
        auto a=v[j],b=v[(j+1)%v.size()];if(a.y==b.y)continue;
        if(a.y>b.y)std::swap(a,b);
        if(y<a.y||y>b.y)continue;
        const float inverse=1.f/(b.y-a.y),dy=y-a.y,sx=a.x+dy*((b.x-a.x)*inverse);
        std::array<float,3> q;for(int k=0;k<3;k++)q[k]=a.q[k]+dy*((b.q[k]-a.q[k])*inverse);
        if(sx<lx){lx=sx;lq=q;}if(sx>rx){rx=sx;rq=q;}
    }
    if(rx<=lx)return {};
    std::array<float,3> q;for(int k=0;k<3;k++)q[k]=lq[k]+(rq[k]-lq[k])*((x-lx)/(rx-lx));
    return q[0]>0?std::optional(q):std::nullopt;
}
}
