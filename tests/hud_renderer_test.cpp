#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/gl_scene.h"
#include "../native/gl_context.h"
#include "../native/model2_scene.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <iostream>
using namespace rchase2;
static void resources(NativeFrame& f) {
    f.blobs.clear();
    f.blobs.push_back({"texture0",std::vector<uint8_t>(2097152)});
    f.blobs.push_back({"texture1",std::vector<uint8_t>(2097152)});
    f.blobs.push_back({"palette",std::vector<uint8_t>(16384)});
    f.blobs.push_back({"colorxlat",std::vector<uint8_t>(49152)});
    f.blobs.push_back({"luma",std::vector<uint8_t>(32768)});
    f.blobs.push_back({"gamma",std::vector<uint8_t>(256)});
    unsigned base=5|(10<<5)|(15<<10);f.blobs[2].bytes[8192]=base;f.blobs[2].bytes[8193]=base>>8;
    for(unsigned c=0;c<3;c++)for(unsigned component=0;component<32;component++)for(unsigned luma=0;luma<64;luma++)
        f.blobs[3].bytes[2*(c*8192+component*256+luma)]=component*4+luma;
    for(unsigned i=0;i<32768;i++)f.blobs[4].bytes[i]=(i&127)/2;
    for(unsigned i=0;i<256;i++)f.blobs[5].bytes[i]=255-i;
}

int main(){
 GLContext context;GLScene scene;GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,496,384,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,496,384);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glViewport(0,0,496,384);
 NativeFrame f;f.width=496;f.height=384;f.yoffset=127;f.lens={496,384,400,400,248,191};f.complete=true;resources(f);f.background.assign(496*384,0xff000000);f.foreground.assign(496*384,0);for(int y=181;y<201;y++)for(int x=158;x<178;x++)f.foreground[y*496+x]=0xff00ff00;
 auto marker=[](float x,float y,bool panel){NativePolygon p;p.count=4;p.center={248,320};p.viewport={0,128,496,512};p.focus=panel?std::array<float,2>{1,1}:std::array<float,2>{400,400};p.luma=252;const float xy[4][2]={{x-10,y-10},{x+10,y-10},{x+10,y+10},{x-10,y+10}};for(int j=0;j<4;j++)p.vertices[j]={float((xy[j][0]-248)*200),float((191-xy[j][1])*200),200,float(j==1||j==2?8:0),float(j>=2?8:0)};return p;};
 f.polygons={marker(248,250,false)};
 f.polygons.push_back(marker(328,191,true));
 auto image=std::make_shared<NativeFrame>(f);scene.set_immersive(true);scene.prepare(image,true);
 const float n=.02f;auto vp=glm::frustum(-248.f/400*n,248.f/400*n,-193.f/400*n,191.f/400*n,n,1000.f);
 auto capture=[&](float size,bool game=true,bool service=false){scene.set_hud_size(size,game,service);glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(vp,.01f,2.f);std::vector<uint8_t> p(496*384*4);glReadPixels(0,0,496,384,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;};
 auto count=[&](const auto& p,bool green,bool panel){int count=0;for(int y=0;y<384;y++)for(int x=0;x<496;x++){auto i=(y*496+x)*4;if(green){if(p[i+1]>240&&p[i]<5&&p[i+2]<5)++count;}else if(y>170&&y<215&&x>250&&p[i]>20&&p[i+1]>20&&p[i+2]>20)++count;}return count;};
 scene.set_reticle_mode(ReticleMode::Custom,true);scene.set_reticle_targets({Vec3{0,-.5,-2},std::nullopt},glm::mat4(1));
 auto base=capture(1);assert(base==capture(1));auto a=capture(.5f),z=capture(2);assert(count(base,true,false)>100);assert(count(a,true,false)>count(base,true,false)*.20&&count(a,true,false)<count(base,true,false)*.30);assert(count(z,true,false)>count(base,true,false)*3.8&&count(z,true,false)<count(base,true,false)*4.2);
 assert(count(base,false,true)>100);assert(count(a,false,true)>count(base,false,true)*.20&&count(a,false,true)<count(base,false,true)*.30);assert(count(z,false,true)>count(base,false,true)*3.8&&count(z,false,true)<count(base,false,true)*4.2);
 for(size_t i=0;i<base.size();i+=4){auto red=[](const auto& p,size_t i){return p[i]>240&&p[i+1]<100&&p[i+2]<100;};assert(red(base,i)==red(a,i)&&red(base,i)==red(z,i));}
 // The unchanged world has a different vertical region from all native UI.
 for(int y=110;y<145;y++)for(int x=215;x<280;x++)for(int c=0;c<4;c++){auto i=(y*496+x)*4+c;assert(base[i]==a[i]&&base[i]==z[i]);}
 assert(base==capture(2,false));assert(base==capture(2,true,true));scene.set_immersive(false);auto flat=capture(1);assert(flat==capture(2));scene.set_immersive(true);
 scene.prepare(image,false);auto original=capture(1);assert(original==capture(2));scene.prepare(image,true);assert(base==capture(1));
 auto rejected=std::make_shared<NativeFrame>(f);rejected->blobs.clear();rejected->native.assign(496*384,0xff123456);scene.prepare(rejected,true);auto fallback=capture(1);assert(fallback==capture(2));assert(glGetError()==GL_NO_ERROR);
 std::cout<<"HUD GL identity, 50/200%, world independence, service/title/original/fallback gates pass\n";
}
