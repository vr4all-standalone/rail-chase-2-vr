// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/host_input_policy.h"
#include <cassert>
using namespace rchase2;
static Hands neutral() {
    Hands out;
    for(auto& hand:out){hand.tracked=true;hand.aim={.5,.5,true,true};}
    return out;
}
static GameState game(uint8_t joined=0,uint8_t played=0) {
    GameState state;state.valid=state.ready=state.pricing_valid=true;
    state.session=42;state.start_cost={2,2};state.played_mask=played;
    for(unsigned p=0;p<2;p++)state.players[p].state=(joined>>p)&1;
    return state;
}
static void stopping_without_an_unfocused_controls_frame(bool paid_on_resume) {
    InputPolicy inputs;NativeCreditPolicy credits;Settings settings;
    auto state=game();auto hands=neutral();
    auto desired=inputs.update(0,hands,state,settings,true,false);
    credits.update(0,desired.cabinet,state,true);
    hands[0].start=hands[0].trigger=true;
    desired=inputs.update(.01,hands,state,settings,true,false);
    auto pending=credits.update(.01,desired.cabinet,state,true);
    assert(pending.coins==1&&pending.starts==0);

    // This is the production non-FOCUSED session-event operation. No call to
    // controls(false) occurs between STOPPING and the resumed focused frame.
    suspend_host_cabinet_policies(inputs,credits);
    state.credits=paid_on_resume?2:0;
    desired=inputs.update(.32,hands,state,settings,true,false);
    auto resumed=credits.update(.32,desired.cabinet,state,true);
    assert(resumed.coins==0&&resumed.starts==0);
    assert(!desired.cabinet.trigger[0]&&!desired.cabinet.trigger[1]);
    desired=inputs.update(.63,hands,state,settings,true,false);
    resumed=credits.update(.63,desired.cabinet,state,true);
    assert(resumed.coins==0&&resumed.starts==0);

    // A release followed by a genuinely new Start request still works.
    hands=neutral();desired=inputs.update(.7,hands,state,settings,true,false);
    credits.update(.7,desired.cabinet,state,true);
    hands[0].start=true;desired=inputs.update(.71,hands,state,settings,true,false);
    assert(desired.cabinet.coins==1);
    resumed=credits.update(.71,desired.cabinet,state,true);
    if(!paid_on_resume)assert(resumed.coins==1&&resumed.starts==0);
    else {
        desired=inputs.update(.87,hands,state,settings,true,false);
        resumed=credits.update(.87,desired.cabinet,state,true);
        assert(resumed.coins==0&&resumed.starts==1);
    }
}
static void labels_match_production_start_requests() {
    for(int layout:{0,1})for(int hand:{0,1}) {
        assert(start_help_player(game(),hand,layout)==0);
        assert(start_help_player(game(0,1),hand,layout)==0);
        assert(start_help_player(game(0,2),hand,layout)==1);
    }

    for(int layout:{0,1})for(uint8_t joined:{0,1,2,3})for(uint8_t played:{0,1,2,3})for(int hand:{0,1}) {
        Settings settings;settings.p1_hand=layout;auto state=game(joined,played);
        auto hands=neutral();InputPolicy inputs;
        inputs.update(0,hands,state,settings,true,false);
        hands[hand].start=true;
        auto desired=inputs.update(.01,hands,state,settings,true,false);
        assert(start_help_player(state,hand,layout)<2);
        if(desired.cabinet.coins)assert(desired.cabinet.coins==(1u<<start_help_player(state,hand,layout)));
        desired=inputs.update(.17,hands,state,settings,true,false);
        if(desired.cabinet.starts)assert(desired.cabinet.starts==(1u<<start_help_player(state,hand,layout)));
    }
}
int main() {
    stopping_without_an_unfocused_controls_frame(false);
    stopping_without_an_unfocused_controls_frame(true);
    labels_match_production_start_requests();
}
