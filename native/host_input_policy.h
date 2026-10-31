// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "input_policy.h"
#include "credit_policy.h"
namespace rchase2 {
// Called directly from the non-FOCUSED OpenXR session-event path, including
// STOPPING transitions that have no intervening unfocused controls frame.
inline void suspend_host_cabinet_policies(InputPolicy& input,NativeCreditPolicy& credits) {
    input.suspend();
    credits.suspend();
}
inline unsigned start_help_player(const GameState& state,int hand,int p1_hand) {
    const bool joined=state.valid&&(state.players[0].joined()||state.players[1].joined());
    if(!joined) {
        if(state.valid&&state.played_mask==2)return 1;
        if(!state.valid||state.played_mask!=3)return 0;
    }
    return hand==(p1_hand==0?0:1)?0:1;
}
}
