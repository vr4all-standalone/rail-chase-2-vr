// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "game_state.h"
namespace rchase2 {
// MAME's passthrough handle is weak: destroying it does not unregister the tap.
template<class Handle> struct OwnedWriteTap {
    Handle handle;
    OwnedWriteTap()=default;
    OwnedWriteTap(const OwnedWriteTap&)=delete;
    OwnedWriteTap& operator=(const OwnedWriteTap&)=delete;
    ~OwnedWriteTap(){handle.remove();}
};
template<class Read32> bool native_shot_fingerprint(Read32 read) {
    constexpr std::array<std::array<uint32_t,2>,10> words{{
        {0x41c74,0x9a703000},{0x41c78,0x524990},{0x422c8,0x9a703000},{0x422cc,0x524990},
        {0x423c0,0x59084810},{0x423c4,0x90a46048},
        {0x3f570,0x90a03000},{0x3f574,0x501e78},{0x3fac0,0x90a03000},{0x3fac4,0x501e7c}}};
    for(const auto& word:words)if(read(word[0])!=word[1])return false;
    return read_game_state(read).valid;
}
// Engine-only counters. This class is deliberately outside SharedRuntime, whose
// existing shot fields and ABI can carry the measured events without layout changes.
class NativeShotJournal {
    std::array<uint32_t,2> shots_{};
    uint64_t session_{};bool ready_{};
public:
    void suspend(){*this=NativeShotJournal{};}
    template<class Read32> bool record(uint32_t pc,uint32_t address,uint32_t child,uint32_t mask,Read32 read) {
        if(!ready_||address!=0x524994||mask!=0xffffffff||!native_shot_fingerprint(read))return false;
        const int player=pc==0x41c7c?0:pc==0x422d0?1:-1;
        if(player<0)return false;
        const auto state=read_game_state(read);
        if(state.mode!=9||!state.players[player].joined())return false;
        const auto pointer=[](uint32_t value){return value>=0x500000&&value<=0x5fff00&&!(value&3);};
        const auto owner=read(0x524990);
        if(!pointer(owner)||!pointer(child)||owner==child)return false;
        // The native allocator has already linked the new projectile to its
        // player root before this original owner/child notification is written.
        if(read(owner+4)!=(player?0x3fac0u:0x3f570u)||read(child+4)!=0x423c0||
           read(owner+0x10)!=child||read(child+0x14)!=owner)return false;
        if(shots_[player]==UINT32_MAX){suspend();return false;}
        ++shots_[player];return true;
    }
    template<class Read32> void publish(GameState& state,Read32 read) {
        const bool known=state.valid&&state.mode==9&&native_shot_fingerprint(read);
        if(!known||!ready_||session_!=state.session)shots_={};
        session_=state.session;ready_=known;
        state.shot_counters_valid=state.counting_shots=known;
        for(unsigned p=0;p<2;p++)state.players[p].shots=shots_[p];
    }
};
}
