// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
namespace rchase2 {
enum class ReticleMode {Ingame=0,Custom=1,None=2};
inline ReticleMode reticle_mode_value(int value){return value>=0&&value<=2?ReticleMode(value):ReticleMode::Ingame;}
inline ReticleMode next_reticle_mode(ReticleMode mode,int direction){return ReticleMode((int(reticle_mode_value(int(mode)))+direction%3+3)%3);}
inline const char* reticle_mode_label(ReticleMode mode){
    switch(reticle_mode_value(int(mode))){case ReticleMode::Custom:return "Custom";case ReticleMode::None:return "None";default:return "In-game";}
}
struct ReticleDrawPolicy {bool suppress_native{},draw_native{},draw_custom{};};
inline ReticleDrawPolicy reticle_draw_policy(ReticleMode mode,bool gameplay,bool identified,bool target){
    if(!gameplay)return {};
    switch(reticle_mode_value(int(mode))){
        case ReticleMode::None:return {identified,false,false};
        case ReticleMode::Custom:return {identified,false,target};
        default:return {identified&&target,identified&&target,false};
    }
}
}
