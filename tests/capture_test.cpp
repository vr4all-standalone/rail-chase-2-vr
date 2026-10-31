#include "../native/model2_capture.h"
#include <iostream>
#include <limits>
#define CHECK(v) do{if(!(v)){std::cerr<<"Failed: " #v " at "<<__LINE__<<'\n';return 1;}}while(0)
int main(){
    using namespace rchase2;
    SceneCapture capture;capture.begin(1,5.0);
    NativePolygon polygon;polygon.count=3;polygon.focus={300,300};polygon.vertices[0]={1,2,3,4,5};polygon.vertices[1]={2,2,3,4,5};polygon.vertices[2]={1,3,3,4,5};
    CHECK(capture.append(polygon));
    std::array<uint8_t,4> texture{1,2,3,4};capture.bytes("texture0",texture.data(),texture.size());
    auto frame=capture.finish();CHECK(frame->polygons.size()==1);CHECK(frame->blobs[0].bytes[0]==1);
    texture[0]=99;polygon.vertices[0].x=99;CHECK(frame->polygons[0].vertices[0].x==1);CHECK(frame->blobs[0].bytes[0]==1);
    capture.begin(2,6);CHECK(capture.finish()->polygons.empty());CHECK(frame->sequence==1);
    capture.begin(3,7);polygon.vertices[0].x=std::numeric_limits<float>::quiet_NaN();CHECK(!capture.append(polygon));
    polygon.vertices[0].x=1;polygon.count=5;CHECK(!capture.append(polygon));
    polygon.count=3;for(unsigned i=0;i<SceneCapture::MaxPolygons;i++)CHECK(capture.append(polygon));CHECK(!capture.append(polygon));
    bool rejected=false;try{capture.bytes("oversize",texture.data(),SceneCapture::MaxBlobBytes+1);}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
    NativeFrame recycled;recycled.width=496;recycled.height=384;recycled.rejected=9;recycled.complete=false;
    recycled.background.resize(496*384);recycled.blobs={{"old",std::vector<uint8_t>(16)},{"unused",std::vector<uint8_t>(32)}};
    recycled.polygons.reserve(200);auto old_bytes=recycled.blobs[0].bytes.data();auto old_polygons=recycled.polygons.data();
    capture.begin(100,9,std::move(recycled));CHECK(capture.append(polygon));capture.bytes("texture0",texture.data(),texture.size());
    auto fresh=capture.take();CHECK(fresh.blobs.size()==1&&fresh.blobs[0].name=="texture0"&&fresh.blobs[0].bytes[0]==99);
    CHECK(fresh.blobs[0].bytes.data()==old_bytes&&fresh.polygons.data()==old_polygons);
    CHECK(fresh.width==0&&fresh.height==0&&fresh.rejected==0&&fresh.complete&&fresh.background.empty());
    std::cout<<"Owned Model 2 scene data, finite vertices and size bounds passed\n";
}
