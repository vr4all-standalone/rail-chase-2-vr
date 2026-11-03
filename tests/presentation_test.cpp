#include "../native/gl_context.h"
#include "../native/presentation.h"
#include "../native/launch_policy.h"
#include "../native/settings_io.h"
#include "../native/shared/glb_mesh.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
using namespace rchase2;
static void check(bool ok,const char* why){if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
int main(int argc,char** argv){
    if(argc!=2)return 2;_putenv_s("RCHASE2_ASSETS",argv[1]);
    for(const char* name:{"models/controllers/left.glb","models/controllers/right.glb"}) {
        std::ifstream in(std::filesystem::path(argv[1])/name,std::ios::binary);
        std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
        vr::xr::GlbMesh mesh;std::string error;
        check(!bytes.empty()&&vr::xr::loadGlb(bytes.data(),bytes.size(),mesh,error,8,1024),"bundled controller GLBs load through the production loader");
        check(mesh.positions.size()>300&&!mesh.images.empty(),"controller GLBs contain their original geometry and textures");
    }
    GLContext context;
    {
        _putenv_s("RCHASE2_SETTINGS","hud-menu-test.ini");Presentation hudmenu;Settings values;SharedRuntime isolated;PresentationInput buttons;double tick=0;buttons.hands[0].tracked=true;buttons.poses[0][3].x=10;
        auto update=[&]{tick+=.05;hudmenu.update(buttons,values,isolated,tick);};
        auto step=[&](glm::vec2 direction){buttons.sticks[0]=direction;update();buttons.sticks[0]={};update();};
        update();buttons.toggle=true;update();buttons.toggle=false;update();
        auto press=[&]{buttons.hands[0].start=true;update();buttons.hands[0].start=false;update();};
        step({0,1});step({1,0});step({0,-1});step({0,-1});press();
        step({0,-1});step({0,-1});
        step({1,0});check(values.hud_size==1.05f,"HUD size immediately after distance changes in5% steps");
        for(int i=0;i<30;i++)step({1,0});check(values.hud_size==2,"HUD size clamps at200%");
        for(int i=0;i<40;i++)step({-1,0});check(values.hud_size==.5f,"HUD size clamps at50%");
        std::ifstream file("hud-menu-test.ini");auto cold=read_settings(file);file.close();check(cold.hud_size==.5f,"actual HUD menu save/load");
        check(values.world_scale==Settings{}.world_scale&&values.panel_distance==Settings{}.panel_distance&&values.p1_hand==Settings{}.p1_hand,"HUD menu does not change world, distance or hand");
        for(int i=0;i<3;i++)step({0,1});press();
        step({0,1});step({1,0});step({0,-1});step({1,0});
        check(values.p1_hand==1&&values.hud_size==.5f,"hand menu transition retains HUD preference");
        for(int i=0;i<3;i++)step({0,-1});press();step({0,-1});press();
        check(values.hud_size==1&&values.p1_hand==Settings{}.p1_hand,"Reset restores HUD100 and hand defaults");
        std::ifstream resetfile("hud-menu-test.ini");auto resetcold=read_settings(resetfile);resetfile.close();check(resetcold.hud_size==1,"actual Reset persists HUD100");
        std::filesystem::remove("hud-menu-test.ini");_putenv_s("RCHASE2_SETTINGS","");
    }

    {
        auto settings_file=std::filesystem::current_path()/"reticle-menu-test.ini";
        const char* previous=std::getenv("RCHASE2_SETTINGS");std::string previous_name=previous?previous:"";
        _putenv_s("RCHASE2_SETTINGS",settings_file.string().c_str());
        Presentation tuning;PresentationInput buttons;Settings values;SharedRuntime isolated;double tick=0;buttons.hands[0].tracked=true;buttons.poses[0][3].x=10;
        tuning.update(buttons,values,isolated,tick++);buttons.toggle=true;tuning.update(buttons,values,isolated,tick++);
        buttons.toggle=false;tuning.update(buttons,values,isolated,tick++);
        auto step=[&](glm::vec2 stick){buttons.sticks[0]=stick;tuning.update(buttons,values,isolated,tick++);buttons.sticks[0]={};tuning.update(buttons,values,isolated,tick++);};
        auto press=[&]{buttons.hands[0].start=true;tuning.update(buttons,values,isolated,tick++);buttons.hands[0].start=false;tuning.update(buttons,values,isolated,tick++);};
        for(int i=0;i<3;i++)step({0,-1});
        step({1,0});check(values.reticle_mode==ReticleMode::Custom,"Reticle selector cycles from In-game to Custom");
        step({1,0});check(values.reticle_mode==ReticleMode::None,"Reticle selector includes None");
        step({0,-1});step({0,-1});press();step({0,-1});step({1,0});step({0,-1});step({1,0});
        check(values.reticle_min_size==.35f&&values.reticle_max_size==1.f
            &&values.reticle_custom_min_size==.35f&&values.reticle_custom_max_size==1.f,
            "None artwork cannot change either mode's saved size");
        for(int i=0;i<3;i++)step({0,1});press();
        for(int i=0;i<3;i++)step({0,-1});step({-1,0});check(values.reticle_mode==ReticleMode::Custom,"Reticle selector cycles backward");
        step({0,-1});step({0,-1});press();step({0,-1});
        step({1,0});check(values.reticle_custom_min_size==.36f&&values.reticle_min_size==.35f,"Custom minimum changes without changing In-game minimum");
        step({0,-1});step({1,0});
        check(values.reticle_custom_max_size==1.05f&&values.reticle_max_size==1.f,"Custom maximum changes in five-percent steps without changing In-game maximum");
        step({0,-1});step({1,0});
        check(values.reticle_shrink_distance==6.3f,"menu Reticle shrink distance changes its own saved value in tenths of a metre");
        check(values.haptics==.65f&&values.world_scale==.055f,"reticle controls do not alter haptics or world size after inserting menu rows");
        std::ifstream saved(settings_file);auto restored=read_settings(saved);saved.close();
        check(restored.reticle_mode==ReticleMode::Custom&&restored.reticle_custom_min_size==.36f&&restored.reticle_custom_max_size==1.05f&&restored.reticle_shrink_distance==6.3f,"actual menu edits survive production settings save/load");
        std::filesystem::remove(settings_file);_putenv_s("RCHASE2_SETTINGS",previous_name.c_str());
    }

    {
        const auto saved=std::filesystem::current_path()/"cart-bottom-menu-test.ini";
        _putenv_s("RCHASE2_SETTINGS",saved.string().c_str());
        Presentation menu;Settings option;SharedRuntime bridge;PresentationInput controls;double time=0;controls.hands[0].tracked=true;controls.poses[0][3].x=10;
        auto update=[&](){time+=.05;menu.update(controls,option,bridge,time);};
        update();controls.toggle=true;update();controls.toggle=false;update();
        auto step=[&](glm::vec2 stick){controls.sticks[0]=stick;update();controls.sticks[0]={};update();};
        auto press=[&]{controls.hands[0].start=true;update();controls.hands[0].start=false;update();};
        auto tab=[&](int right){step({0,1});for(int i=0;i<right;i++)step({1,0});step({0,-1});};
        tab(1);step({0,-1});press();for(int i=0;i<4;i++)step({0,-1});step({1,0});
        check(!option.cart_bottom,"Cart bottom toggles Off in Advanced World");
        std::ifstream file(saved);auto cold=read_settings(file);file.close();
        check(!cold.cart_bottom,"the actual menu saves Cart bottom Off for the next launch");
        for(int i=0;i<5;i++)step({0,1});press();tab(1);
        step({0,-1});press();check(bridge.service_requested&&!menu.open(),"Service sends native request");
        controls.toggle=true;update();controls.toggle=false;update();tab(2);
        for(int i=0;i<4;i++)step({0,-1});press();check(!bridge.quit_requested,"Quit requires confirmation");
        step({0,-1});press();check(bridge.quit_requested,"Quit confirms on the second selection");
        std::filesystem::remove(saved);_putenv_s("RCHASE2_SETTINGS","");
    }
    {
        const auto saved=std::filesystem::current_path()/"reset-menu-test.ini";
        _putenv_s("RCHASE2_SETTINGS",saved.string().c_str());
        Presentation menu;Settings option;SharedRuntime bridge;PresentationInput controls;double time=0;controls.hands[0].tracked=true;controls.poses[0][3].x=10;
        option.laser=false;option.reticle_mode=ReticleMode::Custom;
        option.reticle_custom_min_size=.48f;option.reticle_custom_max_size=2.f;
        option.cart_bottom=false;option.p1_hand=1;option.hud_size=2;
        auto update=[&](){time+=.05;menu.update(controls,option,bridge,time);};
        update();controls.toggle=true;update();controls.toggle=false;update();
        auto step=[&](glm::vec2 stick){controls.sticks[0]=stick;update();controls.sticks[0]={};update();};
        auto press=[&]{controls.hands[0].start=true;update();controls.hands[0].start=false;update();};
        step({0,1});step({1,0});step({1,0});for(int i=0;i<4;i++)step({0,-1});
        press();step({0,-1});press();
        check(option.laser==Settings{}.laser&&option.reticle_mode==Settings{}.reticle_mode
            &&option.reticle_custom_min_size==Settings{}.reticle_custom_min_size
            &&option.reticle_custom_max_size==Settings{}.reticle_custom_max_size
            &&option.hud_size==1&&option.cart_bottom==Settings{}.cart_bottom&&option.p1_hand==Settings{}.p1_hand,
            "Reset to defaults restores the game's VR settings including both artwork sizes");
        check(menu.open()&&!bridge.quit_requested&&!bridge.service_requested,
            "Reset stays in the menu without requesting Quit or native Service");
        std::ifstream file(saved);auto cold=read_settings(file);file.close();
        check(cold.reticle_mode==Settings{}.reticle_mode
            &&cold.reticle_custom_max_size==Settings{}.reticle_custom_max_size
            &&cold.hud_size==1&&cold.p1_hand==Settings{}.p1_hand,"Reset persists across a settings reload");
        std::filesystem::remove(saved);_putenv_s("RCHASE2_SETTINGS","");
    }
    Presentation ui;Settings settings;SharedRuntime runtime;PresentationInput input;
    ui.update(input,settings,runtime,0);input.toggle=true;ui.update(input,settings,runtime,.1);
    check(ui.open(),"menu opens while the arcade engine is loading");
    GLuint image{},fbo{},depth{};glGenTextures(1,&image);glBindTexture(GL_TEXTURE_2D,image);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,256,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,image,0);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,256,256);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    check(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"presentation probe framebuffer is complete");
    glViewport(0,0,256,256);auto vp=glm::perspective(glm::radians(70.f),1.f,.05f,100.f);
    auto render=[&](bool splash){
        glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);ui.draw(vp,settings,GameState{},false,.2,splash);
        std::vector<uint8_t> pixels(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());return pixels;
    };
    auto brand=render(true),menu=render(false);size_t changed=0;
    for(size_t i=0;i<brand.size();i+=4)if(brand[i]!=menu[i]||brand[i+1]!=menu[i+1]||brand[i+2]!=menu[i+2])++changed;
    check(changed>1000,"after the timed splash, an open loading menu must draw over the brand image");
    {
        // Native credits arrive during mode 11, before Start is permitted.
        // Preserve the already-drawn native image instead of covering it again.
        Presentation opening;NativeFrame credits;credits.complete=true;credits.width=496;credits.height=384;
        credits.native.assign(size_t(credits.width)*credits.height,0xffffffff);
        GameState boot;boot.valid=true;boot.mode=11;
        LaunchPolicy launch;launch.observe(&credits);
        glClearColor(.1f,.2f,.3f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        std::vector<uint8_t> before(256*256*4),after(before.size());
        glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,before.data());
        opening.draw(vp,settings,boot,native_frame_presentable(&credits),.2,launch.active());
        glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,after.data());
        check(before==after,"opening credits remain visible before the game becomes Start-ready");
    }
    {
        Presentation instructions;PresentationInput hands;Settings help;GameState game;game.valid=game.ready=true;
        for(int h=0;h<2;h++){hands.hands[h].tracked=true;hands.poses[h]=glm::translate(glm::mat4(1),glm::vec3(h==0?-.17f:.17f,0,-.6f));}
        instructions.update(hands,help,runtime,.3);
        glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        instructions.draw(vp,help,game,true,.3);
        std::vector<uint8_t> pixels(256*256*4);std::vector<float> depths(256*256);
        glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());glReadPixels(0,0,256,256,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());
        size_t lit[2]{},labels[2]{};
        for(int y=0;y<256;y++)for(int x=0;x<256;x++) {
            size_t i=y*256+x;int h=x/128;
            if(depths[i]<.999f&&(pixels[i*4]||pixels[i*4+1]||pixels[i*4+2]))lit[h]++;
            if(depths[i]>.999f&&pixels[i*4]>90&&pixels[i*4+1]>90&&pixels[i*4+2]>90)labels[h]++;
        }
        check(lit[0]>100&&lit[1]>100,"startup shows both textured tracked controllers");
        check(labels[0]>20&&labels[1]>20,"button-anchored instructional labels are visible beside both controllers");
        runtime.state.valid=true;runtime.state.service=true;
        instructions.update(hands,help,runtime,.4);check(instructions.service()&&!instructions.service_guns(),"native Service acknowledgement keeps list navigation separate from gun aiming");
        runtime.state.gun_calibration=true;runtime.state.service_busy=1;
        instructions.update(hands,help,runtime,.5);check(instructions.service_guns(),"actual busy native gun action enables gun-calibration input mode");
        instructions.toggle_service_guns();check(!instructions.service_guns(),"gun-calibration aiming can return to native Test/Service controls");
        runtime.state={};runtime.state.valid=true;runtime.state.ready=true;
        instructions.update(hands,help,runtime,.6);check(!instructions.service()&&!instructions.service_guns(),"only native Service exit acknowledgement returns to game controls");
    }
    {
        Presentation guns;PresentationInput hands;Settings effects;SharedRuntime isolated;
        GameState game;game.valid=game.playing=game.ready=true;
        for(auto& p:game.players){p.state=1;p.health=4;}
        CabinetInput cabinet;cabinet.hand={0,1};cabinet.cabinet.coins=1;guns.feedback(cabinet,0);
        for(int h=0;h<2;h++){
            hands.hands[h].tracked=true;
            hands.poses[h]=glm::translate(glm::mat4(1),glm::vec3(h==0?-.16f:.16f,0,-.65f));
            hands.aim_targets[h]=Vec3{h==0?-.25:.25,.5,-2};
        }
        guns.update(hands,effects,isolated,1);
        auto capture=[&](ReticleMode mode,bool laser){
            effects.reticle_mode=mode;effects.laser=laser;
            glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            guns.draw(vp,effects,game,true,1,false);
            std::vector<uint8_t> pixels(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());return pixels;
        };
        auto original=capture(ReticleMode::Ingame,true),custom=capture(ReticleMode::Custom,true),none=capture(ReticleMode::None,true);
        check(original!=custom&&custom==none,"Custom and None remove auxiliary endpoint squares from both live gun rays");
        check(custom!=capture(ReticleMode::Custom,false),"removing endpoint squares keeps the independently controlled laser beams");
        for(int y=0;y<256;y++)for(int x=0;x<256;x++){
            size_t at=(y*256+x)*4;
            if(original[at]!=custom[at]||original[at+1]!=custom[at+1]||original[at+2]!=custom[at+2])
                check(y>=170&&y<=177&&((x>=101&&x<=109)||(x>=147&&x<=155)),"mode changes only the two projected endpoint squares");
        }
        game.service=true;isolated.state=game;guns.update(hands,effects,isolated,2);
        check(capture(ReticleMode::Custom,true)==capture(ReticleMode::Ingame,true),"Service pointers survive the gameplay reticle selector");
    }
    {
        Presentation guns;PresentationInput hands;hands.focused=true;
        GameState game;game.valid=true;game.ready=true;game.playing=true;game.mode=2;for(auto& player:game.players){player.state=1;player.health=4;}
        CabinetInput cabinet;cabinet.hand={0,1};cabinet.cabinet.coins=1;guns.feedback(cabinet,0);
        for(int h=0;h<2;h++){hands.hands[h].tracked=true;hands.poses[h]=glm::translate(glm::mat4(1),glm::vec3(h==0?-.16f:.16f,0,-.65f));}
        guns.update(hands,settings,runtime,.3);settings.laser=false;
        glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        guns.draw(vp,settings,game,true,1,false);
        std::vector<uint8_t> before(256*256*4);std::vector<float> depths(256*256);
        glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,before.data());glReadPixels(0,0,256,256,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());
        std::array<std::array<int,2>,2> picks{};
        for(int h=0;h<2;h++) {
            bool found=false;
            for(int y=125;y>=70&&!found;y--)for(int x=h*128+12;x<(h+1)*128-12&&!found;x++){
                bool solid=true;for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++)if(depths[(y+dy)*256+x+dx]>=.999f)solid=false;
                if(!solid)continue;picks[h]={x,y};found=true;
                auto point=glm::inverse(vp)*glm::vec4((x+.5f)/128.f-1,(y+.5f)/128.f-1,depths[y*256+x]*2-1,1);
                auto behind=glm::vec3(point)/point.w*2.f;hands.aim_targets[h]=Vec3{behind.x,behind.y,behind.z};
            }
            check(found,"laser occlusion fixture has a solid interior gun pixel in each hand");
        }
        guns.update(hands,settings,runtime,1);settings.laser=true;
        glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);guns.draw(vp,settings,game,true,1,false);
        std::vector<uint8_t> after(before.size());glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,after.data());
        std::vector<float> after_depth(depths.size());glReadPixels(0,0,256,256,GL_DEPTH_COMPONENT,GL_FLOAT,after_depth.data());
        check(depths==after_depth,"translucent laser effects cannot write depth over either gun or the scene");
        for(auto xy:picks){size_t at=(xy[1]*256+xy[0])*4;check(before[at]==after[at]&&before[at+1]==after[at+1]&&before[at+2]==after[at+2],"laser endpoint behind either gun cannot draw through its solid body");}
    }
    {
        Presentation guns;PresentationInput hands;Settings effects;effects.laser=false;
        GameState game;game.valid=true;game.ready=true;game.playing=true;game.mode=2;for(auto& player:game.players){player.state=1;player.health=4;}
        CabinetInput cabinet;cabinet.hand={0,1};cabinet.cabinet.coins=1;guns.feedback(cabinet,0);
        for(int h=0;h<2;h++){
            hands.hands[h].tracked=true;
            hands.poses[h]=glm::translate(glm::mat4(1),glm::vec3(h==0?-.16f:.16f,0,-.65f))*glm::rotate(glm::mat4(1),glm::radians(70.f),glm::vec3(0,1,0));
        }
        guns.update(hands,effects,runtime,1);cabinet.accepted_shots={1,1};guns.feedback(cabinet,1);
        struct Frame {std::vector<uint8_t> rgb;std::vector<float> depth;};
        unsigned capture=0;
        auto frame=[&](const glm::mat4& view,double now){
            glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            guns.draw(view,effects,game,true,now,false);Frame f{std::vector<uint8_t>(256*256*4),std::vector<float>(256*256)};
            glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,f.rgb.data());glReadPixels(0,0,256,256,GL_DEPTH_COMPONENT,GL_FLOAT,f.depth.data());
            if(const char* directory=std::getenv("RCHASE2_EFFECT_PROBE")){
                std::ofstream out(std::filesystem::path(directory)/(std::to_string(capture++)+".ppm"),std::ios::binary);out<<"P6\n256 256\n255\n";
                for(int y=255;y>=0;y--)for(int x=0;x<256;x++)out.write(reinterpret_cast<const char*>(&f.rgb[(y*256+x)*4]),3);
            }
            return f;
        };
        bool flash_visible=true;
        for(float x:{-.032f,.032f}){
            auto f=frame(vp*glm::lookAt(glm::vec3(x,.08f,.04f),glm::vec3(0,0,-.65f),glm::vec3(0,1,0)),1.045);
            size_t light=0;for(size_t i=0;i<f.depth.size();i++)if(f.depth[i]>.9999f&&f.rgb[i*4]>30&&f.rgb[i*4]>f.rgb[i*4+1]*1.1)++light;
            flash_visible=flash_visible&&light>20;
        }
        // Move both guns away: smoke must remain in the room at the previous shot.
        for(auto& pose:hands.poses)pose[3].x+=.6f;
        guns.update(hands,effects,runtime,1.2);
        auto smoke=frame(vp,1.2),again=frame(vp,1.2);
        check(smoke.rgb==again.rgb&&smoke.depth==again.depth,"stereo/repeated draws do not emit extra smoke or change depth");
        size_t puffs=0;
        for(int y=60;y<180;y++)for(int x=12;x<170;x++){
            size_t i=y*256+x;auto r=smoke.rgb[i*4],g=smoke.rgb[i*4+1],b=smoke.rgb[i*4+2];
            if(smoke.depth[i]>.9999f&&r>2&&std::abs(int(r)-g)<4&&std::abs(int(g)-b)<4)++puffs;
        }
        check(puffs>20,"soft smoke remains at the shot location after the guns move away");
        check(flash_visible,"shaped flash is visible outside angled barrels in both eyes after 35ms");
        auto expired=frame(vp,2.0);size_t lingering=0;
        for(size_t i=0;i<expired.depth.size();i++)if(expired.depth[i]>.9999f&&(expired.rgb[i*4]||expired.rgb[i*4+1]||expired.rgb[i*4+2]))++lingering;
        check(lingering==0,"smoke expires instead of leaving persistent room-space artifacts");
        for(int h=0;h<2;h++)hands.poses[h][3].x=h==0?-.16f:.16f;
        guns.update(hands,effects,runtime,3);guns.feedback(cabinet,3);frame(vp,3.01);
        for(auto& hand:hands.hands)hand.tracked=false;
        guns.update(hands,effects,runtime,3.02);guns.feedback(cabinet,3.02);auto lost=frame(vp,3.03);size_t stale=0;
        for(size_t i=0;i<lost.rgb.size();i+=4)if(lost.rgb[i]||lost.rgb[i+1]||lost.rgb[i+2])++stale;
        check(stale==0,"losing both tracked hands clears stale room-space gun effects");
        auto no_effects=[&](const Frame& f){
            for(size_t i=0;i<f.depth.size();i++)if(f.depth[i]>.9999f&&(f.rgb[i*4]||f.rgb[i*4+1]||f.rgb[i*4+2]))return false;
            return true;
        };
        for(auto& hand:hands.hands)hand.tracked=true;
        guns.update(hands,effects,runtime,3.04);
        check(no_effects(frame(vp,3.04)),"tracking recovery cannot replay the old flash or re-emit its smoke");
        guns.update(hands,effects,runtime,4);guns.feedback(cabinet,4);frame(vp,4.01);
        hands.focused=false;guns.update(hands,effects,runtime,4.02);guns.feedback(cabinet,4.02);frame(vp,4.03);
        hands.focused=true;guns.update(hands,effects,runtime,4.04);
        check(no_effects(frame(vp,4.04)),"focus recovery cannot replay stale accepted-shot effects");
        hands.hands[0].tracked=false;guns.update(hands,effects,runtime,5);cabinet.accepted_shots={1,0};guns.feedback(cabinet,5);frame(vp,5.01);
        hands.hands[0].tracked=true;guns.update(hands,effects,runtime,5.02);
        check(no_effects(frame(vp,5.02)),"a late shot for one untracked owning hand cannot replay on recovery");
        runtime.state=game;runtime.state.service=true;runtime.state.ready=runtime.state.playing=false;
        guns.update(hands,effects,runtime,6);auto service_before=frame(vp,6.045);
        guns.feedback(cabinet,6);auto service_after=frame(vp,6.045);
        check(service_before.rgb==service_after.rgb,"a late accepted-shot event cannot recoil or flash after native Service acknowledgement");
    }
    check(glGetError()==GL_NO_ERROR,"presentation GL operations complete without errors");
    glDeleteRenderbuffers(1,&depth);
    glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&image);
    std::cout<<"Loading menu remains visible after the shared timed splash\n";
}
