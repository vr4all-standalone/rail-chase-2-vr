// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG // These executable regression checks must also run in Release.
#endif
#include "model2_scene.h"
#include <cassert>
#include <iostream>
#ifdef RCHASE2_RENDERER_GL_TEST
#include "gl_scene.h"
#include "gl_context.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
static PFNGLDRAWARRAYSPROC original_draw_arrays;
static unsigned observed_draw_calls,observed_world_draw_calls;
static void GLAD_API_PTR observe_draw_arrays(GLenum mode,GLint first,GLsizei count) {
    GLint current{};glGetIntegerv(GL_CURRENT_PROGRAM,&current);
    if(glGetUniformLocation(GLuint(current),"source_scanlines")>=0)++observed_world_draw_calls;
    ++observed_draw_calls;original_draw_arrays(mode,first,count);
}
#endif

using namespace rchase2;
static bool close(double a,double b){return std::abs(a-b)<1e-5;}
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
static NativeFrame frame() {
    NativeFrame f;f.width=496;f.height=384;f.yoffset=127;
    f.lens={496,384,400.,400.,248,191};f.complete=true;resources(f);return f;
}
static NativePolygon quad(float depth=100) {
    NativePolygon p;p.count=4;p.focus={1200,1200};p.center={248,320};p.viewport={0,128,496,512};
    p.header={0,0,0,0};p.luma=252;
    p.vertices={NativeVertex{-10000,10000,depth,0,0},NativeVertex{10000,10000,depth,8,0},
        NativeVertex{10000,-10000,depth,8,8},NativeVertex{-10000,-10000,depth,0,8}};return p;
}
static NativePolygon camera_quad(double yaw_degrees=0,double distance=2,double half_width=.25) {
    auto p=quad();p.focus={400,400};
    const double yaw=yaw_degrees*3.14159265358979323846/180,s=std::sin(yaw),c=std::cos(yaw);
    const double xs[]={-half_width,half_width,half_width,-half_width},ys[]={half_width,half_width,-half_width,-half_width};
    for(unsigned i=0;i<4;i++) {
        // The fixture is a physical square facing the specified gaze direction.
        const double x=s*distance+c*xs[i],z=-c*distance+s*xs[i];
        p.vertices[i]={float(x*40000),float(ys[i]*40000),float(-z*100),float(i==1||i==2?64:0),float(i>=2?64:0)};
    }
    return p;
}
static void immersive_geometry_tests() {
    auto f=frame();auto p=quad();
    // Captured X/Y already contain this polygon's focus. Recover camera units
    // before applying the uniform world scale, independent of the frame lens.
    auto positions=world_positions(p,f,.01f,2.f,true);
    assert(close(positions[0].x,-1./12)&&close(positions[0].y,1./12)&&close(positions[0].z,-1));
    auto world=camera_quad(),reticle=world;reticle.focus={600,600};f.polygons={reticle,reticle,world};
    const auto lens=native_camera_lens(f);
    assert(close(lens.fx,400)&&close(lens.fy,400)&&close(lens.cx,248)&&close(lens.cy,191));
    f.polygons.clear();assert(close(native_camera_lens(f).fx,400));
    for(double yaw:{-60.,60.,90.,180.}) {
        p=camera_quad(yaw);f.polygons={p};
        auto triangles=scene_triangles(f,false,true);assert(triangles.size()==2);
        const double angle=yaw*3.14159265358979323846/180;
        auto hit=trace_scene_aim({{0,0,0},{std::sin(angle),0,-std::cos(angle)}},f,.01f,3.f,false,nullptr,true);
        assert(hit.target&&close(hit.target->x,2*std::sin(angle))&&close(hit.target->z,-2*std::cos(angle)));
        if(yaw==180)assert(scene_triangles(f,true).empty()&&!hit.native.valid);
    }
    // A world subwindow cannot restore the original screen crop in immersive
    // mode; panel primitives retain their native clip and diagnostic behavior.
    p=camera_quad();p.viewport={248,128,496,512};f.polygons={p};
    auto triangles=scene_triangles(f,false,true);assert(triangles.size()==2);
    assert(triangles[0].vertices[0].pixel_x<248);
    p.culled=true;f.polygons={p};assert(scene_triangles(f,true,true).empty());
    // RailChase2 currently captures only a cull boolean: its rejected primitives
    // stay suppressed until this game's culling provenance is actually audited.
    p=camera_quad();f.polygons={p};
    auto hit=trace_scene_aim({{.032,0,0},{0,0,-1}},f,.01f,3.f,false,nullptr,true);
    assert(hit.target&&close(hit.target->x,.032)&&close(hit.target->z,-2));
    auto far=camera_quad(0,4);far.window=2;f.polygons={p,far};
    hit=trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,3.f,false,nullptr,true);
    assert(hit.target&&close(hit.target->z,-4)); // Preserve native window priority.
    // Signed geometry must also retain material occupancy instead of losing all
    // textured fragments because the native positive-Z scanline ring is empty.
    p=camera_quad(180);p.header[0]=0x6000;p.lod=10000;f.polygons={p};
    for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1")std::fill(blob.bytes.begin(),blob.bytes.end(),0x22);
    hit=trace_scene_aim({{0,0,0},{0,0,1}},f,.01f,3.f,false,nullptr,true);assert(hit.target&&close(hit.target->z,2));
    for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1")std::fill(blob.bytes.begin(),blob.bytes.end(),0xff);
    assert(!trace_scene_aim({{0,0,0},{0,0,1}},f,.01f,3.f,false,nullptr,true).target);
}
int main() {
    immersive_geometry_tests();
    auto f=frame();auto p=quad();
    auto model2b=f;
    model2b.blobs[0].bytes.resize(1048576);model2b.blobs[1].bytes.resize(1048576);
    assert(Model2Resources(model2b).valid()); // Actual Model 2B mapped bank sizes.
    // A focus/window change must preserve its native pixels under one common VR lens.
    auto vertices=world_positions(p,f,.01f,2.f);
    auto a=presentation_lens(f).project_point(vertices[0]);
    assert(close(a.u,148./495)&&close(a.v,91./383));
    assert(close(vertices[0].z,-1));
    f.polygons={p};
    auto hit=trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,2.f);
    assert(hit.target&&close(hit.target->z,-1));
    assert(hit.native.valid&&hit.native.inside&&close(hit.native.u,248./495));
    // Shared triangle clipping cannot show or aim through a native viewport border.
    f.polygons[0].viewport={248,128,496,512};
    auto clipped=scene_triangles(f,false);assert(!clipped.empty());
    for(const auto& t:clipped)for(const auto& v:t.vertices)assert(v.pixel_x>=248-1e-5);
    for(const auto& t:scene_triangles(f,true))for(const auto& v:t.vertices)assert(v.pixel_x>=248-1e-5);
    f.polygons[0].culled=true;assert(scene_triangles(f,true).empty());
    // Offscreen native geometry becomes visible only when the requested wider view is enabled.
    f.polygons={p};f.polygons[0].vertices[0].x=-100000;
    auto normal=scene_triangles(f,false),wide=scene_triangles(f,true);
    bool outside=false;for(const auto& t:wide)for(const auto& v:t.vertices)outside|=v.pixel_x<0;
    assert(outside);for(const auto& t:normal)for(const auto& v:t.vertices)assert(v.pixel_x>=0);
    // Native override order, rather than a conventional physical Z buffer, selects the visible surface.
    auto near=quad(50),far=quad(150);near.sort=10;far.sort=20;far.window=2;
    f.polygons={near,far};hit=trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,2.f);
    assert(hit.target&&close(hit.target->z,-1.5));
    // Screen-space direct polygons remain at the selected panel distance.
    p.focus={1,1};f.polygons={p};vertices=world_positions(p,f,.01f,3.f);
    for(auto v:vertices)assert(close(v.z,-3));
    // No geometry, including boot/service screens, still has a usable native aiming panel.
    f.polygons.clear();hit=trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,2.f);
    assert(hit.target&&close(hit.target->z,-2)&&hit.native.valid);
    // Model 2 packs four nibble texels per little-endian word in 2x2 blocks.
    f.blobs[0].bytes[0]=0x34;f.blobs[0].bytes[1]=0x12;Model2Resources material(f);
    assert(material.texel(0,0,0,0,0)==1);assert(material.texel(0,0,0,1,0)==2);
    assert(material.texel(0,0,0,0,1)==3);assert(material.texel(0,0,0,1,1)==4);
    // Transparent solid polygons do not hide world geometry or consume an aim hit.
    p=quad();p.header[0]=0x2000;f.polygons={p};
    assert(scene_triangles(f,false).empty());
    // These literals independently exercise palette -> color translator -> gamma.
    p.header[0]=0;assert(material.sample(p,0,0,100,248,191)==0xffac9884u);
    std::fill(f.blobs[0].bytes.begin(),f.blobs[0].bytes.end(),0x22);
    std::fill(f.blobs[1].bytes.begin(),f.blobs[1].bytes.end(),0x22);
    p.header[0]=0x4000;p.lod=10000;
    assert(material.sample(p,1,1,100,248,191)==0xffe4d0bcu);
    p.header[0]=0xc000;assert(!material.sample(p,1,1,100,249,191));
    // Alpha key 15 must expose the panel, not hide it with a colored polygon.
    std::fill(f.blobs[0].bytes.begin(),f.blobs[0].bytes.end(),0xff);
    std::fill(f.blobs[1].bytes.begin(),f.blobs[1].bytes.end(),0xff);
    p.header[0]=0x6000;f.polygons={p};assert(!material.sample(p,1,1,100,248,191));
    hit=trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,2.f);assert(hit.target&&close(hit.target->z,-2));
    // A comfortable display FOV changes presentation, while its aim still maps
    // to the original native pixel. Native camera focus is not relabeled.
    f=frame();f.polygons={quad()};auto display_lens=f.lens;display_lens.fx=display_lens.fy=390;
    hit=trace_scene_aim({{.032,0,0},{0,0,-1}},f,.01f,2.f,false,&display_lens);
    assert(hit.target&&close(hit.target->z,-1)&&close(hit.native.u,(248+390*.032)/495));
    // Checker coverage uses floor, including negative native coordinates exposed
    // by wider view. Truncating -.5 to zero would select the opposite pixel.
    p=quad();p.header[0]=0x8000;for(unsigned i=0;i<4;i++)p.vertices[i].x*=10;
    f.polygons={p};hit=trace_scene_aim({{0,0,0},{(-.5-248)/f.lens.fx,0,-1}},f,.01f,2.f,true);
    assert(hit.target&&close(hit.target->z,-2));
    // Incomplete frames are never shown and must not produce an actionable aim.
    f.complete=false;assert(!trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,2.f).target);
    // A missing resource causes the renderer's native panel fallback; aim follows it.
    f.complete=true;f.blobs.clear();f.polygons={quad()};
    hit=trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,2.f);assert(hit.target&&close(hit.target->z,-2));
    // A transparent quad's source scanline texel, rather than its triangulation,
    // determines whether a real 3D aim hit reaches the surface or the panel.
    f=frame();p=quad();p.header[0]=0x6000;p.lod=10000;
    p.vertices[0].u=0;p.vertices[0].v=0;p.vertices[1].u=64;p.vertices[1].v=0;
    p.vertices[2].u=640;p.vertices[2].v=640;p.vertices[3].u=0;p.vertices[3].v=64;
    f.polygons={p};
    auto source=native_polygon_raster(f,p);auto q=native_polygon_q(source,248.5f,191.5f);
    assert(q&&close((*q)[1]/(*q)[0],22.20045)&&close((*q)[2]/(*q)[0],22.20045));
    for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1") {
        std::fill(blob.bytes.begin(),blob.bytes.end(),0xff);
        for(int y=20;y<25;y++)for(int x=20;x<25;x++) {
            const int word=(y/2)*512+x/2,shift=(!(y&1)?8:0)+(!(x&1)?4:0);
            auto& byte=blob.bytes[word*2+shift/8];byte=uint8_t((byte&~(15<<(shift&7)))|(2<<(shift&7)));
        }
    }
    const Ray quad_ray{{0,0,0},{.5/400,-.5/400,-1}};
    hit=trace_scene_aim(quad_ray,f,.01f,2.f);assert(hit.target&&close(hit.target->z,-1));
    for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1")std::fill(blob.bytes.begin(),blob.bytes.end(),0xff);
    hit=trace_scene_aim(quad_ray,f,.01f,2.f);assert(hit.target&&close(hit.target->z,-2));
#ifdef RCHASE2_RENDERER_GL_TEST
    // Real GPU color checks: no raster or polygon material is mocked here.
    GLContext context;f=frame();f.polygons={quad()};f.background.assign(size_t(f.width)*f.height,0xff000000);
    GLuint framebuffer,color,depth;glGenFramebuffers(1,&framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,f.width,f.height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,f.width,f.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glViewport(0,0,f.width,f.height);auto l=f.lens;const float n=.001f;
    auto projection=glm::frustum(float(-l.cx/l.fx*n),float((f.width-l.cx)/l.fx*n),float(-(f.height-l.cy)/l.fy*n),float(l.cy/l.fy*n),n,100.f);
    auto pixel=[&](int x,int y){uint8_t p[4];glReadPixels(x,f.height-1-y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);return 0xff000000u|(unsigned(p[0])<<16)|(unsigned(p[1])<<8)|p[2];};
    {GLScene scene;auto image=std::make_shared<NativeFrame>(f);scene.prepare(image);scene.draw(projection,.01f,2.f);assert(pixel(248,191)==0xffac9884u);
        // Cached aiming consumes the very snapshot and clipped triangle stream
        // prepared for drawing, with identical two-hand/material semantics.
        auto compare_aim=[](const SceneAim& a,const SceneAim& b){
            assert(bool(a.target)==bool(b.target)&&a.native.valid==b.native.valid&&a.native.inside==b.native.inside);
            if(a.target)assert(close(a.target->x,b.target->x)&&close(a.target->y,b.target->y)&&close(a.target->z,b.target->z));
            if(a.native.valid)assert(close(a.native.u,b.native.u)&&close(a.native.v,b.native.v));
        };
        {
            GLScene cached;assert(!cached.trace_aim({{0,0,0},{0,0,-1}},.01f,3.f).target);
            NativeLens override_lens=f.lens;override_lens.fx=override_lens.fy=390;
            const Ray hands[]={{{-.032,0,0},{0,0,-1}},{{.032,0,0},{0,0,-1}},{{0,0,0},{.0025,0,-1}},{{0,0,0},{1,0,-1}}};
            auto verify=[&](const NativeFrame& fixture,bool wider,bool immersive){
                auto snapshot=std::make_shared<const NativeFrame>(fixture);cached.prepare(snapshot,wider);cached.set_immersive(immersive);cached.set_presentation_lens(override_lens);
                for(const auto& ray:hands)for(int repeat=0;repeat<2;repeat++)compare_aim(cached.trace_aim(ray,.01f,3.f),trace_scene_aim(ray,*snapshot,.01f,3.f,wider,&override_lens,immersive));
            };
            auto fixture=frame();fixture.polygons={quad()};verify(fixture,false,false);
            auto cached_hit=cached.trace_aim(hands[0],.01f,3.f);assert(cached_hit.target&&close(cached_hit.target->z,-1));
            auto changed_lens=override_lens;changed_lens.fx=changed_lens.fy=300;cached.set_presentation_lens(changed_lens);
            compare_aim(cached.trace_aim(hands[1],.01f,3.f),trace_scene_aim(hands[1],fixture,.01f,3.f,false,&changed_lens));
            fixture.polygons={quad(50)};verify(fixture,false,false);cached_hit=cached.trace_aim(hands[1],.01f,3.f);assert(cached_hit.target&&close(cached_hit.target->z,-.5));
            fixture.polygons={quad()};fixture.polygons[0].vertices[0].x=-100000;verify(fixture,true,false);verify(fixture,false,false);
            auto wide_fixture=frame();wide_fixture.polygons={quad()};for(auto& v:wide_fixture.polygons[0].vertices)v.x*=10;
            const auto wide_snapshot=std::make_shared<const NativeFrame>(wide_fixture);const Ray outside{{0,0,0},{-1,0,-1}};
            cached.prepare(wide_snapshot,false);cached_hit=cached.trace_aim(outside,.01f,3.f);assert(cached_hit.target&&close(cached_hit.target->z,-3));
            cached.prepare(wide_snapshot,true);cached_hit=cached.trace_aim(outside,.01f,3.f);assert(cached_hit.target&&close(cached_hit.target->z,-1));
            cached.prepare(wide_snapshot,false);cached_hit=cached.trace_aim(outside,.01f,3.f);assert(cached_hit.target&&close(cached_hit.target->z,-3));
            fixture.polygons={camera_quad(60)};verify(fixture,false,true);verify(fixture,false,false);
            fixture.polygons={quad()};fixture.polygons[0].header[0]=0x8000;verify(fixture,false,false);
            fixture.polygons[0].header[0]=0x6000;fixture.polygons[0].lod=10000;
            for(auto& blob:fixture.blobs)if(blob.name=="texture0"||blob.name=="texture1")std::fill(blob.bytes.begin(),blob.bytes.end(),0xff);
            verify(fixture,false,false);cached_hit=cached.trace_aim(hands[0],.01f,3.f);assert(cached_hit.target&&close(cached_hit.target->z,-3));
            fixture=frame();fixture.polygons={quad()};fixture.polygons[0].focus={1,1};verify(fixture,false,false);
            fixture.polygons[0].focus={400,400};fixture.rejected=1;verify(fixture,false,false);
            fixture.rejected=0;fixture.blobs.clear();verify(fixture,true,false);
            fixture.complete=false;verify(fixture,false,false);assert(!cached.trace_aim(hands[0],.01f,3.f).target);
            cached.prepare({});assert(!cached.trace_aim(hands[0],.01f,3.f).target);
        }
        for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1")std::fill(blob.bytes.begin(),blob.bytes.end(),0x22);
        f.polygons[0].header[0]=0x4000;f.polygons[0].lod=10000;image=std::make_shared<NativeFrame>(f);scene.prepare(image);scene.draw(projection,.01f,2.f);assert(pixel(248,191)==0xffe4d0bcu);
        f.polygons[0].header[0]=0xc000;image=std::make_shared<NativeFrame>(f);scene.prepare(image);scene.draw(projection,.01f,2.f);
        assert(pixel(249,191)==0xff000000u);assert(pixel(248,191)==0xffe4d0bcu);
        // Moving the eye shifts the same real surface and its ray hit together.
        f.polygons[0].header[0]=0x4000;image=std::make_shared<NativeFrame>(f);scene.prepare(image);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection*glm::translate(glm::mat4(1),glm::vec3(-.032f,0,0)),.01f,2.f);
        // RailChase2 fx400 at1m shifts the native surface by12.8pixels.
        assert(pixel(134,191)==0xff000000u);assert(pixel(136,191)==0xffe4d0bcu);assert(pixel(337,191)==0xff000000u);
        auto aim=trace_scene_aim({{.032,0,0},{0,0,-1}},f,.01f,2.f);assert(aim.target&&close(aim.target->z,-1));
        assert(close(aim.native.u,(248+400.*.032)/495));
        scene.set_presentation_lens(display_lens);
        auto display_projection=glm::frustum(float(-display_lens.cx/display_lens.fx*n),float((f.width-display_lens.cx)/display_lens.fx*n),float(-(f.height-display_lens.cy)/display_lens.fy*n),float(display_lens.cy/display_lens.fy*n),n,100.f);
        scene.draw(display_projection,.01f,2.f);assert(pixel(248,191)==0xffe4d0bcu);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(display_projection*glm::translate(glm::mat4(1),glm::vec3(-.032f,0,0)),.01f,2.f);
        // fx390 at1m gives12.48pixels ofparallax, not58px orpanel6.24px.
        assert(pixel(134,191)==0xff000000u);assert(pixel(137,191)==0xffe4d0bcu);assert(pixel(337,191)==0xff000000u);
        scene.set_presentation_lens({});
        // Native quads interpolate along their left/right scanline edges. A
        // diagonal triangle fan changes non-planar texture coordinates even
        // when the quad's four positions lie on one physical plane.
        f=frame();f.polygons={quad()};auto& warped=f.polygons[0];
        warped.header[0]=0x4000;warped.lod=10000;
        warped.vertices[0].u=0;warped.vertices[0].v=0;
        warped.vertices[1].u=64;warped.vertices[1].v=0;
        warped.vertices[2].u=640;warped.vertices[2].v=640;
        warped.vertices[3].u=0;warped.vertices[3].v=64;
        for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1") {
            std::fill(blob.bytes.begin(),blob.bytes.end(),0);
            for(int y=0;y<32;y++)for(int x=0;x<32;x++) {
                const int word=(y/2)*512+x/2,shift=(!(y&1)?8:0)+(!(x&1)?4:0);
                blob.bytes[word*2+shift/8]|=uint8_t((1+x/4)<<(shift&7));
            }
        }
        const float fx=(248.5f-148.f)/200.f,fy=(191.5f-91.f)/200.f;
        const auto expected_quad=Model2Resources(f).sample(warped,fx*(8+72*fy),fy*(8+72*fx),100,248,191);
        assert(expected_quad);
        image=std::make_shared<NativeFrame>(f);scene.prepare(image);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection,.01f,2.f);
        assert(pixel(248,191)==*expected_quad);
        // One ordered geometry submission must retain each primitive's own
        // material, alpha key, checker state and world/panel projection. Calls
        // are observed while still executing the real GPU draw and readback.
        f=frame();f.background.assign(size_t(f.width)*f.height,0xff000000);
        f.foreground.assign(size_t(f.width)*f.height,0);
        f.foreground[190*f.width+310]=0xff12ab34;
        auto back=quad(),middle=quad(),front=quad();
        back.sort=30;middle.sort=20;front.sort=10;
        middle.focus={1,1};middle.header[0]=0x8000;middle.luma=128;
        front.header={0x6000,1,0,64};front.luma=196;front.lod=10000;
        front.vertices[1].u=front.vertices[2].u=256;
        front.vertices[2].v=front.vertices[3].v=256;
        f.polygons={back,middle,front};
        const unsigned alternate=25|(4<<5)|(12<<10);
        f.blobs[2].bytes[8194]=uint8_t(alternate);f.blobs[2].bytes[8195]=uint8_t(alternate>>8);
        for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1") {
            std::fill(blob.bytes.begin(),blob.bytes.end(),0xff);
            for(int y=0;y<32;y++)for(int x=0;x<16;x++) {
                const int word=(y/2)*512+x/2,shift=(!(y&1)?8:0)+(!(x&1)?4:0);
                auto& value=blob.bytes[word*2+shift/8];value=uint8_t((value&~(15<<(shift&7)))|(2<<(shift&7)));
            }
        }
        Model2Resources mixed_material(f);
        const auto front_color=mixed_material.sample(front,5,16,100,180,191);
        const auto middle_color=mixed_material.sample(middle,0,0,100,300,191);
        const auto back_color=mixed_material.sample(back,0,0,100,301,191);
        assert(front_color&&middle_color&&back_color&&front_color!=middle_color&&middle_color!=back_color);
        image=std::make_shared<NativeFrame>(f);scene.prepare(image);
        original_draw_arrays=glad_glDrawArrays;glad_glDrawArrays=observe_draw_arrays;observed_draw_calls=observed_world_draw_calls=0;
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection,.01f,2.f);
        glad_glDrawArrays=original_draw_arrays;
        assert(pixel(180,191)==*front_color&&pixel(300,191)==*middle_color&&pixel(301,191)==*back_color);
        assert(pixel(310,190)==0xff12ab34&&pixel(100,191)==0xff000000);
        assert(observed_world_draw_calls==1&&observed_draw_calls==3); // Background, geometry, foreground.
        observed_draw_calls=observed_world_draw_calls=0;glad_glDrawArrays=observe_draw_arrays;
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection*glm::translate(glm::mat4(1),glm::vec3(-.016f,0,0)),.01f,2.f);
        glad_glDrawArrays=original_draw_arrays;
        assert(pixel(180,191)==*front_color&&pixel(300,191)==*back_color&&pixel(301,191)==*middle_color);
        assert(pixel(307,190)==0xff12ab34&&observed_world_draw_calls==1&&observed_draw_calls==3);
        // A rejected scene must show its exact native fallback even if its
        // remaining polygons and material banks would otherwise be drawable.
        f=frame();f.polygons={quad()};f.rejected=1;
        f.native.assign(size_t(f.width)*f.height,0xff123456);
        image=std::make_shared<NativeFrame>(f);scene.prepare(image);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection,.01f,2.f);
        assert(pixel(248,191)==0xff123456);
        // Missing material resources use the exact native diagnostic raster.
        f.blobs.clear();f.native.assign(size_t(f.width)*f.height,0xff123456);image=std::make_shared<NativeFrame>(f);scene.prepare(image);scene.draw(projection,.01f,2.f);assert(pixel(248,191)==0xff123456);
        // A focus-600 primitive must use its actual physical coordinates when
        // the eye turns 60 degrees, independent of the focus-400 world lens.
        f=frame();f.background.assign(size_t(f.width)*f.height,0xff000000);
        auto side=camera_quad(60);side.focus={600,600};for(auto& v:side.vertices){v.x*=1.5f;v.y*=1.5f;}
        f.polygons={side};image=std::make_shared<NativeFrame>(f);scene.prepare(image);scene.set_immersive(true);
        const auto side_view=glm::lookAt(glm::vec3(0),glm::vec3(std::sqrt(.75f),0,-.5f),glm::vec3(0,1,0));
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection*side_view,.01f,3.f);
        assert(pixel(248,191)==0xffac9884u);
        // Real eye rotations reveal side, camera-plane-crossing and behind
        // geometry. Texture alpha remains evaluated on each actual surface.
        for(double yaw:{-60.,60.,90.,180.}) {
            f=frame();f.background.assign(size_t(f.width)*f.height,0xff000000);
            p=camera_quad(yaw);p.header[0]=0x4000;p.lod=10000;f.polygons={p};
            for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1")std::fill(blob.bytes.begin(),blob.bytes.end(),0x22);
            const float radians=float(yaw*3.14159265358979323846/180);
            const auto view=glm::lookAt(glm::vec3(0),glm::vec3(std::sin(radians),0,-std::cos(radians)),glm::vec3(0,1,0));
            image=std::make_shared<NativeFrame>(f);scene.prepare(image);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection*view,.01f,3.f);
            assert(pixel(248,191)==0xffe4d0bcu&&pixel(150,191)==0xff000000u);
            auto aim=trace_scene_aim({{0,0,0},{std::sin(radians),0,-std::cos(radians)}},f,.01f,3.f,false,nullptr,true);
            assert(aim.target);
        }
        // Lean parallax follows the physical two-metre square: 6.4 source
        // pixels for 32 mm. A presentation lens override cannot stretch it.
        f=frame();f.background.assign(size_t(f.width)*f.height,0xff000000);f.polygons={camera_quad()};
        image=std::make_shared<NativeFrame>(f);scene.prepare(image);scene.set_presentation_lens({496,384,100,100,248,191});
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection*glm::translate(glm::mat4(1),glm::vec3(-.032f,0,0)),.01f,3.f);
        assert(pixel(190,191)==0xff000000u&&pixel(193,191)==0xffac9884u&&pixel(293,191)==0xff000000u);
        // Background/foreground rasters do not write an opaque depth plane.
        // Native window priorities still win in the single ordered draw.
        auto near_world=camera_quad(),far_world=camera_quad(0,4);far_world.window=2;far_world.luma=128;
        f.polygons={near_world,far_world};f.foreground.assign(size_t(f.width)*f.height,0);f.foreground[191*f.width+248]=0xff12ab34;
        const auto far_color=Model2Resources(f).sample(far_world,0,0,400,249,191);assert(far_color);
        image=std::make_shared<NativeFrame>(f);scene.prepare(image);
        observed_draw_calls=observed_world_draw_calls=0;glad_glDrawArrays=observe_draw_arrays;
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection,.01f,3.f);glad_glDrawArrays=original_draw_arrays;
        assert(pixel(249,191)==*far_color&&pixel(248,191)==0xff12ab34);
        assert(observed_world_draw_calls==1&&observed_draw_calls==3);
        float background_depth=0;glReadPixels(100,f.height-1-191,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&background_depth);assert(close(background_depth,1));
        // Keep RailChase2's native scanline interpolation for a warped quad's
        // UVs; replacing the atlas with triangle UVs changes this center texel.
        f=frame();p=quad();p.focus={400,400};p.header[0]=0x4000;p.lod=10000;
        p.vertices[1].u=64;p.vertices[2].u=640;p.vertices[2].v=640;p.vertices[3].v=64;f.polygons={p};
        for(auto& blob:f.blobs)if(blob.name=="texture0"||blob.name=="texture1") {
            std::fill(blob.bytes.begin(),blob.bytes.end(),0);
            for(int y=0;y<32;y++)for(int x=0;x<32;x++) {
                const int word=(y/2)*512+x/2,shift=(!(y&1)?8:0)+(!(x&1)?4:0);
                blob.bytes[word*2+shift/8]|=uint8_t((1+x/4)<<(shift&7));
            }
        }
        const auto native_quad_color=Model2Resources(f).sample(p,22.20045f,22.20045f,100,248,191);assert(native_quad_color);
        image=std::make_shared<NativeFrame>(f);scene.prepare(image);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection,.01f,3.f);
        assert(pixel(248,191)==*native_quad_color);
        // Turning the prototype off on the same snapshot rebuilds the original
        // clipped stream and restores the untouched diagnostic lens override.
        f=frame();f.polygons={camera_quad(60)};f.background.assign(size_t(f.width)*f.height,0xff000000);
        image=std::make_shared<NativeFrame>(f);scene.prepare(image);scene.set_presentation_lens({});scene.set_immersive(false);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);scene.draw(projection*side_view,.01f,3.f);assert(pixel(248,191)==0xff000000u);
    }
    assert(glGetError()==GL_NO_ERROR);glDeleteFramebuffers(1,&framebuffer);glDeleteTextures(1,&color);glDeleteRenderbuffers(1,&depth);
    std::cout<<"Model 2 production OpenGL material/stereo/fallback tests passed\n";
#endif
    f=frame();f.polygons={quad()};f.rejected=1;
    hit=trace_scene_aim({{0,0,0},{0,0,-1}},f,.01f,2.f);
    assert(hit.target&&close(hit.target->z,-2));
    std::cout<<"Model 2 renderer geometry/aim/texture tests passed\n";
}
