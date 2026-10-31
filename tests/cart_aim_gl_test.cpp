// SPDX-License-Identifier: GPL-2.0-or-later
#include "cart_bottom_fixture.h"
#include "../native/cart_bottom.h"
#include "../native/gl_scene.h"
#include "../native/gl_context.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace rchase2;
static NativePolygon backing_wall(float depth,unsigned sort=1000000){
    NativePolygon p;p.count=4;p.object=0x123456;p.window=1;p.sort=sort;p.luma=255;
    p.focus={768,768};p.center={248,320};p.viewport={0,128,496,512};p.header={192,0,0,0};
    p.vertices={NativeVertex{-768000,768000,depth,0,0},{768000,768000,depth,0,0},
        {768000,-768000,depth,0,0},{-768000,-768000,depth,0,0}};return p;
}
static std::vector<uint8_t> bytes(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary);assert(in);return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
// Optional private capture input is the existing diagnostic text with the
// twelve matrix values after viewport. Captures/resources are never bundled.
static NativeFrame read_capture(const char* text,const std::filesystem::path& root){
    NativeFrame f;size_t count{};std::ifstream in(text);assert(in);
    in>>f.sequence>>f.width>>f.height>>f.xoffset>>f.yoffset>>count;assert(count<=32768);
    f.polygons.resize(count);
    for(auto& p:f.polygons){
        in>>p.count>>p.attr>>p.window>>p.sort>>p.luma>>p.lod>>p.object>>p.culled;assert(p.count>=3&&p.count<=4);
        for(auto& v:p.header)in>>v;for(auto& v:p.focus)in>>v;for(auto& v:p.center)in>>v;
        for(auto& v:p.viewport)in>>v;for(auto& v:p.matrix)in>>v;
        for(unsigned n=0;n<p.count;n++){auto& v=p.vertices[n];in>>v.x>>v.y>>v.z>>v.u>>v.v;}
    }assert(in);
    for(const char* name:{"texture0","texture1","palette","colorxlat","luma","gamma"})f.blobs.push_back({name,bytes(root/(std::string(name)+".bin"))});
    for(const char* name:{"background","foreground","native"}){
        auto raw=bytes(root/(std::string(name)+"-argb32.bin"));assert(raw.size()==size_t(f.width)*f.height*4);
        auto& pixels=std::string(name)=="background"?f.background:std::string(name)=="foreground"?f.foreground:f.native;
        for(size_t at=0;at<raw.size();at+=4)pixels.push_back(raw[at]|(uint32_t(raw[at+1])<<8)|(uint32_t(raw[at+2])<<16)|(uint32_t(raw[at+3])<<24));
    }return f;
}
static void ppm(const std::string& path,const std::vector<uint8_t>& pixels){
    if(path.empty())return;std::ofstream out(path,std::ios::binary);out<<"P6\n496 384\n255\n";
    for(int y=383;y>=0;y--)for(int x=0;x<496;x++)out.write(reinterpret_cast<const char*>(pixels.data()+(y*496+x)*4),3);assert(out);
}
static void verify(NativeFrame f,const std::string& label,const std::string& output={}){
    assert(cart_bottom_pose(f).valid);
    // Use actual captured cart commands with a known opaque world behind them.
    // This isolates the cart's obstruction from unrelated route geometry.
    std::erase_if(f.polygons,[](const auto& p){return p.object!=0x8363ce;});
    f.polygons.push_back(backing_wall(100));
    const NativeLens lens{496,384,384,384,248,192};f.lens=lens;
    GLScene scene;scene.set_presentation_lens(lens);scene.prepare(std::make_shared<NativeFrame>(f),true);
    auto render=[&](){
        glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        auto vp=glm::perspective(glm::radians(85.f),496.f/384,.001f,100.f)*glm::rotate(glm::mat4(1),.55f,glm::vec3(1,0,0));
        scene.draw(vp,.1f,1.8f);glFinish();std::vector<uint8_t> pixels(496*384*4);
        glReadPixels(0,0,496,384,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);return pixels;
    };
    auto image_off=render();scene.set_cart_bottom_enabled(true);auto image_on=render();
    if(!output.empty()){ppm(output+"-off.ppm",image_off);ppm(output+"-on.ppm",image_on);}
    for(bool immersive:{false,true}){
        scene.set_immersive(immersive);scene.set_cart_bottom_enabled(false);
        Ray ray;SceneAim native;bool found=false;Model2Resources material(f);const auto triangles=scene_triangles(f,true,immersive);
        for(size_t n=13;n<17&&!found;n++){
            const auto& p=f.polygons[n];if(p.culled||!source_front_polygon(p))continue;
            auto positions=world_positions(p,f,.1f,1.8f,immersive);Vec3 center{};
            for(unsigned v=0;v<p.count;v++)center=center+positions[v]*(1./p.count);
            ray={{0,0,0},center};native=trace_prepared_scene_aim(ray,f,triangles,&material,.1f,1.8f,lens,immersive);
            found=native.target&&native.target->z<-.1&&native.target->z>-2;
        }assert(found);
        auto target=scene.trace_aim(ray,.1f,1.8f);
        std::cout<<label<<" immersive="<<immersive<<" original-cart-z="<<native.target->z
            <<" host-target-z="<<(target.target?target.target->z:0)<<std::endl;
        assert(target.target&&std::abs(target.target->z+10)<1e-5); // Known world, through visible rim.
        size_t checked_faces=0;
        for(const auto& triangle:triangles)if(f.polygons[triangle.polygon].object==0x8363ce){
            Vec3 center{};for(const auto& v:triangle.vertices)center=center+
                (immersive?v.camera*.1:lens.unproject(v.pixel_x,v.pixel_y,v.depth*.1))*(1./3.);
            if(!finite(center)||center.z>=-.01||std::abs(center.x/center.z)>1.5||std::abs(center.y/center.z)>1.5)continue;
            Ray face_ray{{0,0,0},center};auto original=trace_prepared_scene_aim(face_ray,f,triangles,&material,.1f,1.8f,lens,immersive);
            if(!original.target||original.target->z<=-2||std::abs(original.target->z+1.8)<1e-5)continue;
            auto through=scene.trace_aim(face_ray,.1f,1.8f);
            if(!through.target||std::abs(through.target->z+10)>=1e-5)std::cerr<<"backing-wall miss: ray="<<center.x<<','<<center.y<<','<<center.z<<" target-z="<<(through.target?through.target->z:0)<<std::endl;
            assert(through.target&&std::abs(through.target->z+10)<1e-5);++checked_faces;
        }
        assert(checked_faces>2);std::cout<<label<<" verified-cart-face-rays="<<checked_faces<<std::endl;
        scene.set_cart_bottom_enabled(true);auto enabled=scene.trace_aim(ray,.1f,1.8f);
        assert(enabled.target&&std::abs(enabled.target->z-target.target->z)<1e-8);
        auto unknown=f;for(auto& p:unknown.polygons)if(p.object==0x8363ce)p.object=999;
        scene.prepare(std::make_shared<NativeFrame>(unknown),true);auto blocking=scene.trace_aim(ray,.1f,1.8f);
        assert(blocking.target&&std::abs(blocking.target->z-native.target->z)<1e-5);
        auto malformed=f;malformed.polygons[0].header[2]++;
        scene.prepare(std::make_shared<NativeFrame>(malformed),true);blocking=scene.trace_aim(ray,.1f,1.8f);
        assert(blocking.target&&std::abs(blocking.target->z-native.target->z)<1e-5);
        auto enemy=f;enemy.polygons.push_back(backing_wall(50,100000));
        scene.prepare(std::make_shared<NativeFrame>(enemy),true);auto ordinary=scene.trace_aim(ray,.1f,1.8f);
        assert(ordinary.target&&std::abs(ordinary.target->z+5)<1e-5);
        auto fallback=f;fallback.blobs.clear();scene.prepare(std::make_shared<NativeFrame>(fallback),true);
        auto panel=scene.trace_aim(ray,.1f,1.8f);assert(panel.target&&std::abs(panel.target->z+1.8)<1e-5);
        scene.prepare(std::make_shared<NativeFrame>(f),true);
    }
    scene.set_immersive(false);scene.set_cart_bottom_enabled(false);assert(image_off==render());
    scene.set_cart_bottom_enabled(true);assert(image_on==render()); // Aim changes never change drawing.
}
int main(int argc,char** argv){
    GLContext context;GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,496,384,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,496,384);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glViewport(0,0,496,384);
    if(argc>=3)verify(read_capture(argv[1],argv[2]),"actual-cart",argc>=4?argv[3]:"");
    else{
        auto f=cart_fixture();for(auto pair:{std::pair{"texture0",1048576},{"texture1",1048576},{"palette",16384},{"colorxlat",49152},{"luma",32768},{"gamma",256}})
            f.blobs.push_back({pair.first,std::vector<uint8_t>(pair.second,255)});
        f.background.assign(496*384,0);f.foreground.assign(496*384,0);f.native.assign(496*384,0xff336699);
        verify(f,"synthetic-cart");
    }
    std::cout<<"Cart aim transparency, independent render toggle, unknown/malformed blockers, ordinary geometry and fallback pass\n";
}
