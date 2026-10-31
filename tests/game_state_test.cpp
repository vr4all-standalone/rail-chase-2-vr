// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/game_state.h"
#include <cassert>
#include <map>
int main() {
    using namespace rchase2;
    unsigned reads{};
    auto read=[&](uint32_t){++reads;return 0u;};
    auto state=read_game_state(read);
    assert(!state.valid && !state.ready && !state.playing && !state.service);
    assert(!state.players[0].joined() && !state.players[1].joined());
    assert(!state.shot_counters_valid);
    for(unsigned p=0;p<2;p++) {
        const auto calibration=read_native_calibration(read,p);
        assert(!calibration.valid);
        assert(!native_screen_to_adc(p,.5,.5,calibration).valid);
    }
    ShotTracker tracker;tracker.update(state,read);
    assert(!state.counting_shots && !state.shot_counters_valid);
    // Unknown ROM fingerprint must fail before any state address is read.
    assert(reads>0 && reads<10);
    std::map<uint32_t,uint32_t> memory{{0x27b4,0x90a03000},{0x27b8,0x501d84},
        {0x11fa0,0x90a03000},{0x11fa4,0x501d84},{0x26d90,0x80a03000},{0x26d94,0x501d18},
        {0x120c8,0x92b03000},{0x120cc,0x501d84},
        {0x501d84,9},{0x501d90,1},{0x501d88,3},{0x501f50,1},
        {0x501d18,0xfffffff2},{0x501d1c,0x10002},{0x501d28,0x10002},
        {0x1d00020,4u<<16},{0x501f30,1},{0x501f34,0}};
    const auto fixture=[&](uint32_t a){return memory[a];};
    state=read_game_state(fixture);
    assert(state.valid && state.ready && state.playing);
    assert(state.players[0].joined() && state.players[1].joined());
    assert(state.pricing_valid && state.credits==4);
    assert(state.start_cost[0]==1 && state.start_cost[1]==2);
    memory[0x501d84]=14;state=read_game_state(fixture);
    assert(state.valid && state.service && !state.ready && !state.playing);
    memory[0x501d84]=99;assert(!read_game_state(fixture).valid);
    memory[0x501d84]=9;memory[0x1d00020]=65535u<<16;assert(!read_game_state(fixture).pricing_valid);
    memory[0x501d84]=9;memory[0x501d18]|=8;state=read_game_state(fixture);
    assert(state.free_play && state.start_cost[0]==0 && state.start_cost[1]==0);
    memory[0x27b8]=0;assert(!read_game_state(fixture).valid);
}
