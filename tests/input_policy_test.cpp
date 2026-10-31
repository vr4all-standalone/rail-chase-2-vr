// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/input_policy.h"
#include "../native/game_state.h"
#include <cassert>
#include <limits>
using namespace rchase2;
Hands neutral() {Hands h;for(auto& x:h){x.tracked=true;x.aim={.5,.5,true,true};}return h;}
static void either_hand_starts_p1(int layout,int first) {
    Settings settings;settings.p1_hand=layout;
    NativeState native;native.valid=native.start_ready=true;
    auto hands=neutral();InputPolicy policy;policy.update(0,hands,native,settings,true,false);
    hands[first].start=hands[first].trigger=true;
    auto input=policy.update(.01,hands,native,settings,true,false);
    assert(input.cabinet.coins==1&&input.cabinet.starts==0); // Either hand inserts P1 credit.
    input=policy.update(.12,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==0);
    input=policy.update(.16,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==1); // Native P1 Start follows credit.
    native.joined_mask=1;
    input=policy.update(.18,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==0&&!input.cabinet.trigger[0]);
    input=policy.update(.30,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==0&&!input.cabinet.trigger[0]);
    hands=neutral();policy.update(.31,hands,native,settings,true,false);
    hands[first].trigger=true;input=policy.update(.32,hands,native,settings,true,false);
    assert(input.hand[0]==first&&input.cabinet.trigger[0]); // Solo P1 still uses either hand.
    const int second=1-layout;
    hands[0].trigger=hands[1].trigger=true;hands[second].start=true;
    input=policy.update(.33,hands,native,settings,true,false);
    assert(input.cabinet.coins==2&&input.cabinet.starts==0);
    input=policy.update(.49,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==2);
    native.joined_mask=3;input=policy.update(.51,hands,native,settings,true,false);
    assert(input.hand[0]==layout&&input.hand[1]==1-layout);
    assert(input.cabinet.coins==0&&input.cabinet.starts==0&&!input.cabinet.trigger[0]&&!input.cabinet.trigger[1]);
    input=policy.update(.65,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==0&&!input.cabinet.trigger[0]&&!input.cabinet.trigger[1]);
    hands=neutral();policy.update(.66,hands,native,settings,true,false);
    hands[0].trigger=hands[1].trigger=true;input=policy.update(.67,hands,native,settings,true,false);
    assert(input.cabinet.trigger[0]&&input.cabinet.trigger[1]);
    hands[second].start=true;settings.p1_hand=1-layout;
    input=policy.update(.68,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==0&&!input.cabinet.trigger[0]&&!input.cabinet.trigger[1]);
    input=policy.update(.85,hands,native,settings,true,false);
    assert(input.cabinet.coins==0&&input.cabinet.starts==0);
    hands=neutral();policy.update(.86,hands,native,settings,true,false);
    hands[settings.p1_hand].start=true;input=policy.update(.87,hands,native,settings,true,false);
    assert(input.cabinet.coins==1&&input.cabinet.starts==0); // Explicit reversal remains respected.
}
int main() {
    {
        NativeState dead;dead.valid=dead.start_ready=true;dead.played_mask=2;
        Settings settings;InputPolicy resume;auto hands=neutral();
        resume.update(0,hands,dead,settings,true,false);
        hands[0].start=true;
        assert(resume.update(.01,hands,dead,settings,true,false).cabinet.coins==2);
        assert(resume.update(.16,hands,dead,settings,true,false).cabinet.starts==2);
    }

    for(int layout:{0,1})for(int first:{1,0})either_hand_starts_p1(layout,first);
    {
        Settings defaults;NativeState game;game.valid=game.start_ready=true;game.joined_mask=3;
        auto controllers=neutral();InputPolicy normal;
        auto neutral_input=normal.update(0,controllers,game,defaults,true,false);
        assert(neutral_input.hand[0]==0&&neutral_input.hand[1]==1);
        controllers[0].trigger=true;controllers[0].start=true;
        auto left=normal.update(.02,controllers,game,defaults,true,false);
        assert(left.cabinet.trigger[0]&&!left.cabinet.trigger[1]&&left.cabinet.coins==1);
        controllers=neutral();normal.update(.03,controllers,game,defaults,true,false);
        assert(normal.update(.18,controllers,game,defaults,true,false).cabinet.starts==1);
        controllers[1].trigger=true;controllers[1].start=true;
        auto right=normal.update(.4,controllers,game,defaults,true,false);
        assert(!right.cabinet.trigger[0]&&right.cabinet.trigger[1]&&right.cabinet.coins==2);
    }
    // Native observations belong to the cabinet owner that was actually
    // latched while the game accepted the projectile, not the next XR sample.
    ShotOwnership observed;GameState before,current;CabinetInput delivered;
    before.valid=before.counting_shots=before.shot_counters_valid=true;before.session=55;before.players[0].shots=10;
    current=before;current.players[0].shots=11;delivered.hand={1,-1};delivered.cabinet.trigger[0]=true;
    observed.update(current,before,delivered);
    assert(observed.valid && observed.shots_by_hand[0][1]==1 && observed.shots_by_hand[0][0]==0);
    before=current;current.players[0].shots=12;delivered.hand[0]=0;observed.update(current,before,delivered);
    assert(observed.shots_by_hand[0][0]==1 && observed.shots_by_hand[0][1]==1);
    before=current;current.players[0].shots=13;delivered.cabinet.trigger[0]=false;observed.update(current,before,delivered);
    assert(observed.shots_by_hand[0][0]==1 && observed.shots_by_hand[0][1]==1);
    before=current;current.players[0].shots=14;delivered.hand[0]=-1;delivered.cabinet.trigger[0]=true;observed.update(current,before,delivered);
    assert(observed.shots_by_hand[0][0]==1 && observed.shots_by_hand[0][1]==1);
    before=current;current.session=56;current.players[0].shots=1;observed.update(current,before,delivered);
    assert(observed.session==56 && observed.shots_by_hand[0][0]==0 && observed.shots_by_hand[0][1]==0);
    Settings settings;settings.p1_hand=1; // Existing reversed-layout coverage remains explicit.
    NativeState native;native.valid=native.start_ready=true;native.joined_mask=1;
    Hands hands=neutral();InputPolicy policy;
    hands[1].trigger=true;
    assert(!policy.update(0,hands,native,settings,true,false).cabinet.trigger[0]);
    hands[1].trigger=false;policy.update(.01,hands,native,settings,true,false);
    hands[1].trigger=true;auto shot=policy.update(.02,hands,native,settings,true,false);
    assert(shot.cabinet.trigger[0]);assert(shot.hand[0]==1 && shot.hand[1]==-1);
    assert(policy.update(.03,hands,native,settings,true,false).cabinet.trigger[0]); // Automatic fire stays held.
    assert(shot.accepted_shots[0]==0); // Pulls are not proof the native game fired.
    assert(!policy.update(.04,hands,native,settings,true,true).cabinet.trigger[0]);
    assert(!policy.update(.05,hands,native,settings,true,false).cabinet.trigger[0]);
    hands[1].trigger=false;policy.update(.06,hands,native,settings,true,false);
    hands[1].trigger=true;assert(policy.update(.07,hands,native,settings,true,false).cabinet.trigger[0]);
    hands[1].tracked=false;assert(!policy.update(.08,hands,native,settings,true,false).cabinet.trigger[0]);
    hands[1].tracked=true;assert(!policy.update(.09,hands,native,settings,true,false).cabinet.trigger[0]);
    hands=neutral();policy.update(.10,hands,native,settings,true,false);
    hands[0].trigger=true;auto left=policy.update(.11,hands,native,settings,true,false);
    assert(left.hand[0]==0 && left.cabinet.trigger[0]); // Fresh solo hand transfer.
    native.joined_mask=3;hands[1].trigger=true;
    auto joined=policy.update(.12,hands,native,settings,true,false);
    assert(joined.hand[0]==1 && joined.hand[1]==0);
    assert(!joined.cabinet.trigger[0] && !joined.cabinet.trigger[1]);
    hands=neutral();policy.update(.13,hands,native,settings,true,false);
    hands[0].trigger=hands[1].trigger=true;
    auto both=policy.update(.14,hands,native,settings,true,false);
    assert(both.cabinet.trigger[0] && both.cabinet.trigger[1]);
    settings.p1_hand=0;auto swap=policy.update(.15,hands,native,settings,true,false);
    assert(swap.hand[0]==0 && swap.hand[1]==1);
    assert(!swap.cabinet.trigger[0] && !swap.cabinet.trigger[1]);
    hands=neutral();policy.update(.16,hands,native,settings,true,false);
    hands[0].trigger=true;hands[0].aim.inside=false;
    auto outside=policy.update(.17,hands,native,settings,true,false);
    assert(!outside.cabinet.trigger[0]); // RailChase2 has no invented offscreen reload pulse.
    hands[0].aim.inside=true;
    assert(!policy.update(.18,hands,native,settings,true,false).cabinet.trigger[0]);
    hands=neutral();policy.update(.19,hands,native,settings,true,false);
    hands[0].trigger=true;hands[0].aim.u=std::numeric_limits<double>::quiet_NaN();
    assert(!policy.update(.20,hands,native,settings,true,false).cabinet.trigger[0]);
    hands[0].aim.u=.5;
    assert(!policy.update(.205,hands,native,settings,true,false).cabinet.trigger[0]);
    native.valid=false;assert(!policy.update(.21,hands,native,settings,true,false).cabinet.trigger[0]);

    // Start must be released after boot/menu/focus; credit precedes native Start.
    InputPolicy starts;hands=neutral();settings.p1_hand=1;native={};
    hands[1].start=true;assert(starts.update(1,hands,native,settings,true,false).cabinet.coins==0);
    native.valid=native.start_ready=true;
    assert(starts.update(2,hands,native,settings,true,false).cabinet.coins==0);
    hands[1].start=false;starts.update(2.01,hands,native,settings,true,false);
    hands[1].start=true;auto coin=starts.update(2.02,hands,native,settings,true,false);
    assert(coin.cabinet.coins==1 && coin.cabinet.starts==0);
    hands[1].start=false;auto start=starts.update(2.18,hands,native,settings,true,false);
    assert(start.cabinet.coins==0 && start.cabinet.starts==1); // Retain short tap.
    assert(starts.update(2.4,hands,native,settings,true,false).cabinet.starts==0);
    native.joined_mask=1;hands[0].start=true;
    assert(starts.update(2.5,hands,native,settings,true,false).cabinet.coins==2);
    hands[0].start=false;assert(starts.update(2.66,hands,native,settings,true,false).cabinet.starts==2);
    starts.update(2.7,hands,native,settings,true,true);
    assert(starts.update(2.71,hands,native,settings,true,false).cabinet.starts==0);
    assert(starts.update(std::numeric_limits<double>::quiet_NaN(),hands,native,settings,true,false).cabinet.coins==0);

    // Feedback only follows a contiguous measured native counter in one session.
    InputPolicy feedback;native.valid=native.start_ready=true;native.joined_mask=1;
    native.shot_counters_valid=native.shot_ownership_valid=true;native.session=3;native.shots={100,0};
    native.shots_by_hand[0][1]=100;hands=neutral();
    feedback.update(3,hands,native,settings,true,false);
    native.shots[0]=native.shots_by_hand[0][1]=103;assert(feedback.update(3.01,hands,native,settings,true,false).accepted_shots[0]==3);
    native.shots[0]=native.shots_by_hand[0][1]=0;assert(feedback.update(3.02,hands,native,settings,true,false).accepted_shots[0]==0);
    native.session=4;native.shots[0]=4;
    assert(feedback.update(3.03,hands,native,settings,true,false).accepted_shots[0]==0);

    InputPolicy restart;hands=neutral();native.session=10;native.joined_mask=1;
    restart.update(4,hands,native,settings,true,false);
    hands[1].trigger=hands[1].start=true;
    assert(restart.update(4.01,hands,native,settings,true,false).cabinet.trigger[0]);
    native.session=11;
    auto reset=restart.update(4.16,hands,native,settings,true,false);
    assert(!reset.cabinet.trigger[0] && reset.cabinet.coins==0 && reset.cabinet.starts==0);

    InputPolicy handoff;hands=neutral();native.joined_mask=1;
    handoff.update(5,hands,native,settings,true,false);
    hands[1].trigger=true;handoff.update(5.01,hands,native,settings,true,false);
    hands[0].trigger=true;handoff.update(5.02,hands,native,settings,true,false);
    hands[1].tracked=false;hands[1].trigger=false;
    auto lost=handoff.update(5.03,hands,native,settings,true,false);
    assert(lost.hand[0]==0 && !lost.cabinet.trigger[0]);
    hands[0].trigger=false;handoff.update(5.04,hands,native,settings,true,false);
    hands[0].trigger=true;
    assert(handoff.update(5.05,hands,native,settings,true,false).cabinet.trigger[0]);

    // Counter acknowledgements from the former solo hand must not pulse the new
    // hand, including acknowledgements arriving after the handoff host sample.
    InputPolicy ownership;hands=neutral();native={};native.valid=native.start_ready=true;
    native.shot_counters_valid=native.shot_ownership_valid=true;native.joined_mask=1;native.session=20;native.shots[0]=100;
    native.shots_by_hand[0][1]=100;
    ownership.update(6,hands,native,settings,true,false);
    hands[1].trigger=true;ownership.update(6.01,hands,native,settings,true,false);
    hands[1].trigger=false;hands[0].trigger=true;native.shots[0]=101;native.shots_by_hand[0][1]=101;
    auto transferred=ownership.update(6.02,hands,native,settings,true,false);
    assert(transferred.hand[0]==0 && transferred.accepted_shots[0]==0);
    native.shots[0]=102;native.shots_by_hand[0][1]=102;
    assert(ownership.update(6.03,hands,native,settings,true,false).accepted_shots[0]==0);
    native.shots[0]=103;native.shots_by_hand[0][0]=1;
    assert(ownership.update(6.04,hands,native,settings,true,false).accepted_shots[0]==1);
    native.shots[0]=105;native.shots_by_hand[0][0]=2;native.shots_by_hand[0][1]=103;
    assert(ownership.update(6.05,hands,native,settings,true,false).accepted_shots[0]==1);

    // The production GameState overload must use the same ownership snapshot
    // and must reject metadata from another native session.
    InputPolicy bridge;current={};current.valid=current.ready=current.counting_shots=current.shot_counters_valid=true;
    current.players[0].state=1;current.session=77;current.players[0].shots=100;
    observed={};observed.valid=true;observed.session=77;observed.shots_by_hand[0][1]=100;hands=neutral();
    bridge.update(7,hands,current,settings,true,false,&observed);
    current.players[0].shots=101;observed.shots_by_hand[0][1]=101;
    assert(bridge.update(7.01,hands,current,settings,true,false,&observed).accepted_shots[0]==1);
    observed.session=78;observed.shots_by_hand[0][1]=102;
    assert(bridge.update(7.02,hands,current,settings,true,false,&observed).accepted_shots[0]==0);
}
