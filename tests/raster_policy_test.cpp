#include "../native/raster_policy.h"
#include <cassert>
using namespace rchase2;
int main() {
    NativeFrame frame;frame.sequence=7;frame.width=496;frame.height=384;frame.yoffset=128;
    frame.background.resize(496*384);
    constexpr const char* names[]={"texture0","texture1","palette","colorxlat","luma","gamma"};
    constexpr size_t sizes[]={1048576,1048576,16384,49152,32768,256};
    for(int i=0;i<6;i++)frame.blobs.push_back({names[i],std::vector<uint8_t>(sizes[i])});
    NativePolygon p;p.count=4;p.window=1;p.focus={768,768};p.center={248,320};p.viewport={0,128,496,512};
    p.vertices={NativeVertex{-10000,10000,100,0,0},NativeVertex{10000,10000,100,8,0},NativeVertex{10000,-10000,100,8,8},NativeVertex{-10000,-10000,100,0,8}};
    frame.polygons={p};
    RasterContext good;good.vr=good.requested=good.host_ready=good.full_frame=true;good.sequence=7;
    good.state.valid=good.state.playing=true;good.state.mode=9;
    assert(skip_cpu_raster(good,&frame));
    for(int field=0;field<10;field++) {
        auto c=good;
        switch(field){case 0:c.vr=false;break;case 1:c.requested=false;break;case 2:c.host_ready=false;break;
        case 3:c.full_frame=false;break;case 4:c.reference=true;break;case 5:c.capture=true;break;
        case 6:c.direct=true;break;case 7:c.quit=true;break;case 8:c.state.service=true;break;case 9:c.sequence=8;break;}
        assert(!skip_cpu_raster(c,&frame));
    }
    assert(!skip_cpu_raster(good,nullptr));
    auto c=good;c.state.valid=false;assert(!skip_cpu_raster(c,&frame));
    c=good;c.state.mode=11;assert(!skip_cpu_raster(c,&frame));
    frame.rejected=1;assert(!skip_cpu_raster(good,&frame));frame.rejected=0;
    frame.background.clear();assert(!skip_cpu_raster(good,&frame));frame.background.resize(496*384);
    frame.blobs.back().bytes.clear();assert(!skip_cpu_raster(good,&frame));frame.blobs.back().bytes.resize(256);
    frame.polygons[0].culled=true;assert(!skip_cpu_raster(good,&frame));
    frame.polygons[0]=p;frame.polygons[0].focus={1,1};assert(!skip_cpu_raster(good,&frame));
    frame.polygons[0]=p;for(auto& v:frame.polygons[0].vertices)v.x=v.y=0;assert(!skip_cpu_raster(good,&frame));
    frame.polygons.clear();assert(!skip_cpu_raster(good,&frame));
}
