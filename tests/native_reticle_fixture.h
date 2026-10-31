#pragma once
#include "../native/native_reticle.h"
using namespace rchase2;
static NativeFrame fixture(){
    NativeFrame f;f.width=496;f.height=384;f.yoffset=128;
    for(int player=0;player<2;player++)for(int part=0;part<9;part++){
        NativePolygon p;p.object=player?0x835f24:0x835f85;p.window=1;p.luma=255;
        p.attr=part==8?1537:part==6?5378:part%2==0?1282:1025;
        p.culled=p.attr==1025;p.count=p.culled||part==8?4:3;
        p.header=part<7?std::array<unsigned,4>{16384,0,3528,player?256u:7936u}:std::array<unsigned,4>{24585,1,3200,384};
        p.focus={768,768};p.center={248,320};p.viewport={0,128,496,512};
        p.matrix={.4f,0,0,0,.4f,0,0,0,1,player?.5f:-.5f,-.25f,6};
        for(unsigned j=0;j<p.count;j++)p.vertices[j]={float(j*20),float(j%2*30),part==8?6.0098877f:5.99377441f,0,0};
        if(part==7)for(int j=2;j<4;j++)p.vertices[j].z=6.0098877f;
        f.polygons.push_back(p);
    }return f;
}
