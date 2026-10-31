// SPDX-License-Identifier: GPL-2.0-or-later
#include "gl_scene.h"
#include "hud_size.h"
#include "model2_scene.h"
#include "model2_scanline.h"
#include "native_reticle.h"
#include "reticle_size.h"
#include "custom_reticle.h"
#include "shaders/custom_reticle_shaders.h"
#include "cart_bottom.h"
#include "shaders/model2_shaders.h"
#include "shaders/cart_bottom_shaders.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace rchase2 {
static GLuint compile(GLenum type,const char* source) {
    std::string code=source;
#ifdef __ANDROID__
    code.replace(0,code.find('\n'),"#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp usampler2D;\nprecision highp sampler2D;");
#endif
    auto shader=glCreateShader(type);const char* text=code.c_str();glShaderSource(shader,1,&text,nullptr);glCompileShader(shader);
    GLint ok{};glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){char log[4096]{};glGetShaderInfoLog(shader,sizeof(log),nullptr,log);glDeleteShader(shader);throw std::runtime_error(log);}return shader;
}
static GLuint program(const char* vertex,const char* fragment) {
    auto vs=compile(GL_VERTEX_SHADER,vertex),fs=compile(GL_FRAGMENT_SHADER,fragment);auto p=glCreateProgram();
    glAttachShader(p,vs);glAttachShader(p,fs);glLinkProgram(p);glDeleteShader(vs);glDeleteShader(fs);
    GLint ok{};glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[4096]{};glGetProgramInfoLog(p,sizeof(log),nullptr,log);glDeleteProgram(p);throw std::runtime_error(log);}return p;
}
struct GLScene::Impl {
    float hud_size{1};bool hud_gameplay{},hud_service{};
    GLuint world{},panel{},vao{},vbo{},panel_vao{},panel_vbo{},reticle_vao{},reticle_vbo{},custom{},custom_vao{},custom_vbo{};
    GLuint resources[6]{},rasters[3]{},scanlines{};
    // Flat integer attributes preserve every primitive's native material and
    // source scanline index in one ordered triangle stream.
    struct WorldVertex {std::array<float,5> raster;std::array<int32_t,4> header,properties;std::array<float,3> camera,original;};
    std::vector<SceneTriangle> triangles,aim_triangles;
    NativeReticles reticles;
    ReticleSize reticle_ingame_size,reticle_custom_size;ReticleMode reticle_mode{ReticleMode::Ingame};bool reticle_gameplay{};
    std::vector<int> reticle_players;
    std::array<std::optional<Vec3>,2> reticle_targets;
    glm::mat4 head_rotation{1};Vec3 head_position;
    std::array<GLint,2> reticle_first{};std::array<GLsizei,2> reticle_count{};std::shared_ptr<const NativeFrame> frame;
    std::unique_ptr<Model2Resources> materials;
    GLsizei vertex_count{};
    bool usable{},wider_view{},immersive{};
    NativeLens presentation_override;
    bool cart_enabled{};CartBottomPose cart_pose;
    GLuint cart_program{},cart_vao{},cart_vbo{};
    struct CartUniforms {GLint vp,matrix,lens,native_lens,scale,immersive,luma,lod;} c{};
    struct WorldUniforms {GLint vp,lens,scale,distance,immersive,reticle_pass,reticle_mask,reticle_model,reticle_center,hud_size;} w{};
    struct PanelUniforms {GLint vp,lens,distance,size,hud_size;} p{};
    struct CustomUniforms {GLint vp,model,half_size,color;} k{};
    void prepare_cart_buffer(){
        if(cart_program)return;
        const auto fragment=cart_bottom_fragment_shader();cart_program=program(CartBottomVertexShader,fragment.c_str());
        c={glGetUniformLocation(cart_program,"vp"),glGetUniformLocation(cart_program,"cart_to_camera"),glGetUniformLocation(cart_program,"lens"),
            glGetUniformLocation(cart_program,"native_lens"),glGetUniformLocation(cart_program,"scale"),glGetUniformLocation(cart_program,"immersive"),
            glGetUniformLocation(cart_program,"luma"),glGetUniformLocation(cart_program,"lod")};
        glUseProgram(cart_program);
        constexpr const char* names[]={"texture0","texture1","palette","colorxlat","lumaram","gamma","source_scanlines"};
        for(int n=0;n<7;n++)glUniform1i(glGetUniformLocation(cart_program,names[n]),n);
        glGenVertexArrays(1,&cart_vao);glBindVertexArray(cart_vao);glGenBuffers(1,&cart_vbo);glBindBuffer(GL_ARRAY_BUFFER,cart_vbo);
        const auto& mesh=cart_bottom_mesh();glBufferData(GL_ARRAY_BUFFER,sizeof(mesh),mesh.data(),GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(CartBottomVertex),nullptr);
        glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(CartBottomVertex),reinterpret_cast<void*>(3*sizeof(float)));
    }
    void draw_cart(const glm::mat4& vp,const NativeLens& lens,float scale){
        prepare_cart_buffer();glUseProgram(cart_program);
        const auto& m=cart_pose.matrix;glm::mat4 model(1);
        for(int col=0;col<3;col++)for(int row=0;row<3;row++)model[col][row]=m[col*3+row];
        model[3]=glm::vec4(m[9],m[10],m[11],1);
        glUniformMatrix4fv(c.vp,1,GL_FALSE,glm::value_ptr(vp));glUniformMatrix4fv(c.matrix,1,GL_FALSE,glm::value_ptr(model));
        glUniform4f(c.lens,lens.fx,lens.fy,lens.cx,lens.cy);
        glUniform4f(c.native_lens,768,768,frame->xoffset+248,384-320+frame->yoffset);
        glUniform1f(c.scale,scale);glUniform1i(c.immersive,immersive);glUniform1i(c.luma,cart_pose.luma);glUniform1i(c.lod,cart_pose.lod);
        glDepthFunc(GL_LESS);glBindVertexArray(cart_vao);glDrawArrays(GL_TRIANGLES,0,GLsizei(cart_bottom_mesh().size()));glDepthFunc(GL_ALWAYS);
    }
    void raster(GLuint texture,const std::vector<uint32_t>& pixels,bool transparent) {
        std::vector<uint8_t> rgba(pixels.size()*4);
        for(size_t i=0;i<pixels.size();i++) {auto c=pixels[i];rgba[i*4]=c>>16;rgba[i*4+1]=c>>8;rgba[i*4+2]=c;rgba[i*4+3]=transparent&&c==0?0:255;}
        glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,frame->width,frame->height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    }
    void draw_raster(unsigned index,const glm::mat4& vp,const NativeLens& lens,float distance) {
        const auto& pixels=index==0?frame->background:index==1?frame->foreground:frame->native;
        if(pixels.size()!=size_t(frame->width)*frame->height)return;
        glUseProgram(panel);glUniformMatrix4fv(p.vp,1,GL_FALSE,glm::value_ptr(vp));
        glUniform4f(p.lens,lens.fx,lens.fy,lens.cx,lens.cy);glUniform1f(p.distance,distance);glUniform2f(p.size,frame->width,frame->height);
        glUniform1f(p.hud_size,index==1?effective_hud_size(hud_size,immersive&&wider_view,hud_gameplay,hud_service,usable):1.f);
        glBindVertexArray(panel_vao);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,rasters[index]);glDrawArrays(GL_TRIANGLES,0,6);
    }
    void draw_custom(const glm::mat4& vp,const NativeLens&,float distance){
        if(!reticle_gameplay||reticle_mode!=ReticleMode::Custom)return;
        glDepthMask(GL_FALSE);glUseProgram(custom);glUniformMatrix4fv(k.vp,1,GL_FALSE,glm::value_ptr(vp));glBindVertexArray(custom_vao);
        for(unsigned player=0;player<2;player++)if(reticle_targets[player]){
            const auto& target=*reticle_targets[player];auto model=head_rotation;
            model[3]=glm::vec4(float(target.x),float(target.y),float(target.z),1);
            const auto delta=target-head_position;
            const auto reference=reticle_size_reference(std::hypot(delta.x,delta.y,delta.z),distance,reticle_custom_size);
            glUniformMatrix4fv(k.model,1,GL_FALSE,glm::value_ptr(model));
            glUniform2f(k.half_size,float(.07*reference),float(.07*reference));
            if(player==0)glUniform3f(k.color,1,.125f,.125f);else glUniform3f(k.color,.125f,.4f,1);
            glDrawArrays(GL_TRIANGLES,0,72);
        }
    }
};
void GLScene::set_hud_size(float size,bool gameplay,bool service){auto& i=*impl_;i.hud_size=size;i.hud_gameplay=gameplay;i.hud_service=service;}
GLScene::GLScene():impl_(std::make_unique<Impl>()) {
    auto& i=*impl_;i.world=program(Model2Vertex,Model2Fragment);i.panel=program(Model2PanelVertex,Model2PanelFragment);
    i.custom=program(CustomReticleVertexShader,CustomReticleFragmentShader);
    i.k={glGetUniformLocation(i.custom,"vp"),glGetUniformLocation(i.custom,"model"),glGetUniformLocation(i.custom,"half_size"),glGetUniformLocation(i.custom,"player_color")};
    i.w={glGetUniformLocation(i.world,"vp"),glGetUniformLocation(i.world,"lens"),glGetUniformLocation(i.world,"scale"),glGetUniformLocation(i.world,"distance"),glGetUniformLocation(i.world,"immersive"),
        glGetUniformLocation(i.world,"reticle_pass"),glGetUniformLocation(i.world,"reticle_mask"),glGetUniformLocation(i.world,"reticle_model"),glGetUniformLocation(i.world,"reticle_center"),glGetUniformLocation(i.world,"hud_size")};
    i.p={glGetUniformLocation(i.panel,"vp"),glGetUniformLocation(i.panel,"lens"),glGetUniformLocation(i.panel,"distance"),glGetUniformLocation(i.panel,"size"),glGetUniformLocation(i.panel,"hud_size")};
    glUseProgram(i.world);constexpr const char* names[]={"texture0","texture1","palette","colorxlat","lumaram","gamma"};
    for(int n=0;n<6;n++)glUniform1i(glGetUniformLocation(i.world,names[n]),n);
    glUniform1i(glGetUniformLocation(i.world,"source_scanlines"),6);
    glUseProgram(i.panel);glUniform1i(glGetUniformLocation(i.panel,"image"),0);
    glGenTextures(6,i.resources);glGenTextures(3,i.rasters);glGenTextures(1,&i.scanlines);
    glBindTexture(GL_TEXTURE_2D,i.scanlines);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    for(auto texture:i.resources){glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
    for(auto texture:i.rasters){glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}
    auto world_buffer=[](GLuint& vao,GLuint& vbo){
        glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Impl::WorldVertex),nullptr);
        glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Impl::WorldVertex),reinterpret_cast<void*>(3*sizeof(float)));
        glEnableVertexAttribArray(2);glVertexAttribIPointer(2,4,GL_INT,sizeof(Impl::WorldVertex),reinterpret_cast<void*>(offsetof(Impl::WorldVertex,header)));
        glEnableVertexAttribArray(3);glVertexAttribIPointer(3,4,GL_INT,sizeof(Impl::WorldVertex),reinterpret_cast<void*>(offsetof(Impl::WorldVertex,properties)));
        glEnableVertexAttribArray(4);glVertexAttribPointer(4,3,GL_FLOAT,GL_FALSE,sizeof(Impl::WorldVertex),reinterpret_cast<void*>(offsetof(Impl::WorldVertex,camera)));
        glEnableVertexAttribArray(5);glVertexAttribPointer(5,3,GL_FLOAT,GL_FALSE,sizeof(Impl::WorldVertex),reinterpret_cast<void*>(offsetof(Impl::WorldVertex,original)));
    };
    world_buffer(i.vao,i.vbo);world_buffer(i.reticle_vao,i.reticle_vbo);
    glGenVertexArrays(1,&i.panel_vao);glBindVertexArray(i.panel_vao);glGenBuffers(1,&i.panel_vbo);glBindBuffer(GL_ARRAY_BUFFER,i.panel_vbo);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);
    glGenVertexArrays(1,&i.custom_vao);glBindVertexArray(i.custom_vao);glGenBuffers(1,&i.custom_vbo);glBindBuffer(GL_ARRAY_BUFFER,i.custom_vbo);
    const auto custom=custom_reticle_mesh();glBufferData(GL_ARRAY_BUFFER,sizeof(custom),custom.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(CustomReticleVertex),nullptr);
}
GLScene::~GLScene(){auto& i=*impl_;glDeleteProgram(i.world);glDeleteProgram(i.panel);glDeleteProgram(i.custom);glDeleteBuffers(1,&i.custom_vbo);glDeleteVertexArrays(1,&i.custom_vao);glDeleteProgram(i.cart_program);glDeleteBuffers(1,&i.cart_vbo);glDeleteVertexArrays(1,&i.cart_vao);glDeleteBuffers(1,&i.vbo);glDeleteBuffers(1,&i.panel_vbo);glDeleteBuffers(1,&i.reticle_vbo);glDeleteVertexArrays(1,&i.vao);glDeleteVertexArrays(1,&i.panel_vao);glDeleteVertexArrays(1,&i.reticle_vao);glDeleteTextures(6,i.resources);glDeleteTextures(3,i.rasters);glDeleteTextures(1,&i.scanlines);}
void GLScene::set_immersive(bool enabled){if(impl_->immersive!=enabled){impl_->immersive=enabled;auto frame=impl_->frame;impl_->frame={};prepare(std::move(frame),impl_->wider_view);}}
void GLScene::set_cart_bottom_enabled(bool enabled){
    impl_->cart_enabled=enabled;
}
void GLScene::set_presentation_lens(const NativeLens& lens){impl_->presentation_override=lens;}
void GLScene::set_native_reticles_enabled(bool enabled){impl_->reticle_gameplay=enabled;}
void GLScene::set_reticle_size(float minimum_ratio,float shrink_distance){set_reticle_sizes(minimum_ratio,1,minimum_ratio,1,shrink_distance);}
void GLScene::set_reticle_sizes(float ingame_min,float ingame_max,float custom_min,float custom_max,float shrink_distance){
    impl_->reticle_ingame_size=reticle_size_settings(ingame_min,shrink_distance,ingame_max);
    impl_->reticle_custom_size=reticle_size_settings(custom_min,shrink_distance,custom_max);
}
void GLScene::set_reticle_mode(ReticleMode mode,bool gameplay){impl_->reticle_mode=reticle_mode_value(int(mode));impl_->reticle_gameplay=gameplay;}

void GLScene::set_reticle_targets(const std::array<std::optional<Vec3>,2>& targets,const glm::mat4& head){
    auto& i=*impl_;i.reticle_targets={};
    for(int column=0;column<4;column++)for(int row=0;row<4;row++)if(!std::isfinite(head[column][row]))return;
    i.head_rotation=glm::mat4(glm::mat3(head));
    i.head_position={head[3].x,head[3].y,head[3].z};
    for(unsigned player=0;player<2;player++)if(targets[player]&&finite(*targets[player]))i.reticle_targets[player]=targets[player];
}
SceneAim GLScene::trace_aim(Ray ray,float scale,float distance) const {
    const auto& i=*impl_;if(!i.frame)return {};
    const auto lens=i.immersive?native_camera_lens(*i.frame):presentation_lens(*i.frame,&i.presentation_override);
    return trace_prepared_scene_aim(ray,*i.frame,i.aim_triangles,i.usable?i.materials.get():nullptr,scale,distance,lens,i.immersive);
}
void GLScene::prepare(std::shared_ptr<const NativeFrame> frame,bool wider_view) {
    auto& i=*impl_;if(i.frame==frame&&i.wider_view==wider_view)return;
    // Keep the preceding immutable snapshot alive until all comparisons finish;
    // its pool allocation must not be recycled while checking uploaded bytes.
    auto previous=std::move(i.frame);const bool previous_usable=i.usable;
    const bool previous_complete=previous&&previous->complete;
    i.frame=std::move(frame);i.wider_view=wider_view;i.triangles.clear();i.aim_triangles.clear();i.materials.reset();i.vertex_count=0;i.usable=false;
    i.cart_pose={};i.reticles={};i.reticle_players.clear();i.reticle_count={};if(!i.frame||!i.frame->complete)return;
    i.reticles=native_reticles(*i.frame);
    const auto& f=*i.frame;i.materials=std::make_unique<Model2Resources>(f);const auto& resource=*i.materials;
    i.triangles=f.rejected==0?scene_triangles(f,wider_view,i.immersive):std::vector<SceneTriangle>{};
    i.usable=resource.valid()&&!i.triangles.empty();
    if(i.usable)i.cart_pose=cart_bottom_pose(f);
    if(!i.usable)i.reticles={};
    i.reticle_players.assign(f.polygons.size(),-1);
    for(unsigned player=0;player<2;player++)for(size_t index:i.reticles[player].duplicates)i.reticle_players[index]=int(player);
    // Keep the original cart visible but aim through it, independently of the
    // authored floor option. A matching object ID alone never removes a hit:
    // require the complete measured cart signature, shell and rigid pose.
    for(const auto& triangle:i.triangles)if(i.reticle_players[triangle.polygon]<0
        &&!(i.cart_pose.valid&&f.polygons[triangle.polygon].object==0x8363ce))i.aim_triangles.push_back(triangle);
    glActiveTexture(GL_TEXTURE0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    constexpr const char* names[]={"texture0","texture1","palette","colorxlat","luma","gamma"};
    if(i.usable)for(int n=0;n<6;n++)for(const auto& b:f.blobs)if(b.name==names[n]) {
        // A preceding fallback did not upload resources, even if its blobs
        // happen to match. Compare content by name, never by pooled address.
        if(previous_complete&&previous_usable) {
            auto old=std::find_if(previous->blobs.begin(),previous->blobs.end(),[&](const auto& blob){return blob.name==b.name;});
            if(old!=previous->blobs.end()&&old->bytes==b.bytes)continue;
        }
        const int rows=int((b.bytes.size()+2047)/2048);std::vector<uint8_t> padded(size_t(rows)*2048);std::copy(b.bytes.begin(),b.bytes.end(),padded.begin());
        glBindTexture(GL_TEXTURE_2D,i.resources[n]);glTexImage2D(GL_TEXTURE_2D,0,GL_R8UI,2048,rows,0,GL_RED_INTEGER,GL_UNSIGNED_BYTE,padded.data());
    }
    const bool previous_shape=previous_complete&&previous->width==f.width&&previous->height==f.height;
    if(f.background.size()==size_t(f.width)*f.height&&(!previous_shape||previous->background!=f.background))i.raster(i.rasters[0],f.background,false);
    if(f.foreground.size()==size_t(f.width)*f.height&&(!previous_shape||previous->foreground!=f.foreground))i.raster(i.rasters[1],f.foreground,true);
    // Native pixels of a usable frame were not uploaded; entering fallback
    // must refresh this texture regardless of that frame's native contents.
    if(!i.usable&&f.native.size()==size_t(f.width)*f.height&&(!previous_shape||previous_usable||previous->native!=f.native))i.raster(i.rasters[2],f.native,false);
    if(i.usable) {
        // Two headers per source polygon, then compact records for its native
        // scanline spans and its original near-clipped ring. Integer offsets
        // are stored as separate x/y coordinates so float precision cannot
        // truncate an index. The 2048-square cap is portable to GLES 3.
        constexpr size_t limit=2048*2048;
        std::vector<std::array<float,4>> atlas(f.polygons.size()*2);
        auto location=[](size_t at){return std::array<float,2>{float(at%2048),float(at/2048)};};
        for(size_t n=0;n<f.polygons.size();n++) {
            const auto raster=native_polygon_raster(f,f.polygons[n]);
            if(atlas.size()+raster.rows.size()*2+raster.wider_vertices.size()*2>limit)throw std::runtime_error("Model 2 scanline atlas exceeds portable texture bounds");
            auto at=location(atlas.size());atlas[n*2]={at[0],at[1],float(raster.first_y),float(raster.rows.size())};
            for(const auto& row:raster.rows) {
                atlas.push_back({row.start[0],row.start[1],row.start[2],float(row.start_x)});
                atlas.push_back({row.step[0],row.step[1],row.step[2],float(row.stop_x)});
            }
            at=location(atlas.size());atlas[n*2+1]={at[0],at[1],float(raster.wider_vertices.size()),source_front_polygon(f.polygons[n])?0.f:1.f};
            for(const auto& v:raster.wider_vertices) {
                atlas.push_back({v.x,v.y,v.q[0],v.q[1]});atlas.push_back({v.q[2],0,0,0});
            }
        }
        const size_t rows=(atlas.size()+2047)/2048;atlas.resize(rows*2048);
        glBindTexture(GL_TEXTURE_2D,i.scanlines);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,2048,GLsizei(rows),0,GL_RGBA,GL_FLOAT,atlas.data());
    }
    std::vector<Impl::WorldVertex> vertices;vertices.reserve(i.triangles.size()*3);
    for(auto it=i.triangles.rbegin();it!=i.triangles.rend();++it) {
        const auto& p=f.polygons[it->polygon];
        const std::array<int32_t,4> header={int32_t(p.header[0]),int32_t(p.header[1]),int32_t(p.header[2]),int32_t(p.header[3])};
        const std::array<int32_t,4> properties={int32_t(p.luma),int32_t(p.lod),int32_t(it->panel)|((i.reticle_players[it->polygon]+1)<<1),int32_t(it->polygon)};
        // Preserve reverse native occupancy order, including window overrides,
        // checker holes, alpha keys and interleaved world/panel primitives.
        for(const auto& v:it->vertices)vertices.push_back({{v.pixel_x,v.pixel_y,v.depth,v.u,v.v},header,properties,
            {float(v.camera.x),float(v.camera.y),float(v.camera.z)},
            {float(v.native_homogeneous.x),float(v.native_homogeneous.y),float(v.native_homogeneous.z)}});
    }
    i.vertex_count=GLsizei(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER,i.vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Impl::WorldVertex),vertices.data(),GL_STREAM_DRAW);
    vertices.clear();const auto order=native_order(f);
    for(unsigned player=0;player<2;player++) {
        i.reticle_first[player]=GLint(vertices.size());
        for(auto it=order.rbegin();it!=order.rend();++it)if(i.reticle_players[*it]==int(player)&&f.polygons[*it].window==i.reticles[player].window&&!f.polygons[*it].culled) {
            const auto& p=f.polygons[*it];
            const std::array<int32_t,4> header={int32_t(p.header[0]),int32_t(p.header[1]),int32_t(p.header[2]),int32_t(p.header[3])};
            const std::array<int32_t,4> properties={int32_t(p.luma),int32_t(p.lod),int32_t((player+1)<<1),int32_t(*it)};
            // Retain the original complete planar ring, including portions that
            // the cabinet viewport clipped when the native gun was offscreen.
            for(unsigned triangle=1;triangle+1<p.count;triangle++)for(unsigned index:{0u,triangle,triangle+1}) {
                const auto& v=p.vertices[index];
                vertices.push_back({{f.xoffset+p.center[0]+v.x/v.z,384-p.center[1]+f.yoffset-v.y/v.z,v.z,v.u/8,v.v/8},header,properties,{},{}});
            }
        }
        i.reticle_count[player]=GLsizei(vertices.size())-i.reticle_first[player];
    }
    glBindBuffer(GL_ARRAY_BUFFER,i.reticle_vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Impl::WorldVertex),vertices.data(),GL_STREAM_DRAW);
    const float w=float(f.width),h=float(f.height);float quad[]={0,0,w,0,w,h,0,0,w,h,0,h};
    glBindBuffer(GL_ARRAY_BUFFER,i.panel_vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STREAM_DRAW);
}
void GLScene::draw(const glm::mat4& vp,float scale,float distance) {
    auto& i=*impl_;if(!i.frame||!i.frame->complete||scale<=0||distance<=0)return;
    const auto lens=i.immersive?native_camera_lens(*i.frame):presentation_lens(*i.frame,&i.presentation_override);if(!lens.valid())return;
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_STENCIL_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    if(!i.usable){i.draw_raster(2,vp,lens,distance);i.draw_custom(vp,presentation_lens(*i.frame),distance);glDepthMask(GL_TRUE);glDepthFunc(GL_LESS);return;}
    // Native background is a backdrop rather than an opaque depth plane.
    const bool cart=i.cart_enabled&&i.cart_pose.valid;
    if(i.immersive||cart)glDepthMask(GL_FALSE);
    i.draw_raster(0,vp,lens,distance);
    if(i.immersive||cart)glDepthMask(GL_TRUE);
    glUseProgram(i.world);glUniformMatrix4fv(i.w.vp,1,GL_FALSE,glm::value_ptr(vp));glUniform4f(i.w.lens,lens.fx,lens.fy,lens.cx,lens.cy);
    std::array<ReticleDrawPolicy,2> policy;
    for(unsigned player=0;player<2;player++)policy[player]=reticle_draw_policy(i.reticle_mode,i.reticle_gameplay,i.reticle_count[player]>0,bool(i.reticle_targets[player]));
    glUniform1i(i.w.reticle_pass,GL_FALSE);glUniform2i(i.w.reticle_mask,policy[0].suppress_native,policy[1].suppress_native);
    glUniform1f(i.w.hud_size,effective_hud_size(i.hud_size,i.immersive&&i.wider_view,i.hud_gameplay,i.hud_service,i.usable));
    glUniform1f(i.w.scale,scale);glUniform1f(i.w.distance,distance);glUniform1i(i.w.immersive,i.immersive);glBindVertexArray(i.vao);
    for(int n=0;n<6;n++){glActiveTexture(GL_TEXTURE0+n);glBindTexture(GL_TEXTURE_2D,i.resources[n]);}
    glActiveTexture(GL_TEXTURE6);glBindTexture(GL_TEXTURE_2D,i.scanlines);
    glDrawArrays(GL_TRIANGLES,0,i.vertex_count);
    if(cart)i.draw_cart(vp,lens,scale);
    if(i.immersive)glDepthMask(GL_FALSE);
    i.draw_raster(1,vp,lens,distance);
    if(policy[0].draw_native||policy[1].draw_native) {
        const auto native_lens=presentation_lens(*i.frame);
        glDepthMask(GL_FALSE);glUseProgram(i.world);glUniform1i(i.w.reticle_pass,GL_TRUE);
        glUniform4f(i.w.lens,native_lens.fx,native_lens.fy,native_lens.cx,native_lens.cy);glBindVertexArray(i.reticle_vao);
        // Panel drawing bound its bitmap on unit zero. Restore all native
        // material inputs before rendering the original reticle polygons.
        for(int n=0;n<6;n++){glActiveTexture(GL_TEXTURE0+n);glBindTexture(GL_TEXTURE_2D,i.resources[n]);}
        glActiveTexture(GL_TEXTURE6);glBindTexture(GL_TEXTURE_2D,i.scanlines);
        for(unsigned player=0;player<2;player++)if(policy[player].draw_native) {
            auto model=i.head_rotation;const auto& target=*i.reticle_targets[player];
            model[3]=glm::vec4(float(target.x),float(target.y),float(target.z),1);
            const auto delta=target-i.head_position;
            // Natural shrinking changes artwork offsets, never actual target depth.
            const double reference=reticle_size_reference(std::hypot(delta.x,delta.y,delta.z),distance,i.reticle_ingame_size);
            glUniform1f(i.w.distance,float(reference));
            glUniformMatrix4fv(i.w.reticle_model,1,GL_FALSE,glm::value_ptr(model));
            glUniform2f(i.w.reticle_center,float(i.reticles[player].center_x),float(i.reticles[player].center_y));
            glDrawArrays(GL_TRIANGLES,i.reticle_first[player],i.reticle_count[player]);
        }
    }
    i.draw_custom(vp,presentation_lens(*i.frame),distance);
    glDepthMask(GL_TRUE);glActiveTexture(GL_TEXTURE0);glDepthFunc(GL_LESS);
}
}
