// SPDX-License-Identifier: GPL-2.0-or-later
#include "native_reticle_fixture.h"
#include "../native/gl_scene.h"
#include "../native/gl_context.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <iostream>
static PFNGLTEXIMAGE2DPROC real_texture_image;
static unsigned raster_uploads;
static void GLAD_API_PTR observe_texture_image(GLenum target,GLint level,GLint format,GLsizei width,GLsizei height,GLint border,GLenum source,GLenum type,const void* pixels){
    if(width==496&&height==384&&source==GL_RGBA&&type==GL_UNSIGNED_BYTE)++raster_uploads;
    real_texture_image(target,level,format,width,height,border,source,type,pixels);
}
static void resources(NativeFrame& f){
    for(auto pair:{std::pair{"texture0",1048576}, {"texture1",1048576},{"palette",16384},{"colorxlat",49152},{"luma",32768},{"gamma",256}})
        f.blobs.push_back({pair.first,std::vector<uint8_t>(pair.second,255)});
    f.background.assign(496*384,0);f.foreground.assign(496*384,0);f.native.assign(496*384,0xff336699);
}
int main(){
    GLContext context;GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,496,384,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,496,384);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glViewport(0,0,496,384);
    auto f=fixture();f.polygons.resize(9);
    for(auto& p:f.polygons)for(unsigned n=0;n<p.count;n++){
        p.vertices[n].x=-384.f+(n==0?-48.f:n==1?48.f:0.f);
        p.vertices[n].y=-192.f+(n==2?48.f:-24.f);
    }
    resources(f);GLScene scene;
    real_texture_image=glad_glTexImage2D;glad_glTexImage2D=observe_texture_image;raster_uploads=0;
    scene.prepare(std::make_shared<NativeFrame>(f));assert(raster_uploads==2); // Native fallback is unused for a valid GPU scene.
    auto render=[&](Vec3 at,float eye=0.f,float yaw=0.f,bool p2=false){
        auto head=glm::rotate(glm::mat4(1),yaw,glm::vec3(0,1,0));
        scene.set_reticle_targets({at,p2?std::optional<Vec3>{Vec3{.35,0,-2}}:std::nullopt},head);
        glClearColor(0,0,0,1);glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        auto vp=glm::frustum(-248.f/768*.001f,248.f/768*.001f,-192.f/768*.001f,192.f/768*.001f,.001f,1000.f)*glm::translate(glm::mat4(1),glm::vec3(-eye,0,0))*glm::inverse(head);
        scene.draw(vp,.1f,1.8f);glFinish();std::vector<uint8_t> pixels(496*384*4);glReadPixels(0,0,496,384,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);return pixels;
    };
    auto parked_a=render({0,0,-2}),parked_b=render({.2,0,-4});assert(parked_a==parked_b); // User deferred replacement; default must retain native presentation.
    scene.set_native_reticles_enabled(true);
    // Use a fixed falloff here; the calibrated gameplay default can change
    // independently of the GL test's hand-derived area-ratio threshold.
    scene.set_reticle_size(.25f,1.8f);
    auto original=render({NAN,0,-2}),moved=render({0,0,-2});assert(original!=moved);
    auto centroid=[](const auto& image){double x=0,y=0,n=0;for(int row=0;row<384;row++)for(int col=0;col<496;col++){size_t i=(row*496+col)*4;if(image[i]>200&&image[i+1]>200&&image[i+2]>200){x+=col;y+=row;n++;}}assert(n>10);return std::array<double,3>{x/n,y/n,n};};
    auto center=centroid(moved),left=centroid(render({0,0,-2},-.032f)),right=centroid(render({0,0,-2},.032f)),far=centroid(render({0,0,-4}));
    assert(std::abs(center[0]-248)<4&&std::abs(center[1]-192)<4);assert(left[0]-right[0]>20&&left[0]-right[0]<28);
    assert(far[2]<center[2]*.5&&far[2]>center[2]*.1); // Natural shrinking, rather than the parked fixed angular size.
    auto facing=centroid(render({-2*std::sin(.7),0,-2*std::cos(.7)},0,.7f));
    assert(std::abs(facing[0]-center[0])<1&&std::abs(facing[1]-center[1])<1&&std::abs(facing[2]-center[2])<center[2]*.05);
    // None removes only known geometry even without controller targets.
    scene.set_reticle_mode(ReticleMode::None,true);auto none=render({NAN,0,-2});
    assert(none!=original);
    for(auto mode:{ReticleMode::Ingame,ReticleMode::Custom,ReticleMode::None}){
        scene.set_reticle_mode(mode,false);assert(render({0,0,-2})==original);
    }
    auto colored=[](const auto& pixels,unsigned channel){size_t n=0;for(size_t i=0;i<pixels.size();i+=4)
        if(pixels[i+channel]>200&&pixels[i+(channel==0?2:0)]<80&&pixels[i+1]<150)++n;return n;};
    scene.set_reticle_mode(ReticleMode::Custom,true);
    for(float eye:{-.032f,.032f}){
        auto custom=render({0,0,-2},eye,0,true);assert(colored(custom,0)>100&&colored(custom,2)>100);
        int x=int(248-eye*768/2);size_t center_pixel=(192*496+x)*4;
        assert(custom[center_pixel]==0&&custom[center_pixel+1]==0&&custom[center_pixel+2]==0);
    }
    scene.set_reticle_size(.25f,1.8f);auto small=render({0,0,-8});
    scene.set_reticle_size(.75f,1.8f);auto large=render({0,0,-8});assert(colored(large,0)>colored(small,0)*4);
    scene.set_reticle_size(.05f,1.f);small=render({0,0,-6});
    scene.set_reticle_size(.05f,4.f);large=render({0,0,-6});assert(colored(large,0)>colored(small,0)*3);
    scene.set_reticle_mode(ReticleMode::Ingame,true);scene.set_reticle_size(.25f,1.8f);
    auto unknown=f;unknown.polygons[0].object=123;scene.prepare(std::make_shared<NativeFrame>(unknown));auto a=render({0,0,-2}),b=render({.2,0,-4});assert(a==b);
    auto invalid=f;invalid.blobs.clear();raster_uploads=0;scene.prepare(std::make_shared<NativeFrame>(invalid));assert(raster_uploads==1);a=render({0,0,-2});b=render({.2,0,-4});assert(a==b); // Identical BG/FG stay uploaded; native fallback uploads freshly.
    const size_t middle=(192*496+248)*4;assert(a[middle]==0x33&&a[middle+1]==0x66&&a[middle+2]==0x99);
    scene.set_reticle_mode(ReticleMode::Custom,true);auto fallback_custom=render({0,0,-2});
    assert(colored(fallback_custom,0)>100);
    const auto clear_center=render({0,0,-2});assert(clear_center[middle]==0x33&&clear_center[middle+1]==0x66&&clear_center[middle+2]==0x99);
    scene.set_reticle_mode(ReticleMode::Ingame,true);
    // The native reticle is excluded from aim; the backing world remains hittable.
    NativePolygon world;world.count=4;world.window=1;world.focus={768,768};world.center={248,320};world.viewport={0,128,496,512};world.header={192,0,0,0};world.luma=255;
    world.vertices={NativeVertex{-30000,30000,100,0,0},{30000,30000,100,0,0},{30000,-30000,100,0,0},{-30000,-30000,100,0,0}};
    f.polygons.push_back(world);scene.prepare(std::make_shared<NativeFrame>(f));
    auto hit=scene.trace_aim({{0,0,0},{-.0834,-.0417,-1}},.1f,1.8f);assert(hit.target&&std::abs(hit.target->z+10)<1e-4);
    glad_glTexImage2D=real_texture_image;
    std::cout<<"GL native endpoint movement, stereo, distance sizing, fallback and aim exclusion pass\n";
}
