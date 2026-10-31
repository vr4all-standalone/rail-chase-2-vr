// SPDX-License-Identifier: GPL-2.0-or-later
#include "presentation.h"
#include "settings_io.h"
#include "button_latch.h"
#include "controller_controls.h"
#include "host_input_policy.h"
#include "controller_labels.h"
#include "gun_effects.h"
#include "gun_pose.h"
#include "arcade_color.h"
#include "model2_scene.h"
#include "shared/xr_panel.h"
#include "shared/glb_mesh.h"
#include "shared/gun_model.h"
#include "shared/ui_compat.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace rchase2 {
namespace {
const char* labels[]={"Resume","Recenter","Gun","Gun angle","Fire mode","Two players","Laser","Reticle","Reticle min size","Reticle max size","Reticle shrink distance","Haptics","World size","Panel distance","HUD size","Gun size","Wider view","View width","Cart bottom","How to play","Open Service Menu","Reset to defaults","Quit"};
constexpr int row_count=std::size(labels);
std::string number(float n,int digits=2){std::ostringstream o;o<<std::fixed<<std::setprecision(digits)<<n;return o.str();}
void save(const Settings& s) {const char* name=std::getenv("RCHASE2_SETTINGS");std::ofstream out(name?name:"rchase2-settings.ini");if(out)write_settings(out,s);}
struct Model {
    bool arcade_model{};GLuint vao{},vertices{},indices{};std::vector<GLuint> textures;vr::xr::GlbMesh mesh;glm::mat4 placement{1};glm::vec3 muzzle{0,.0471,-.1401};
    void clear(){for(auto t:textures)glDeleteTextures(1,&t);textures.clear();if(indices)glDeleteBuffers(1,&indices);if(vertices)glDeleteBuffers(1,&vertices);if(vao)glDeleteVertexArrays(1,&vao);vao=vertices=indices=0;mesh={};arcade_model=false;}
    ~Model(){clear();}
    void upload() {
        std::vector<float> data;data.reserve(mesh.positions.size()*4);
        for(size_t i=0;i<mesh.positions.size()/3;i++) {
            for(int c=0;c<3;c++)data.push_back(mesh.positions[i*3+c]);
            for(int c=0;c<2;c++)data.push_back(mesh.uvs.empty()?0:mesh.uvs[i*2+c]);
            for(int c=0;c<3;c++)data.push_back(mesh.normals[i*3+c]);
            for(int c=0;c<4;c++)data.push_back(mesh.colors.empty()?1:mesh.colors[i*4+c]);
        }
        glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vertices);glBindBuffer(GL_ARRAY_BUFFER,vertices);glBufferData(GL_ARRAY_BUFFER,data.size()*4,data.data(),GL_STATIC_DRAW);
        for(int i=0;i<4;i++){glEnableVertexAttribArray(i);glVertexAttribPointer(i,i==0||i==2?3:i==1?2:4,GL_FLOAT,GL_FALSE,48,reinterpret_cast<void*>(uintptr_t(i==0?0:i==1?12:i==2?20:32)));}
        glGenBuffers(1,&indices);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*4,mesh.indices.data(),GL_STATIC_DRAW);
        for(const auto& im:mesh.images) {GLuint texture{};glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,im.width,im.height,0,GL_RGBA,GL_UNSIGNED_BYTE,im.rgba.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);textures.push_back(texture);}
    }
    void arcade() {
        using namespace vr::xr::gunmodel;
        clear();arcade_model=true;for(auto p:Positions)mesh.positions.push_back(p*PositionScale);for(auto n:Normals)mesh.normals.push_back(n/127.f);
        for(auto i:Indices)mesh.indices.push_back(i);
        const float colors[][4]={{.5,.035,.05,1},{.035,.035,.045,1},{.15,.15,.17,1},{.12,.22,.3,1}};
        for(unsigned i=0;i<std::size(Parts);i++){vr::xr::GlbPart part;part.firstIndex=Parts[i].first;part.indexCount=Parts[i].count;std::copy_n(colors[i],4,part.color);mesh.parts.push_back(part);}
        placement=glm::translate(glm::mat4(1),glm::vec3(0,-.06,.006));muzzle={0,.0471,-.1401};upload();
    }
    bool file(const char* name,bool zapper) {
        size_t size{};auto bytes=resource::load(name,size);if(!bytes||size>16*1024*1024)return false;
        vr::xr::GlbMesh loaded;std::string error;if(!vr::xr::loadGlb(bytes.get(),size,loaded,error,8,1024))return false;
        clear();mesh=std::move(loaded);
        if(zapper) {
            glm::vec3 grip(.0281f,.038f,-.105f);muzzle={.02765f,.06172f,.0821f};
            placement=glm::rotate(glm::mat4(1),glm::pi<float>(),glm::vec3(0,1,0))*glm::translate(glm::mat4(1),-grip);
        } else {
            glm::vec3 lo(1e9f),hi(-1e9f);for(size_t i=0;i<mesh.positions.size();i+=3){glm::vec3 p(mesh.positions[i],mesh.positions[i+1],mesh.positions[i+2]);lo=glm::min(lo,p);hi=glm::max(hi,p);}
            float span=glm::max(glm::max(hi.x-lo.x,hi.y-lo.y),hi.z-lo.z);if(!std::isfinite(span)||span<1e-6f){clear();return false;}
            glm::vec3 grip((lo.x+hi.x)*.5f,lo.y+(hi.y-lo.y)*.25f,hi.z-(hi.z-lo.z)*.2f);
            placement=glm::scale(glm::mat4(1),glm::vec3(.25f/span))*glm::translate(glm::mat4(1),-grip);muzzle={(lo.x+hi.x)*.5f,(lo.y+hi.y)*.5f,lo.z};
        }
        upload();return true;
    }

};
}
struct Presentation::Impl {
    ControllerHelpPolicy startup_help;
    controller_help::Labels controller_labels;
    bool opened{},help{},fresh{},recenter{},quit_armed{},custom_failed{},service_mode{},service_seen_native{},service_guns{},native_calibration{},calibration_busy{};double quit_at{};int selected{},loaded_gun{-1};
    SharedRuntime* runtime{};
    std::array<ButtonLatch,2> trigger_latch,start_latch;ButtonLatch toggle_latch;bool stick_wait{true};glm::vec2 stick_was{};double repeat_at{};
    int hovered[2]{-1,-1};
    glm::mat4 panel_model{1};PresentationInput input;CabinetInput cabinet;std::array<double,2> shot_time{-100,-100};
    Model gun;std::array<Model,2> controllers;bool controllers_loaded{};GLuint model_program{};GunEffects gun_effects;
    vr::xr::RowsPanel rows(const Settings& s) const {
        vr::xr::RowsPanel p;p.title="VR4ALL / Rail Chase 2";p.footer="Either trigger chooses  /  Menu returns";
        if(selected==7)p.footer="Original artwork, a hollow X, or no gameplay reticle";
        if(selected==8)p.footer="Far-distance minimum for the selected reticle artwork";
        if(selected==9)p.footer="Close-up maximum for the selected reticle artwork";
        if(selected==10)p.footer="Larger distances make reticles shrink more slowly";
        if(selected==14)p.footer="Known 2D HUD only in large view; small view keeps 100%";
        const char* haptics[]={"Off","Light","Medium","Strong"};
        const std::string gun=s.gun==0?"Arcade":s.gun==1?"Zapper":custom_failed?"Custom (Arcade fallback)":"Custom";
        const bool custom=s.reticle_mode==ReticleMode::Custom;
        std::string values[]={"","",gun,number(s.gun_angle,0),"Hold trigger",s.p1_hand==0?"P1 left / P2 right":"P1 right / P2 left",s.laser?"On":"Off",reticle_mode_label(s.reticle_mode),number((custom?s.reticle_custom_min_size:s.reticle_min_size)*100,0)+"%",number((custom?s.reticle_custom_max_size:s.reticle_max_size)*100,0)+"%",number(s.reticle_shrink_distance,1)+" m",haptics[haptic_level(s.haptics)],number(s.world_scale,3),number(s.panel_distance)+" m",number(s.hud_size*100,0)+"%",number(s.gun_scale),s.wider_view?"On":"Off",number(s.view_fov,0)+" deg",s.cart_bottom?"On":"Off",help?"Shown":"", service_mode?(service_guns?"Aim guns":"Native menu"):"","",quit_armed?"Choose again":""};
        static_assert(std::size(values)==std::size(labels));
        for(int i=0;i<row_count;i++)p.rows.push_back({i==20&&service_mode?"Service controls":labels[i],values[i],selected==i,(i==8||i==9)&&s.reticle_mode==ReticleMode::None,i==2||i==3||((i>=7&&i<=18)&&!(s.reticle_mode==ReticleMode::None&&(i==8||i==9)))||(i==20&&native_calibration)});return p;
    }
    void choose(int row,int direction,Settings& s,SharedRuntime& runtime,double now) {
        if(row!=22)quit_armed=false;
        switch(row) {
        case 0:opened=false;help=false;break;
        case 1:recenter=true;opened=false;break;
        case 2:s.gun=(s.gun+direction+3)%3;break;
        case 3:s.gun_angle=std::clamp(s.gun_angle+direction,-30.f,30.f);break;
        case 4:break;
        case 5:s.p1_hand=1-s.p1_hand;break;
        case 6:s.laser=!s.laser;break;
        case 7:s.reticle_mode=next_reticle_mode(s.reticle_mode,direction);break;
        case 8:if(s.reticle_mode==ReticleMode::None)break;{auto& value=s.reticle_mode==ReticleMode::Custom?s.reticle_custom_min_size:s.reticle_min_size;value=std::clamp(std::round(value*100)+direction,5.f,100.f)/100;break;}
        case 9:if(s.reticle_mode==ReticleMode::None)break;{auto& value=s.reticle_mode==ReticleMode::Custom?s.reticle_custom_max_size:s.reticle_max_size;value=std::clamp(std::round(value*100)+direction*5.f,25.f,300.f)/100;break;}
        case 10:s.reticle_shrink_distance=std::clamp(std::round(s.reticle_shrink_distance*10)+direction,2.f,200.f)/10;break;
        case 11:s.haptics=HapticLevels[std::clamp(haptic_level(s.haptics)+direction,0,3)];break;
        case 12:s.world_scale=std::clamp(s.world_scale+direction*.005f,.01f,1.f);break;
        case 13:s.panel_distance=std::clamp(s.panel_distance+direction*.1f,.5f,10.f);break;
        case 14:s.hud_size=std::clamp(std::round(s.hud_size*100)+direction*5.f,50.f,200.f)/100;break;
        case 15:s.gun_scale=std::clamp(s.gun_scale+direction*.05f,.4f,1.6f);break;
        case 16:s.wider_view=!s.wider_view;break;
        case 17:s.view_fov=std::clamp(s.view_fov+direction*5.f,35.f,95.f);break;
        case 18:s.cart_bottom=!s.cart_bottom;break;
        case 19:help=!help;break;
        case 20:if(!service_mode){runtime.service_requested=true;service_seen_native=false;service_guns=false;}else if(native_calibration)service_guns=!service_guns;runtime.release_inputs();service_mode=true;opened=false;help=false;break;
        case 21:s=Settings{};runtime.release_inputs();help=false;fresh=true;break;
        case 22:if(quit_armed&&now-quit_at<3)runtime.quit_requested=true;else {quit_armed=true;quit_at=now;}break;
        }
        save(s);
    }
    void init_model() {
        if(model_program)return;
        OpenGlSource vs,fs;
        vs.addSource(R"(
in vec3 in_pos;in vec2 in_uv;in vec3 in_normal;in vec4 in_color;
uniform mat4 mvp;uniform mat4 model;out vec2 uv;out vec3 normal;out vec4 color;
void main(){gl_Position=mvp*vec4(in_pos,1);uv=in_uv;normal=mat3(model)*in_normal;color=in_color;})");
        fs.addSource(R"(
in vec2 uv;in vec3 normal;in vec4 color;uniform vec4 base;uniform sampler2D tex;uniform bool textured;out vec4 result;
void main(){vec4 c=base*color;if(textured)c*=texture(tex,uv);if(c.a<.1)discard;float light=.35+.65*max(0.,dot(normalize(normal),normalize(vec3(-.4,.8,.5))));result=vec4(c.rgb*light,c.a);})");
        model_program=gl_CompileAndLink(vs.generate().c_str(),fs.generate().c_str());
    }
};
Presentation::Presentation():impl_(std::make_unique<Impl>()){}
Presentation::~Presentation(){if(impl_->model_program)glDeleteProgram(impl_->model_program);vr::xr::termPanel();}
bool Presentation::open() const{return impl_->opened;}
bool Presentation::service() const{return impl_->service_mode;}
bool Presentation::service_guns() const{return impl_->service_guns;}
void Presentation::toggle_service_guns(){if(impl_->service_mode)impl_->service_guns=!impl_->service_guns;}
void Presentation::prepare(const Settings& s,SharedRuntime* runtime) {
    auto& p=*impl_;if(runtime)p.runtime=runtime;
    if(p.loaded_gun!=s.gun) {
        p.loaded_gun=s.gun;p.custom_failed=false;
        if(s.gun==0||!p.gun.file(s.gun==1?"models/guns/zapper.glb":"models/guns/custom.glb",s.gun==1)) {
            p.custom_failed=s.gun==2;p.gun.arcade();
        }
    }
}
void Presentation::suspend(){auto& p=*impl_;for(auto& latch:p.trigger_latch)latch.suspend();for(auto& latch:p.start_latch)latch.suspend();p.toggle_latch.suspend();p.stick_wait=true;p.input.hands={};p.input.aim_targets={};p.gun_effects.clear();p.shot_time={-100,-100};}
bool Presentation::recenter_requested(){return std::exchange(impl_->recenter,false);}
glm::vec3 Presentation::muzzle(const glm::mat4& aim,const Settings& s,int hand) const{return glm::vec3(gun_model_transform(aim,s.gun_scale,hand,impl_->gun.placement)*glm::vec4(impl_->gun.muzzle,1));}
void Presentation::feedback(const CabinetInput& in,double now){
    auto& p=*impl_;p.cabinet=in;p.startup_help.observe(in);
    // Native acknowledgements can arrive after update() has already lost tracking/focus.
    for(int player=0;player<2;player++){
        int hand=in.hand[player];
        if(in.accepted_shots[player]&&p.input.focused&&!p.opened&&!p.service_mode&&hand>=0&&hand<2&&p.input.hands[hand].tracked)p.shot_time[player]=now;
    }
}
std::array<std::optional<Vec3>,2> Presentation::reticle_targets(const GameState& state) const {
    const auto& p=*impl_;std::array<std::optional<Vec3>,2> targets;
    if(!reticles_active(state))return targets;
    // The delivered cabinet mapping owns native identity, including solo transfer.
    if(p.cabinet.hand[0]>=0&&p.cabinet.hand[0]==p.cabinet.hand[1])return targets;
    for(int player=0;player<2;player++){
        int hand=p.cabinet.hand[player];
        if(state.players[player].alive()&&hand>=0&&hand<2&&p.input.hands[hand].tracked)
            if(const auto& target=p.input.aim_targets[hand];target&&finite(*target))targets[player]=target;
    }
    return targets;
}
bool Presentation::reticles_active(const GameState& state) const {
    const auto& p=*impl_;
    return p.input.focused&&!p.opened&&!p.service_mode&&!state.service
        &&!p.startup_help.show(p.help,p.opened,p.service_mode)&&state.valid&&state.playing;
}
void Presentation::update(const PresentationInput& input,Settings& s,SharedRuntime& runtime,double now) {
    auto& p=*impl_;p.input=input;p.runtime=&runtime;
    {std::lock_guard lock(runtime.mutex);
        const auto& state=runtime.state;const bool was_service=p.service_mode;
        if(state.valid) {
            if(state.service){p.service_seen_native=true;p.service_mode=true;}
            else if(p.service_seen_native){p.service_mode=false;p.service_seen_native=false;}
            const bool busy=state.gun_calibration&&state.service_busy!=0;
            if(state.gun_calibration!=p.native_calibration||busy!=p.calibration_busy)p.service_guns=busy;
            p.native_calibration=state.gun_calibration;
            p.calibration_busy=busy;
            if(!p.service_mode)p.service_guns=false;
            if(was_service!=p.service_mode){p.shot_time={-100,-100};p.gun_effects.clear();}
        }
    }
    if(!input.focused)suspend();
    else if(!input.hands[0].tracked&&!input.hands[1].tracked){p.gun_effects.clear();p.shot_time={-100,-100};}
    bool trigger_edge[2],start_edge[2];
    for(int h=0;h<2;h++){trigger_edge[h]=p.trigger_latch[h].edge(input.hands[h].trigger,input.focused&&input.hands[h].tracked);start_edge[h]=p.start_latch[h].edge(input.hands[h].start,input.focused);}
    if(p.toggle_latch.edge(input.toggle,input.focused)) {
        p.opened=!p.opened;p.fresh=true;p.help=false;p.quit_armed=false;p.selected=0;
        auto f=glm::vec3(input.head*glm::vec4(0,0,-1,0));auto at=glm::vec3(input.head[3]);
        p.panel_model=glm::translate(glm::mat4(1),at)*glm::rotate(glm::mat4(1),std::atan2(-f.x,-f.z),glm::vec3(0,1,0))*glm::translate(glm::mat4(1),glm::vec3(0,-.1,-1.2));
    }
    if(p.opened&&input.focused) {
        if(now-p.quit_at>3)p.quit_armed=false;
        auto rows=p.rows(s);
        for(int h=0;h<2;h++)if(input.hands[h].tracked) {
            auto aim=p.help?input.poses[h]:pitched_gun_pose(input.gun_pose(h),s.gun_angle);
            auto o=p.help?glm::vec3(aim[3]):muzzle(aim,s,h),d=glm::vec3(aim*glm::vec4(0,0,-1,0));float u{};
            int row=vr::xr::rowAt(p.panel_model,rows,o,d,&u);
            if(row>=0) {if(row!=p.hovered[h])p.selected=row;if(trigger_edge[h]&&!p.fresh)p.choose(row,u<.5f?-1:1,s,runtime,now);}
            p.hovered[h]=row;
        }
        if(glm::length(input.stick)<.25f)p.stick_wait=false;
        glm::vec2 stick=p.stick_wait?glm::vec2(0):input.stick;
        bool vertical=std::abs(stick.y)>.65f,horizontal=std::abs(stick.x)>.65f;
        bool step=now>=p.repeat_at;
        if(vertical&&(std::abs(p.stick_was.y)<.65f||step)){p.selected=(p.selected+(stick.y>0?-1:1)+row_count)%row_count;p.repeat_at=now+(std::abs(p.stick_was.y)<.65f?.4:.15);p.quit_armed=false;}
        if(horizontal&&p.selected!=21&&(std::abs(p.stick_was.x)<.65f||step)){p.choose(p.selected,stick.x>0?1:-1,s,runtime,now);p.repeat_at=now+.15;}
        for(int h=0;h<2;h++)if(start_edge[h]&&!p.fresh)p.choose(p.selected,1,s,runtime,now);
        p.fresh=false;
    }
    p.stick_was=input.stick;
}
void Presentation::draw(const glm::mat4& vp,const Settings& s,const GameState& state,bool ready,double now,bool splash) {
    auto& p=*impl_;
    bool show_help=p.startup_help.show(p.help,p.opened,p.service_mode);
    if(splash||!ready) {vr::xr::Panel panel;panel.brand=true;vr::xr::drawPanel(vp,panel);if(splash)return;}
    prepare(s);
    if(show_help&&!p.controllers_loaded) {p.controllers_loaded=true;for(int h=0;h<2;h++) {if(p.controllers[h].file(h==0?"models/controllers/left.glb":"models/controllers/right.glb",false))p.controllers[h].placement=glm::rotate(glm::mat4(1),glm::pi<float>(),glm::vec3(0,1,0));else p.controllers[h].arcade();}}
    p.init_model();bool depth_cleared=false;
    struct Effect {glm::vec3 muzzle,direction,target,flash_muzzle,flash_direction;glm::vec4 tint;double since;int player;};
    std::vector<Effect> effects;effects.reserve(2);
    const bool shot_effects=!show_help&&!p.opened&&!p.service_mode&&p.input.focused;
    if(!shot_effects)p.gun_effects.clear();
    for(int player=0;player<2;player++) {
        int hand=p.opened||p.service_mode||show_help?player:p.cabinet.hand[player];if(hand<0||!p.input.hands[hand].tracked)continue;
        if(!p.opened&&!p.service_mode&&!show_help&&(!state.valid||!state.playing||!state.players[player].alive()))continue;
        auto hand_pose=show_help?p.input.poses[hand]:p.input.gun_pose(hand);double since=now-p.shot_time[player];float kick=since>=0&&since<.18?float(std::sin(since/.18*glm::pi<double>())*std::exp(-since*12)):0;
        auto rest=show_help?hand_pose:pitched_gun_pose(hand_pose,s.gun_angle);
        auto& mesh=show_help?p.controllers[hand]:p.gun;
        auto kicked=rest*glm::translate(glm::mat4(1),glm::vec3(0,0,kick*.035f))*glm::rotate(glm::mat4(1),kick*.12f,glm::vec3(1,0,0));
        auto model=show_help?hand_pose*mesh.placement:gun_model_transform(kicked,s.gun_scale,hand,mesh.placement);
        if(!depth_cleared){glDepthMask(GL_TRUE);glClear(GL_DEPTH_BUFFER_BIT);depth_cleared=true;}
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glUseProgram(p.model_program);glBindVertexArray(mesh.vao);
        auto mvp=vp*model;glUniformMatrix4fv(glGetUniformLocation(p.model_program,"mvp"),1,GL_FALSE,&mvp[0][0]);glUniformMatrix4fv(glGetUniformLocation(p.model_program,"model"),1,GL_FALSE,&model[0][0]);
        glActiveTexture(GL_TEXTURE0);glUniform1i(glGetUniformLocation(p.model_program,"tex"),0);
        for(const auto& part:mesh.mesh.parts) {
            bool textured=part.image>=0&&size_t(part.image)<mesh.textures.size();glUniform1i(glGetUniformLocation(p.model_program,"textured"),textured);if(textured)glBindTexture(GL_TEXTURE_2D,mesh.textures[part.image]);
            std::array<float,4> base;std::copy_n(part.color,4,base.begin());
            if(mesh.arcade_model&&!show_help) {
                unsigned owner=player;
                if(p.opened||p.service_mode){owner=hand==s.p1_hand?0:1;for(unsigned candidate=0;candidate<2;candidate++)if(p.cabinet.hand[candidate]==hand)owner=candidate;}
                for(unsigned index=0;index<std::size(vr::xr::gunmodel::Parts);index++)if(part.firstIndex==vr::xr::gunmodel::Parts[index].first)base=arcade_part_color(index,owner,base);
            }
            glUniform4fv(glGetUniformLocation(p.model_program,"base"),1,base.data());glDrawElements(GL_TRIANGLES,part.indexCount,GL_UNSIGNED_INT,reinterpret_cast<void*>(uintptr_t(part.firstIndex*4)));
        }
        auto muzzle=show_help?glm::vec3(hand_pose[3]):this->muzzle(rest,s,hand);auto direction=glm::normalize(glm::vec3((show_help?hand_pose:rest)*glm::vec4(0,0,-1,0)));auto target=muzzle+direction*3.f;
        if(!p.opened&&!show_help)if(auto hit=p.input.aim_targets[hand])target={float(hit->x),float(hit->y),float(hit->z)};
        if(p.opened)vr::xr::rowsPanelHit(p.panel_model,p.rows(s),muzzle,direction,&target);
        glm::vec4 tint=player==0?glm::vec4(1,.15,.2,.8):glm::vec4(.2,.5,1,.8);if(p.opened)tint={.66,.61,1,.8};
        auto flash_muzzle=glm::vec3(model*glm::vec4(mesh.muzzle,1)),flash_direction=glm::normalize(glm::vec3(kicked*glm::vec4(0,0,-1,0)));
        if(shot_effects)p.gun_effects.emit(player,p.shot_time[player],now,muzzle,direction,s.gun_scale);
        effects.push_back({muzzle,direction,target,flash_muzzle,flash_direction,tint,since,player});
    }
    // Both opaque guns establish depth before either player's translucent effects.
    for(const auto& effect:effects) {
        const auto& muzzle=effect.muzzle;const auto& direction=effect.direction;const auto& target=effect.target;const auto& tint=effect.tint;
        if((s.laser&&!show_help)||p.opened) {
            auto center=(muzzle+target)*.5f;glm::mat4 beam(1);auto side=glm::normalize(glm::cross(direction,glm::vec3(0,0,1)));if(!std::isfinite(side.x))side={1,0,0};
            beam[0]=glm::vec4(side,0);beam[1]=glm::vec4(direction,0);beam[2]=glm::vec4(glm::cross(side,direction),0);beam[3]=glm::vec4(center,1);
            vr::xr::drawTexture(vp,beam,0,.0008f,glm::length(target-muzzle)*.5f,tint,true);
            if(!reticles_active(state)||s.reticle_mode==ReticleMode::Ingame)vr::xr::drawTexture(vp,glm::translate(glm::mat4(1),target),0,.006f,.006f,tint,true);
        }
    }
    // Recover the perspective eye from VP without changing the presentation/aim interface.
    auto eye_h=glm::inverse(vp)*glm::vec4(0,0,1,0);auto eye=glm::vec3(eye_h)/eye_h.w;
    if(shot_effects) {
        p.gun_effects.smoke(vp,eye,now);
        for(const auto& e:effects)p.gun_effects.flash(vp,eye,e.player,e.since,e.flash_muzzle,e.flash_direction,s.gun_scale);
    }
    if(p.opened) {
        vr::xr::drawRowsPanel(vp,p.panel_model,p.rows(s));
    }
    if(p.service_mode&&!p.opened) {
        int width{},height{};auto text=!p.service_seen_native?"Entering native Service Menu...":p.service_guns?"AIM SET  Trigger: native aim setting   A / X: native Start   Right stick click: menu controls":"SERVICE  Stick down / B / Y: next   Trigger / A / X: Test select / exit";
        auto texture=vr::xr::textTexture(text,24,vr::xr::theme::Text,width,height);
        vr::xr::drawTexture(vp,glm::translate(glm::mat4(1),glm::vec3(0,-.65f,-1.8f)),texture,width*.00065f,height*.00065f);
    }
    // As in Confidential Mission, labels go over the scene, controllers and menu.
    if(show_help) {
        const bool dual=state.valid&&state.players[0].joined()&&state.players[1].joined();
        for(int hand=0;hand<2;hand++)if(p.input.hands[hand].tracked) {
            const int start_player=start_help_player(state,hand,s.p1_hand);
            const int gun_player=dual?start_player:(state.valid&&!state.players[0].joined()&&state.players[1].joined()?1:0);
            p.controller_labels.draw(vp,hand,p.input.poses[hand]*p.controllers[hand].placement,start_player,gun_player);
        }
    }
}
}
