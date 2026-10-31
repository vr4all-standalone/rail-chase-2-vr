// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/native_reticle.h"
#include <cassert>
#include "../native/reticle_mode.h"
#include "../native/reticle_size.h"
#include "../native/custom_reticle.h"
#include <iostream>
using namespace rchase2;
#include "native_reticle_fixture.h"
static bool close(double a,double b){return std::abs(a-b)<1e-5;}
static void reticle_mode_tests(){
    auto policy=reticle_draw_policy(ReticleMode::Ingame,true,true,true);
    assert(policy.suppress_native&&policy.draw_native&&!policy.draw_custom);
    policy=reticle_draw_policy(ReticleMode::None,true,true,false);
    assert(policy.suppress_native&&!policy.draw_native&&!policy.draw_custom);
    policy=reticle_draw_policy(ReticleMode::Custom,true,false,true);
    assert(!policy.suppress_native&&!policy.draw_native&&policy.draw_custom);
    for(auto mode:{ReticleMode::Ingame,ReticleMode::Custom,ReticleMode::None}){
        policy=reticle_draw_policy(mode,false,true,true);
        assert(!policy.suppress_native&&!policy.draw_native&&!policy.draw_custom);
        policy=reticle_draw_policy(mode,true,false,false);assert(!policy.suppress_native);
    }
    policy=reticle_draw_policy(ReticleMode(77),true,true,true);
    assert(policy.suppress_native&&policy.draw_native&&!policy.draw_custom);
    const auto mesh=custom_reticle_mesh();
    auto covered=[&](double x,double y,int layer){
        for(size_t i=0;i<mesh.size();i+=3)if(layer<0||mesh[i].layer==layer){
            std::array<Vec3,3> vertices;
            for(unsigned j=0;j<3;j++)vertices[j]={mesh[i+j].x,mesh[i+j].y,-1};
            if(intersect_triangle({{x,y,0},{0,0,-1}},vertices))return true;
        }
        return false;
    };
    for(double x:{-.5,.5})for(double y:{-.5,.5})assert(covered(x,y,2));
    assert(!covered(0,0,-1)&&!covered(0,.5,-1)&&!covered(.5,0,-1));
}
static void reticle_size_tests(){
    // A slower user-selected falloff must visibly retain more artwork at six
    // metres, independently of its minimum angular floor.
    assert(close(reticle_size_reference(6,2,{.05f,4.f}),3));
    assert(close(reticle_size_reference(6,2,{.05f,1.f}),1.2));
    // Defaults shrink monotonically to a 35% perceived-size floor. These
    // hand-derived ratios include both sides of the floor boundary.
    const double distances[]={2,3,4,6,8,16};
    const double ratios[]={1,.861111111111,.756097560976,.607843137255,.508196721311,.35};
    double previous=2;
    for(unsigned i=0;i<6;i++){
        const double perceived=reticle_size_reference(distances[i],2)/distances[i];
        assert(close(perceived,ratios[i])&&perceived<=previous);previous=perceived;
    }
    // Matching falloff to the reference reproduces original constant physical
    // scaling until the floor, without the removed 1.35 near-size multiplier.
    for(double distance:{.5,1.,2.,4.,8.,20.})assert(close(reticle_size_reference(distance,2,{.05f,2.f}),2));
    assert(close(reticle_size_reference(2,2),2));
    assert(close(reticle_size_reference(22,2,{-3,100}),11)); // .05 minimum, 20m falloff.
    assert(close(reticle_size_reference(22,2,{3,-1}),22)); // 100% floor wins.
    assert(close(reticle_size_reference(22,2,{.05f,-1}),1.1)); // .2m falloff.
    assert(close(reticle_size_reference(4,2,{NAN,NAN}),3.024390243902));
    assert(close(reticle_size_reference(4,2,{.5f,INFINITY}),3.024390243902));
    assert(close(reticle_size_reference(NAN,2),2));
    assert(close(reticle_size_reference(4,NAN),0));
}
int main(){
    reticle_mode_tests();reticle_size_tests();
    auto f=fixture();auto r=native_reticles(f);
    assert(r[0].valid()&&r[1].valid());assert(r[0].polygons.size()==9&&r[1].polygons.size()==9);
    assert(std::abs(r[0].center_x-184)<1e-6&&std::abs(r[1].center_x-312)<1e-6&&std::abs(r[0].center_y-224)<1e-6);
    auto reject=[&](auto mutate){auto bad=f;mutate(bad);auto got=native_reticles(bad);assert(!got[0].valid()&&got[1].valid());};
    reject([](auto& b){b.polygons[0].object=123;});
    reject([](auto& b){b.polygons[0].header[3]=256;});
    reject([](auto& b){b.polygons[0].window=2;}); // Partial additional group.
    reject([](auto& b){b.polygons[0].focus[0]=600;});
    reject([](auto& b){b.polygons[0].matrix[0]=1;});
    reject([](auto& b){b.polygons[0].matrix[9]+=.1f;});
    reject([](auto& b){b.polygons[0].vertices[0].z=7;});
    reject([](auto& b){b.polygons[0].culled=true;});
    reject([](auto& b){b.polygons.push_back(b.polygons[0]);});
    reject([](auto& b){b.polygons.erase(b.polygons.begin());});
    for(int mode=0;mode<3;mode++){auto bad=f;if(mode==0)bad.complete=false;if(mode==1)bad.rejected=1;if(mode==2)bad.width=640;auto got=native_reticles(bad);assert(!got[0].valid()&&!got[1].valid());}
    {auto shifted=f;for(auto& p:shifted.polygons)p.window=3;
     auto selected=native_reticles(shifted);assert(selected[0].valid()&&selected[1].valid());
     for(auto p:f.polygons){p.window=2;shifted.polygons.push_back(p);}
     selected=native_reticles(shifted);assert(selected[0].valid()&&selected[0].polygons.size()==9&&selected[0].duplicates.size()==18);
     shifted.polygons.back().vertices[0].x+=.00001f;
     selected=native_reticles(shifted);assert(selected[0].valid()&&!selected[1].valid());}
    f.polygons.erase(f.polygons.begin()+9,f.polygons.end());r=native_reticles(f);assert(r[0].valid()&&!r[1].valid());
    std::cout<<"native reticle signatures, independent players and unknown-source fallback pass\n";
}
