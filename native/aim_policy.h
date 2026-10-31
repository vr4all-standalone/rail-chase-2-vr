// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "game_state.h"
#include "input_policy.h"
namespace rchase2 {
class NativeAimPolicy {
    std::array<bool,2> blocked_{};
public:
    void suspend(){blocked_={true,true};}
    void apply(CabinetInput& input,const Hands& hands,const std::array<NativeAimCalibration,2>& calibration) {
        for(unsigned player=0;player<2;player++) {
            const int hand=input.hand[player];
            if(hand<0||hand>1){input.cabinet.trigger[player]=false;input.accepted_shots[player]=0;continue;}
            const auto& source=hands[hand];
            const auto aim=native_screen_to_adc(player,source.aim.u,source.aim.v,calibration[player]);
            if(source.tracked&&source.aim.valid&&aim.valid) {
                input.cabinet.x[player]=aim.x;input.cabinet.y[player]=aim.y;
                // A release counts only after the source mapping has recovered.
                if(!source.trigger)blocked_[hand]=false;
            } else blocked_[hand]=true;
            if(blocked_[hand]){input.cabinet.trigger[player]=false;input.accepted_shots[player]=0;}
        }
    }
};
}
