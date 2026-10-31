// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "cabinet_io.h"
namespace rchase2 {
struct PlayerState {
    uint32_t state{},ammo{},capacity{},shots{},weapon{};
    float health{};
    bool joined() const {return state!=0;}
    bool alive() const {return joined()&&health>0;}
};
struct GameState {
    bool valid{},ready{},playing{},counting_shots{},service{},gun_calibration{};
    bool shot_counters_valid{};
    uint32_t mode{},submode{},service_menu{},service_submode{},service_busy{};
    uint64_t session{};
    std::array<PlayerState,2> players{};
    bool pricing_valid{},free_play{};
    uint16_t credits{};uint8_t played_mask{};
    std::array<uint16_t,2> start_cost{};
};
// Revision A dispatcher, Start handler and native price routine are independently
// identified in the loaded program. See docs/native-state-evidence.md.
template<class Read32> GameState read_game_state(Read32 read) {
    GameState out;
    constexpr std::array<std::array<uint32_t,2>,8> fingerprint{{
        {0x27b4,0x90a03000},{0x27b8,0x501d84},
        {0x11fa0,0x90a03000},{0x11fa4,0x501d84},
        {0x26d90,0x80a03000},{0x26d94,0x501d18},
        {0x120c8,0x92b03000},{0x120cc,0x501d84}}};
    for(const auto& word:fingerprint)if(read(word[0])!=word[1])return out;
    out.mode=read(0x501d84);if(out.mode>15)return {};
    out.valid=true;out.submode=read(0x501d90);out.service=out.mode==14;
    out.ready=out.mode>2&&out.mode<14&&read(0x501f50)!=0&&!(read(0x501d94)&0x10);
    out.playing=out.mode==9||out.mode==11;
    const auto active=read(0x501d88)&3;
    for(unsigned p=0;p<2;p++) {
        out.players[p].state=out.playing?((active>>p)&1):0;
        // Native active-seat eligibility, not a guessed numerical life value.
        out.players[p].health=out.players[p].joined()?1.f:0.f;
    }
    const auto half=[&](uint32_t address){return uint16_t(read(address&~3u)>>((address&2)*8));};
    out.free_play=(read(0x501d18)&8)!=0;
    out.credits=half(0x1d00022);
    const std::array<uint32_t,2> history{read(0x501f30),read(0x501f34)};
    out.played_mask=(history[0]?1:0)|(history[1]?2:0);
    out.pricing_valid=out.free_play||out.credits<=99;
    for(unsigned p=0;p<2;p++) {
        out.start_cost[p]=out.free_play?0:half(history[p]?0x501d2a:history[1-p]?0x501d28:0x501d1c);
        if(!out.free_play&&(!out.start_cost[p]||out.start_cost[p]>99))out.pricing_valid=false;
    }
    return out;
}
class ShotTracker {
public:
    void suspend() {}
    template<class Read32> void update(GameState& state,Read32) {
        state.counting_shots=state.shot_counters_valid=false;
    }
};
struct NativeAimCalibration {
    bool valid{},factory{},screen_inverse_valid{};
    uint8_t left{},right{},top{},bottom{},center_x{},center_y{};
    unsigned player{};
};
inline bool valid_calibration(const NativeAimCalibration& c) {
    return c.valid&&c.player<2&&c.left>128&&c.right<128&&c.top>128&&c.bottom<128;
}
template<class Read32> NativeAimCalibration read_native_calibration(Read32 read,unsigned player) {
    if(player>1||!read_game_state(read).valid)return {};
    const auto byte=[&](uint32_t a){return uint8_t(read(a&~3u)>>((a&3)*8));};
    NativeAimCalibration c;c.player=player;
    const auto base=0x501d0fu+4*player;
    c.left=byte(base+3);c.right=byte(base+2);c.top=byte(base);c.bottom=byte(base+1);
    c.center_x=c.center_y=128;c.valid=true;
    if(!valid_calibration(c))return {};
    c.factory=c.left==192&&c.right==64&&c.top==192&&c.bottom==64;
    c.screen_inverse_valid=true;return c;
}
struct NativeAimPoint {double x{},y{},z{};bool valid{};};
inline NativeAimPoint native_aim_point(const NativeAimCalibration& c,uint8_t x,uint8_t y) {
    if(!valid_calibration(c))return {};
    // Original 0x41500/0x41610 gains and 0x41720/0x41d20 reticle transform.
    // Piecewise calibration is asymmetric between player views. The TGP then
    // multiplies pixel-relative coordinates by 6 and divides by 768.
    const float dx=128.f-x,dy=128.f-y;
    const float gx=dx<=0?(c.player?310.f:186.f)/(int(c.left)-128):
                                 (c.player?186.f:310.f)/(128-int(c.right));
    const float gy=dy<=0?192.f/(128-int(c.top)):144.f/(int(c.bottom)-128);
    const float px=dx*gx+(c.player?62.f:-62.f),py=dy*gy;
    return {double(px)/128.,double(py)/128.,6.,true};
}
inline AimValue native_screen_to_adc(unsigned player,double u,double v,const NativeAimCalibration& c) {
    if(player>1||c.player!=player||!c.screen_inverse_valid||!valid_calibration(c)||!std::isfinite(u)||!std::isfinite(v))return {};
    const auto closest=[](double target,int lo,int hi,auto project) {
        int best=lo;double distance=INFINITY;
        for(int raw=lo;raw<=hi;raw++){const auto error=std::abs(project(uint8_t(raw))-target);if(error<distance){distance=error;best=raw;}}
        return uint8_t(best);
    };
    const auto& limits=NativeRanges[player];
    auto x=closest(std::clamp(u,0.,1.)*495.,limits.right,limits.left,[&](uint8_t raw){return 248.+128.*native_aim_point(c,raw,128).x;});
    auto y=closest(std::clamp(v,0.,1.)*383.,limits.bottom,limits.top,[&](uint8_t raw){return 192.-128.*native_aim_point(c,128,raw).y;});
    return {x,y,true};
}
}
