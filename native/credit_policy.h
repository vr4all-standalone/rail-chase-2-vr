// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "game_state.h"
namespace rchase2 {
// Bounded cabinet coin/start requests. Price comes from the native coin routine,
// so initial play, joining, continue and operator coinage remain game-owned.
class NativeCreditPolicy {
    int pending_{-1};double began_{},start_at_{-1},previous_time_{};
    uint8_t previous_request_{},previous_joined_{};uint64_t session_{};
public:
    void suspend(){*this=NativeCreditPolicy{};}
    Cabinet update(double now,const Cabinet& desired,const GameState& state,bool enabled) {
        Cabinet out=desired;out.coins=out.starts=0;
        const uint8_t joined=(state.players[0].joined()?1:0)|(state.players[1].joined()?2:0);
        if(!std::isfinite(now)||now<previous_time_||state.session!=session_||joined!=previous_joined_) {
            pending_=-1;previous_request_=0;start_at_=-1;
        }
        previous_time_=now;session_=state.session;previous_joined_=joined;
        const uint8_t request=desired.coins&3;
        if(!enabled||!state.valid||!state.ready||!state.pricing_valid) {
            pending_=-1;previous_request_=request;return out;
        }
        const auto fresh=request&~previous_request_;
        previous_request_=request;
        if(pending_<0)for(int p=0;p<2;p++)if((fresh&(1<<p))&&!(joined&(1<<p))) {
            pending_=p;began_=now;start_at_=-1;break;
        }
        if(pending_<0)return out;
        if(now-began_>4 || (joined&(1<<pending_))) {pending_=-1;return out;}
        const bool paid=state.free_play||state.credits>=state.start_cost[pending_];
        if(!paid) {
            // A real release separates coin edges; keep retrying only while the
            // independently measured credit balance has not reached native cost.
            if(std::fmod(now-began_,.3)<.1)out.coins=1<<pending_;
        } else {
            if(start_at_<0)start_at_=std::max(began_+.14,now);
            if(now>=start_at_&&std::fmod(now-start_at_,.35)<.12)out.starts=1<<pending_;
        }
        return out;
    }
};
}
