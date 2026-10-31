// SPDX-License-Identifier: GPL-2.0-or-later
// Observe real driver uploads: removing the byte/shape/eligibility checks must fail.
#include "native_reticle_fixture.h"
#include "../native/gl_scene.h"
#include "../native/gl_context.h"
#include "../native/frame_pool.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>

struct Upload {GLuint texture;GLint format;GLsizei width,height;std::vector<uint8_t> bytes;};
static PFNGLTEXIMAGE2DPROC real_upload;
static std::vector<Upload> uploads;
static unsigned atlas_uploads;
static void GLAD_API_PTR observe(GLenum target,GLint level,GLint format,GLsizei width,GLsizei height,GLint border,GLenum source,GLenum type,const void* pixels){
    if(format==GL_RGBA32F)++atlas_uploads;
    if(pixels&&(format==GL_R8UI||format==GL_RGBA8)){
        GLint texture{};glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
        size_t size=size_t(width)*height*(format==GL_R8UI?1:4);const auto* first=static_cast<const uint8_t*>(pixels);
        uploads.push_back({GLuint(texture),format,width,height,{first,first+size}});
    }
    real_upload(target,level,format,width,height,border,source,type,pixels);
}
static void clear_uploads(){uploads.clear();atlas_uploads=0;}
static size_t count(GLint format){return std::count_if(uploads.begin(),uploads.end(),[&](const auto& u){return u.format==format;});}
static NativeFrame frame_fixture(){
    auto f=fixture();f.polygons.resize(9);
    for(auto& p:f.polygons)for(unsigned n=0;n<p.count;n++){
        p.vertices[n].x=-384.f+(n==0?-48.f:n==1?48.f:0.f);p.vertices[n].y=-192.f+(n==2?48.f:-24.f);
    }
    for(auto pair:{std::pair{"texture0",1048576},{"texture1",1048576},{"palette",16384},{"colorxlat",49152},{"luma",32768},{"gamma",256}})
        f.blobs.push_back({pair.first,std::vector<uint8_t>(pair.second,255)});
    f.background.assign(496*384,0x00112233);f.foreground.assign(496*384,0);f.native.assign(496*384,0xff336699);return f;
}
static std::vector<uint8_t> render(GLScene& scene,float eye=0){
    glViewport(0,0,496,384);glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    auto vp=glm::frustum(-248.f/768*.001f,248.f/768*.001f,-192.f/768*.001f,192.f/768*.001f,.001f,1000.f)*glm::translate(glm::mat4(1),glm::vec3(-eye,0,0));
    scene.draw(vp,.1f,1.8f);glFinish();std::vector<uint8_t> pixels(496*384*4);glReadPixels(0,0,496,384,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);return pixels;
}
static void equal_fresh(GLScene& cached,const NativeFrame& frame,bool wider=false){
    const auto cached_left=render(cached,-.032f),cached_right=render(cached,.032f);
    GLScene fresh;fresh.prepare(std::make_shared<NativeFrame>(frame),wider);
    assert(cached_left==render(fresh,-.032f)&&cached_right==render(fresh,.032f));
}
int main(){
    GLContext context;GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,496,384,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,496,384);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    real_upload=glad_glTexImage2D;glad_glTexImage2D=observe;
    {
        auto f=frame_fixture();GLScene scene;clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));
        assert(count(GL_R8UI)==6&&count(GL_RGBA8)==2&&atlas_uploads==1);
        std::array<GLuint,6> resource_ids{};for(size_t i=0;i<6;i++)resource_ids[i]=uploads[i].texture;
        clear_uploads();++f.sequence;scene.prepare(std::make_shared<NativeFrame>(f));
        assert(count(GL_R8UI)==0&&count(GL_RGBA8)==0&&atlas_uploads==1); // Distinct immutable allocation, identical bytes.
        equal_fresh(scene,f);
        for(size_t bank=0;bank<6;bank++)for(size_t offset:{size_t(0),f.blobs[bank].bytes.size()/2,f.blobs[bank].bytes.size()-1}){
            f.blobs[bank].bytes[offset]^=1;clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));
            assert(count(GL_R8UI)==1&&count(GL_RGBA8)==0);assert(uploads[0].texture==resource_ids[bank]);
            assert(uploads[0].bytes[offset]==f.blobs[bank].bytes[offset]);
            for(size_t n=f.blobs[bank].bytes.size();n<uploads[0].bytes.size();n++)assert(uploads[0].bytes[n]==0);
            equal_fresh(scene,f);
        }
        std::reverse(f.blobs.begin(),f.blobs.end());clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));assert(uploads.empty());
        f.blobs[0].bytes.push_back(0);clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));assert(count(GL_R8UI)==1&&count(GL_RGBA8)==0);
        f.background[0]^=1;clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));assert(count(GL_RGBA8)==1&&count(GL_R8UI)==0);assert(uploads[0].bytes[2]==uint8_t(f.background[0]));equal_fresh(scene,f);
        f.foreground[10]=0xffaabbcc;clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));assert(count(GL_RGBA8)==1&&uploads[0].bytes[10*4+3]==255);equal_fresh(scene,f);
        clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f),true);assert(uploads.empty()&&atlas_uploads==1);equal_fresh(scene,f,true);
        auto fallback=f;fallback.blobs.clear();clear_uploads();scene.prepare(std::make_shared<NativeFrame>(fallback));assert(count(GL_RGBA8)==1&&count(GL_R8UI)==0);equal_fresh(scene,fallback);
        clear_uploads();scene.prepare(std::make_shared<NativeFrame>(fallback));assert(uploads.empty()&&atlas_uploads==0);
        clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));assert(count(GL_R8UI)==6&&count(GL_RGBA8)==0);
        // Native bytes changed in a usable frame were not uploaded, so the next fallback must upload them.
        f.native[0]^=0x00010203;scene.prepare(std::make_shared<NativeFrame>(f));fallback=f;fallback.blobs.clear();clear_uploads();scene.prepare(std::make_shared<NativeFrame>(fallback));assert(count(GL_RGBA8)==1);equal_fresh(scene,fallback);
        auto reshaped=fallback;std::swap(reshaped.width,reshaped.height);clear_uploads();scene.prepare(std::make_shared<NativeFrame>(reshaped));assert(count(GL_RGBA8)==3);for(const auto& u:uploads)assert(u.width==384&&u.height==496);
        scene.prepare({});clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));assert(count(GL_R8UI)==6&&count(GL_RGBA8)==2);
        auto incomplete=f;incomplete.complete=false;scene.prepare(std::make_shared<NativeFrame>(incomplete));clear_uploads();scene.prepare(std::make_shared<NativeFrame>(f));assert(count(GL_R8UI)==6&&count(GL_RGBA8)==2);
    }
    {
        NativeFramePool pool;GLScene scene;auto published=pool.publish(std::make_unique<NativeFrame>(frame_fixture()));std::weak_ptr<const NativeFrame> old=published;
        scene.prepare(published);published.reset();assert(!old.expired());
        auto replacement=frame_fixture();replacement.background[0]^=1;scene.prepare(pool.publish(std::make_unique<NativeFrame>(replacement)));assert(old.expired());
        auto reused=pool.acquire();reused.background[0]^=2;clear_uploads();scene.prepare(pool.publish(std::make_unique<NativeFrame>(std::move(reused))));assert(count(GL_RGBA8)==1); // Recycled allocation is compared by content.
    }
    glad_glTexImage2D=real_upload;glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&color);glDeleteRenderbuffers(1,&depth);
    std::cout<<"Immutable texture upload reuse, exact mutation payloads, eligibility transitions, dimensions, pool lifetime and fresh-scene pixel equality pass\n";
}
