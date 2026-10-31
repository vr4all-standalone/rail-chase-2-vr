// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace rchase2 {
// MAME 0.289 model2.cpp: rchase2 Revision A -> model2crx.
// All Revision A axes use PORT_REVERSE. rchase2a is a different contract.
struct Cabinet {
    std::array<uint8_t,2> x{0x82,0x7d},y{0x73,0x73};
    std::array<bool,2> trigger{};
    uint8_t coins{},starts{};
    bool test{},service{};
};
inline uint8_t in0(uint8_t native,const Cabinet& input) {
    uint8_t active=(input.coins&3)|((input.starts&3)<<4);
    if(input.test)active|=4;
    if(input.service)active|=8;
    return native&~active;
}
inline uint8_t in1(uint8_t native,const Cabinet& input) {
    return native&~((input.trigger[0]?1:0)|(input.trigger[1]?2:0));
}
inline uint8_t adc(unsigned channel,const Cabinet& input) {
    switch(channel) {
    case 0:return input.x[1];case 1:return input.x[0];
    case 2:return input.y[1];case 3:return input.y[0];
    default:return 0xff;
    }
}
struct Calibration {uint8_t left{},right{},top{},bottom{};};
inline constexpr std::array<Calibration,2> NativeRanges{{{0xca,0x3a,0xcb,0x1c},{0xc7,0x34,0xcb,0x1c}}};
struct AimValue {uint8_t x{},y{};bool valid{};};
inline AimValue map_aim(unsigned player,double u,double v,Calibration calibration) {
    if(player>1 || !std::isfinite(u) || !std::isfinite(v) ||
       calibration.left==calibration.right || calibration.top==calibration.bottom)return {};
    const auto interpolate=[](double amount,uint8_t from,uint8_t to) {
        return uint8_t(std::lround(from+std::clamp(amount,0.,1.)*(int(to)-int(from))));
    };
    return {interpolate(u,calibration.left,calibration.right),interpolate(v,calibration.top,calibration.bottom),true};
}
inline AimValue map_aim(unsigned player,double u,double v) {
    if(player>1)return {};
    return map_aim(player,u,v,NativeRanges[player]);
}
}
