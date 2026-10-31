#include "../native/settings_io.h"
#include <cassert>
#include <sstream>
using namespace rchase2;
int main(){
    std::ostringstream accepted_defaults;write_settings(accepted_defaults,Settings{});
    assert(accepted_defaults.str().find("world_scale=0.055\n")!=std::string::npos);
    assert(accepted_defaults.str().find("view_fov=55\n")!=std::string::npos);
    assert(accepted_defaults.str().find("reticle_min_size=0.35\n")!=std::string::npos);
    assert(accepted_defaults.str().find("reticle_shrink_distance=6.2\n")!=std::string::npos);
    // A newly introduced control must survive production save/load, without
    // migrating unrelated explicit choices back to their defaults.
    {std::istringstream previous("version=3\ngun_angle_zero_version=1\ncart_bottom=0\np1_hand=1\ngun_angle=12\nwider_view=0\ngun=2\npanel_distance=2.4\n");
     bool upgraded=false;auto options=read_settings(previous,&upgraded);
     assert(upgraded);
     std::ostringstream saved;write_settings(saved,options);
     assert(saved.str().find("reticle_mode=0\n")!=std::string::npos);
     assert(saved.str().find("reticle_min_size=0.35\n")!=std::string::npos);
     assert(saved.str().find("reticle_shrink_distance=2.4\n")!=std::string::npos);
     std::istringstream next(saved.str());auto restored=read_settings(next,&upgraded);
     assert(!upgraded&&!restored.cart_bottom&&restored.p1_hand==1&&restored.gun_angle==12&&!restored.wider_view&&restored.gun==2);}
    {std::istringstream input("version=4\ngun_angle_zero_version=1\nreticle_mode=1\nreticle_min_size=0.55\nreticle_shrink_distance=4.2\n");
     auto value=read_settings(input);assert(value.reticle_mode==ReticleMode::Custom&&value.reticle_min_size==.55f&&value.reticle_shrink_distance==4.2f);
     for(auto mode:{ReticleMode::Ingame,ReticleMode::Custom,ReticleMode::None}){
        value.reticle_mode=mode;std::ostringstream saved;write_settings(saved,value);std::istringstream cold(saved.str());
        bool migrated=true;auto loaded=read_settings(cold,&migrated);assert(!migrated&&loaded.reticle_mode==mode&&loaded.reticle_min_size==.55f&&loaded.reticle_shrink_distance==4.2f);
     }}
    for(const char* invalid:{"-1","3","0.5","nan","inf","1x",""}){
        std::istringstream input(std::string("reticle_mode=")+invalid+"\n");assert(read_settings(input).reticle_mode==ReticleMode::Ingame);
    }
    {std::istringstream input("reticle_min_size=nan\nreticle_shrink_distance=999\npanel_distance=2.4\n");auto value=read_settings(input);assert(value.reticle_min_size==.35f&&value.reticle_shrink_distance==2.4f);}
    // A missing new option must be written as On for legacy/fresh installs.
    {std::istringstream legacy_cart("version=3\ngun_angle_zero_version=1\n");
     std::ostringstream saved;write_settings(saved,read_settings(legacy_cart));
     assert(saved.str().find("cart_bottom=1\n")!=std::string::npos);}
    assert(Settings{}.cart_bottom);
    for(const char* invalid:{"-1","2","0.5","nan","inf","1x",""}) {
        std::istringstream input(std::string("cart_bottom=")+invalid+"\n");assert(read_settings(input).cart_bottom);
    }
    {std::istringstream disabled("version=3\ngun_angle_zero_version=1\ncart_bottom=0\nworld_scale=0.25\np1_hand=1\ngun_angle=12\n");
     auto cart=read_settings(disabled);assert(!cart.cart_bottom&&cart.world_scale==.25f&&cart.p1_hand==1&&cart.gun_angle==12);
     std::ostringstream saved;write_settings(saved,cart);std::istringstream cold_start(saved.str());
     assert(!read_settings(cold_start).cart_bottom);}
    std::istringstream old_angle("version=3\ngun_angle=20\n");
    bool angle_migrated=false;auto reset_angle=read_settings(old_angle,&angle_migrated);
    assert(reset_angle.gun_angle==0&&angle_migrated);
    std::ostringstream reset_saved;write_settings(reset_saved,reset_angle);
    assert(reset_saved.str().find("gun_angle_zero_version=1\n")!=std::string::npos);
    std::istringstream cold_start(reset_saved.str());angle_migrated=true;
    auto cold=read_settings(cold_start,&angle_migrated);assert(cold.gun_angle==0&&!angle_migrated);
    cold.gun_angle=12;std::ostringstream manual;write_settings(manual,cold);
    std::istringstream manual_start(manual.str());
    assert(read_settings(manual_start,&angle_migrated).gun_angle==12&&!angle_migrated);
    std::istringstream new_angle("version=3\ngun_angle_zero_version=1\ngun_angle=-17\n");
    auto angle=read_settings(new_angle);assert(angle.gun_angle==-17);
    std::ostringstream angle_saved;write_settings(angle_saved,angle);
    std::istringstream angle_next(angle_saved.str());assert(read_settings(angle_next).gun_angle==-17);
    assert(angle_saved.str().find("gun_angle_zero_version=1\n")!=std::string::npos);
    assert(Settings{}.p1_hand==0);
    assert(Settings{}.wider_view);
    std::istringstream legacy("version=1\nwider_view=0\nworld_scale=0.25\n");
    auto migrated=read_settings(legacy);
    assert(migrated.wider_view && migrated.world_scale==.25f);
    std::istringstream fresh("laser=0\n");
    assert(read_settings(fresh).wider_view);
    std::istringstream current("wider_view=0\nversion=2\n");
    auto selected=read_settings(current);assert(!selected.wider_view);
    std::ostringstream output;write_settings(output,selected);
    assert(output.str().find("version=4\n")==0);
    std::istringstream roundtrip(output.str());assert(!read_settings(roundtrip).wider_view);
    std::istringstream invalid("version=2\nwider_view=nan\n");
    assert(read_settings(invalid).wider_view);
    std::istringstream old_hands("version=2\np1_hand=1\nwider_view=0\nworld_scale=0.25\n");
    auto hand_migration=read_settings(old_hands);
    assert(hand_migration.p1_hand==0&&!hand_migration.wider_view&&hand_migration.world_scale==.25f);
    // Once upgraded, an explicit reversed assignment survives later launches.
    hand_migration.p1_hand=1;
    std::ostringstream reversed;write_settings(reversed,hand_migration);
    std::istringstream reloaded(reversed.str());
    assert(read_settings(reloaded).p1_hand==1);
    std::istringstream current_hands("p1_hand=1\nversion=3\ngun_angle_zero_version=1\n");
    bool upgraded=true;assert(read_settings(current_hands,&upgraded).p1_hand==1&&upgraded);
    std::istringstream needs_upgrade("version=2\np1_hand=1\n");
    assert(read_settings(needs_upgrade,&upgraded).p1_hand==0&&upgraded);
}
