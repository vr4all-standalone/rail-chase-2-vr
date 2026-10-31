// SPDX-License-Identifier: GPL-2.0-or-later
#include "cart_bottom_fixture.h"
#include "../native/gl_scene.h"
#include "../native/gl_context.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <iostream>
using namespace rchase2;
static PFNGLBUFFERDATAPROC real_buffer_data;
static unsigned cart_mesh_uploads;
static void GLAD_API_PTR observe_buffer_data(GLenum target,GLsizeiptr size,const void* data,GLenum usage){
    if(usage==GL_STATIC_DRAW&&size==96*5*sizeof(float))++cart_mesh_uploads;
    real_buffer_data(target,size,data,usage);
}
int main(){
    GLContext context;GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,496,384,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,496,384);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glViewport(0,0,496,384);
    auto f=cart_fixture();
    for(auto pair:{std::pair{"texture0",1048576},{"texture1",1048576},{"palette",16384},{"colorxlat",49152},{"luma",32768},{"gamma",256}})
        f.blobs.push_back({pair.first,std::vector<uint8_t>(pair.second,255)});
    f.background.assign(496*384,0);f.foreground.assign(496*384,0);f.native.assign(496*384,0xff336699);
    GLScene scene;scene.prepare(std::make_shared<NativeFrame>(f),true);scene.set_presentation_lens({496,384,384,384,248,192});
    real_buffer_data=glad_glBufferData;glad_glBufferData=observe_buffer_data;
    auto render=[&](float yaw=0.f,float scale=.1f,float eye=0.f,float pitch=1.3f,float fov=70.f){
        glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        auto head=glm::rotate(glm::mat4(1),yaw,glm::vec3(0,1,0))*glm::rotate(glm::mat4(1),-pitch,glm::vec3(1,0,0));
        head=glm::translate(glm::mat4(1),glm::vec3(eye,0,0))*head;
        auto vp=glm::perspective(glm::radians(fov),496.f/384,.001f,1000.f)*glm::inverse(head);
        scene.draw(vp,scale,1.8f);glFinish();std::vector<uint8_t> pixels(496*384*4);glReadPixels(0,0,496,384,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);return pixels;
    };
    auto off=render();scene.set_cart_bottom_enabled(false);assert(off==render());
    const auto aim=scene.trace_aim({{0,0,0},{0,-1,-.1}},.1f,1.8f);
    scene.set_cart_bottom_enabled(true);auto on=render();assert(off!=on);
    const size_t middle=(192*496+248)*4;assert(on[middle]>200&&on[middle+1]>200&&on[middle+2]>200);
    assert(render(3.14159265f)[middle]>200); // Rear half must survive native-camera clipping.
    for(float scale:{.01f,.1f,1.f})for(float eye:{-.032f,.032f})assert(render(0,scale,eye)[middle]>200);
    // At a straight-down 110-degree view, the signed rear end of this measured
    // floor stops before row10. Raw native camera mapping would extend twice
    // as far here and paint this pixel, breaking the accepted lens fit.
    auto wide_on=render(0,.1f,0,1.5707963f,110);
    scene.set_cart_bottom_enabled(false);auto wide_off=render(0,.1f,0,1.5707963f,110);
    const size_t outside=(10*496+248)*4;
    assert(wide_on[middle]>200&&wide_off[middle]==0&&wide_on[outside]==wide_off[outside]);
    scene.set_cart_bottom_enabled(true);
    for(float fov:{35.f,95.f}){
        float focal=496/(2*std::tan(glm::radians(fov)*.5f));scene.set_presentation_lens({496,384,focal,focal,248,192});
        assert(render()[middle]>200);
    }
    scene.set_presentation_lens({496,384,384,384,248,192});
    auto same_aim=scene.trace_aim({{0,0,0},{0,-1,-.1}},.1f,1.8f);
    assert(bool(aim.target)==bool(same_aim.target)&&aim.native.u==same_aim.native.u&&aim.native.v==same_aim.native.v);
    auto unknown=f;unknown.polygons[0].object=0;scene.prepare(std::make_shared<NativeFrame>(unknown),true);
    auto unknown_on=render();scene.set_cart_bottom_enabled(false);assert(unknown_on==render());
    scene.prepare(std::make_shared<NativeFrame>(f),true);assert(off==render()); // Toggle back returns original byte-for-byte.
    assert(cart_mesh_uploads==1);glad_glBufferData=real_buffer_data;
    std::cout<<"GL cart On/Off, head-down/rear, stereo, scale, unknown fallback and native aim invariance pass\n";
}
