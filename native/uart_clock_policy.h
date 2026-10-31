// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <algorithm>
namespace rchase2 {
struct UartClockPolicy {
    struct Event {uint64_t edge;bool prepare;};
    struct Grid {int32_t seconds;int64_t attoseconds;};
    static Grid next_global_grid(int32_t seconds,int64_t attoseconds) noexcept {
        constexpr int64_t tick=1000000000000LL,second=1000000000000000000LL;
        const int64_t next=(attoseconds/tick+1)*tick;
        return next==second?Grid{seconds+1,0}:Grid{seconds,next};
    }
    static bool precedes_global_grid(int32_t event_seconds,int64_t event_attoseconds,
                                     int32_t base_seconds,int64_t base_attoseconds) noexcept {
        const auto grid=next_global_grid(base_seconds,base_attoseconds);
        return event_seconds<grid.seconds ||
            (event_seconds==grid.seconds && event_attoseconds<grid.attoseconds);
    }
    static uint64_t cpu_flush_limit(uint64_t whole,bool exact,uint64_t pending,
                                   uint64_t completed_whole,bool executing_cpu) noexcept {
        auto limit=flush_limit(whole,exact,pending);
        if(executing_cpu)limit=std::max(limit,flush_limit(completed_whole,false,pending));
        return limit;
    }
    static bool enabled(const char* vr,const char* coalesce,const char* reference) noexcept {
        return vr && coalesce && coalesce[0]=='1' && coalesce[1]=='\0' && !reference;
    }
    // Edge indices are on the unchanged 1us half-period grid: odd=RX rising,
    // even=TX falling. Preparing one edge before a terminal TX tick preserves
    // its legacy insertion time relative to longer-period serial timers.
    static Event next(uint64_t edge,int count,int divisor,bool eligible,uint64_t armed) noexcept {
        if(!eligible || (divisor!=16 && divisor!=64) || count<0 || count>=divisor)
            return {edge+1,false};
        const uint64_t first_fall=(edge&1)?edge+1:edge+2;
        const uint64_t terminal=first_fall+2*uint64_t(divisor-count-1);
        if(terminal==armed || terminal==edge+1)return {terminal,false};
        return {terminal-1,true};
    }
    static uint64_t flush_limit(uint64_t whole_edges,bool exact,uint64_t pending) noexcept {
        if(exact && whole_edges)--whole_edges;
        return std::min(whole_edges,pending?pending-1:whole_edges);
    }
    static bool keep_armed(uint64_t pending,uint64_t desired,uint64_t now) noexcept {
        return pending && (pending==desired || pending<=now);
    }
};
}
