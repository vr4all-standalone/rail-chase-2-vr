// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "cabinet_io.h"
#include "reticle_mode.h"
namespace rchase2 {
struct Aim {double u{.5},v{.5};bool valid{},inside{};};
struct HandInput {bool tracked{},trigger{},start{},pump{};Aim aim;};
using Hands=std::array<HandInput,2>; // Left, right.
enum class BackgroundMode {Original,Depth3D};
inline constexpr std::array<float,4> HapticLevels{0,.35f,.65f,1};
inline int haptic_level(float value){int best=0;for(int i=1;i<4;i++)if(std::abs(value-HapticLevels[i])<std::abs(value-HapticLevels[best]))best=i;return best;}
struct Settings {
    int p1_hand{0};std::array<Calibration,2> calibration=NativeRanges;
    float world_scale{.055f},panel_distance{1.8f},hud_size{1.f},gun_angle{},gun_scale{1},haptics{.65f},view_fov{55};
    bool laser{true},wider_view{true},cart_bottom{true};int gun{};
    float reticle_min_size{.35f},reticle_max_size{1.f},reticle_custom_min_size{.35f},reticle_custom_max_size{1.f},reticle_shrink_distance{6.2f};
    ReticleMode reticle_mode{ReticleMode::Ingame};
    BackgroundMode background{BackgroundMode::Original};
};
// Populate only from separately verified native observations. In particular,
// physical trigger state must never stand in for an accepted native shot.
struct NativeState {
    bool valid{},start_ready{},service{},shot_counters_valid{};
    bool shot_ownership_valid{};
    uint8_t joined_mask{},played_mask{};
    uint64_t session{};
    std::array<uint32_t,2> shots{};
    std::array<std::array<uint32_t,2>,2> shots_by_hand{};
};
struct CabinetInput {
    Cabinet cabinet;
    std::array<int,2> hand{-1,-1};
    std::array<uint32_t,2> accepted_shots{};
};
struct ShotOwnership {
    bool valid{};uint64_t session{};
    std::array<std::array<uint32_t,2>,2> shots_by_hand{};
    template<class State> void update(const State& current,const State& previous,const CabinetInput& delivered) {
        if(session!=current.session){*this={};session=current.session;}
        valid=current.valid&&current.counting_shots&&current.shot_counters_valid;
        if(!valid || !previous.valid || !previous.counting_shots || !previous.shot_counters_valid ||
           previous.session!=current.session)return;
        for(unsigned player=0;player<2;player++) {
            const auto before=previous.players[player].shots,after=current.players[player].shots;
            const int hand=delivered.hand[player];
            // Only observed accepted projectiles with a known delivered owner
            // can later acknowledge a hand. Unknown ownership is discarded.
            if(hand>=0 && hand<2 && delivered.cabinet.trigger[player] && after>=before && after-before<=2)
                shots_by_hand[player][hand]+=after-before;
        }
    }
};
class CabinetLatch {
    CabinetInput previous_;
public:
    CabinetInput update(const CabinetInput& desired) {
        auto out=desired;
        for(int p=0;p<2;p++)if(previous_.hand[p]!=desired.hand[p]&&previous_.cabinet.trigger[p]&&desired.cabinet.trigger[p])out.cabinet.trigger[p]=false;
        previous_=out;return out;
    }
};
class OperatorLatch {
    std::array<unsigned,4> ticks_{};
public:
    uint16_t update(uint16_t pending) {
        uint16_t out{};constexpr unsigned buttons[]={4,8,0x10,0x20};
        for(int i=0;i<4;i++){if(pending&buttons[i])ticks_[i]=12;if(ticks_[i]){out|=buttons[i];--ticks_[i];}}
        return out;
    }
};
class InputPolicy {
    Hands previous_{};
    NativeState previous_state_{};
    std::array<bool,2> blocked_{true,true},outside_pull_{},start_waiting_{true,true};
    struct StartPulse {double time{};int player{-1};};
    std::array<StartPulse,2> pulses_{};
    std::array<int,2> start_owner_{-1,-1};
    bool initialized_{},was_enabled_{},was_ready_{};
    uint8_t previous_joined_{};
    int previous_layout_{0},solo_hand_{0};
    double previous_time_{};
public:
    void suspend(){*this=InputPolicy{};}
    template<class State> CabinetInput update(double now,const Hands& hands,const State& state,
                        const Settings& settings,bool focused,bool menu,const ShotOwnership* ownership=nullptr) {
        NativeState native;native.valid=state.valid;native.start_ready=state.ready;
        native.service=state.service;native.session=state.session;native.played_mask=state.played_mask;
        native.shot_counters_valid=state.counting_shots&&state.shot_counters_valid;
        if(ownership && ownership->valid && ownership->session==state.session) {
            native.shot_ownership_valid=true;native.shots_by_hand=ownership->shots_by_hand;
        }
        for(int p=0;p<2;p++){if(state.players[p].joined())native.joined_mask|=1<<p;native.shots[p]=state.players[p].shots;}
        return update(now,hands,native,settings,focused,menu);
    }
    CabinetInput update(double now,const Hands& hands,const NativeState& state,
                        const Settings& settings,bool focused,bool menu) {
        CabinetInput out;
        if(!std::isfinite(now)){suspend();return out;}
        if(initialized_&&now<previous_time_)suspend();
        const int layout=settings.p1_hand==0?0:1;
        const uint8_t joined=state.valid?state.joined_mask&3:0;
        const bool enabled=focused&&!menu&&state.valid&&!state.service;
        const bool ready=enabled&&state.start_ready;
        const bool session_changed=initialized_&&state.session!=previous_state_.session;
        const bool transition=!initialized_||enabled!=was_enabled_||joined!=previous_joined_||layout!=previous_layout_||session_changed;
        if(transition){blocked_={true,true};outside_pull_={};}
        if(!initialized_||layout!=previous_layout_)solo_hand_=layout;
        if(joined==3)solo_hand_=layout;
        else if(previous_joined_==3 && joined==2)solo_hand_=1-layout;
        const int previous_solo=solo_hand_;
        if(joined!=3&&enabled) {
            const int other=1-solo_hand_;
            if(!hands[solo_hand_].trigger&&hands[other].tracked&&
               (!hands[solo_hand_].tracked || (hands[other].trigger&&!previous_[other].trigger&&!transition)))solo_hand_=other;
        }
        if(solo_hand_!=previous_solo&&previous_[solo_hand_].trigger)blocked_[solo_hand_]=true;
        if(joined==3)out.hand={layout,1-layout};
        else if(joined==1)out.hand={solo_hand_,-1};
        else if(joined==2)out.hand={-1,solo_hand_};
        for(int h=0;h<2;h++) {
            const auto& in=hands[h];
            const bool aim_ok=in.aim.valid&&std::isfinite(in.aim.u)&&std::isfinite(in.aim.v);
            if(!enabled||!in.tracked||!aim_ok)blocked_[h]=true;
            else if(!in.trigger){blocked_[h]=false;outside_pull_[h]=false;}
            if(in.trigger&&!previous_[h].trigger)outside_pull_[h]=!in.aim.inside;
            if(!ready||!in.tracked||ready!=was_ready_||layout!=previous_layout_||session_changed) {
                start_waiting_[h]=true;pulses_[h]={};start_owner_[h]=-1;
            }
            if(joined!=previous_joined_) {
                pulses_[h]={};start_owner_[h]=-1;
                // A held solo Start must not become P2 Start when the cabinet
                // acknowledges P1. A genuinely fresh press may still join P2.
                if(in.start&&previous_[h].start)start_waiting_[h]=true;
            }
            if(ready&&in.tracked&&!in.start)start_waiting_[h]=false;
            if(!in.start)start_owner_[h]=-1;
            if(ready&&in.tracked&&!start_waiting_[h]&&in.start&&!previous_[h].start) {
                const int player=joined==0?(state.played_mask==2?1:state.played_mask==3?(h==layout?0:1):0):(h==layout?0:1);
                if(player==0 || (state.played_mask&(1<<player)) || joined==3 || ((joined&1)&&hands[0].tracked&&hands[1].tracked)) {
                    start_owner_[h]=player;
                    auto& pulse=pulses_[h];
                    if(pulse.player!=player || now-pulse.time>=.26)pulse={now,player};
                }
            }
        }
        for(int p=0;p<2;p++) {
            const int h=out.hand[p];if(h<0)continue;
            const auto& hand=hands[h];
            const auto aim=map_aim(p,hand.aim.u,hand.aim.v,settings.calibration[p]);
            if(hand.tracked&&hand.aim.valid&&aim.valid) {
                out.cabinet.x[p]=aim.x;out.cabinet.y[p]=aim.y;
                out.cabinet.trigger[p]=enabled&&!blocked_[h]&&hand.trigger&&hand.aim.inside&&!outside_pull_[h];
            }
        }
        if(ready)for(int h=0;h<2;h++) {
            const auto& pulse=pulses_[h];if(pulse.player<0)continue;
            const double elapsed=now-pulse.time;
            if(elapsed>=0&&elapsed<.1)out.cabinet.coins|=1<<pulse.player;
            if(elapsed>=.14&&(elapsed<.26||(hands[h].start&&start_owner_[h]==pulse.player)))out.cabinet.starts|=1<<pulse.player;
        }
        if(enabled&&!transition&&state.shot_counters_valid&&state.shot_ownership_valid&&previous_state_.valid&&
           previous_state_.shot_counters_valid&&previous_state_.shot_ownership_valid&&state.session==previous_state_.session) {
            for(int p=0;p<2;p++)if(joined&(1<<p)) {
                const int hand=out.hand[p];if(hand<0 || hand>1)continue;
                const auto before=previous_state_.shots_by_hand[p][hand],current=state.shots_by_hand[p][hand];
                if(current>=before&&current-before<=64)out.accepted_shots[p]=current-before;
            }
        }
        previous_=hands;previous_state_=state;previous_layout_=layout;previous_joined_=joined;
        was_enabled_=enabled;was_ready_=ready;previous_time_=now;initialized_=true;
        return out;
    }
};
}
