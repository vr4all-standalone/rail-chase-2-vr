// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/uart_clock_policy.h"
#include <cassert>
#include <initializer_list>
using rchase2::UartClockPolicy;
int main() {
    // Cold phase is low: rising edge 1us, falling edge 2us. After mode
    // programming x16, the first TX terminal edge is the sixteenth fall.
    auto event=UartClockPolicy::next(0,0,16,true,0);
    assert(event.edge==31 && event.prepare);
    event=UartClockPolicy::next(30,15,16,true,32);
    assert(event.edge==32 && !event.prepare);
    event=UartClockPolicy::next(31,15,16,true,32);
    assert(event.edge==32 && !event.prepare);
    event=UartClockPolicy::next(32,0,16,true,32);
    assert(event.edge==63 && event.prepare);
    // data_w does not restart a partially elapsed divider.
    event=UartClockPolicy::next(10,5,16,true,0);
    assert(event.edge==31 && event.prepare);
    // A mode write resets only the TX divider, retaining the physical phase.
    event=UartClockPolicy::next(10,0,16,true,0);
    assert(event.edge==41 && event.prepare);
    event=UartClockPolicy::next(11,0,16,true,0);
    assert(event.edge==41 && event.prepare);
    event=UartClockPolicy::next(0,0,64,true,0);
    assert(event.edge==127 && event.prepare);
    event=UartClockPolicy::next(126,63,64,true,0);
    assert(event.edge==127 && event.prepare);
    event=UartClockPolicy::next(127,63,64,true,0);
    assert(event.edge==128 && !event.prepare);
    event=UartClockPolicy::next(30,15,16,true,0);
    assert(event.edge==31 && event.prepare);
    for(int divisor:{0,1,3,16,64}) {
        event=UartClockPolicy::next(10,0,divisor,false,0);
        assert(event.edge==11 && !event.prepare);
    }
    for(int count:{-1,16,17}) {
        event=UartClockPolicy::next(10,count,16,true,0);
        assert(event.edge==11 && !event.prepare);
    }
    for(int divisor:{0,1,3,15,17,63,65}) {
        event=UartClockPolicy::next(10,0,divisor,true,0);
        assert(event.edge==11 && !event.prepare);
    }
    // Compare every valid divider count and both phases with a literal edge
    // oracle. Preparing may defer quiet edges, but never a terminal TX event.
    for(int divisor:{16,64})for(uint64_t phase=0;phase<128;phase++)for(int count=0;count<divisor;count++) {
        uint64_t terminal=phase;int counter=count;
        do {++terminal;if((terminal&1)==0)++counter;}while(counter<divisor);
        event=UartClockPolicy::next(phase,count,divisor,true,0);
        assert(event.edge+(event.prepare?1:0)==terminal);
        const auto armed=UartClockPolicy::next(phase,count,divisor,true,terminal);
        assert(armed.edge==terminal && !armed.prepare);
    }
    // An exact-timestamp input mutation precedes the still-pending virtual
    // edge. This is essential for RXD falling at a preparatory rising edge.
    assert(UartClockPolicy::flush_limit(31,true,32)==30);
    assert(UartClockPolicy::flush_limit(31,false,32)==31);
    assert(UartClockPolicy::flush_limit(35,false,32)==31);
    assert(UartClockPolicy::flush_limit(0,true,1)==0);
    // If a timer is already armed at the required timestamp, preserve its
    // insertion order rather than cancelling and reinserting on a bus read.
    assert(UartClockPolicy::keep_armed(32,32,31));
    assert(UartClockPolicy::keep_armed(32,64,32));
    assert(!UartClockPolicy::keep_armed(32,31,31));
    assert(!UartClockPolicy::keep_armed(32,64,31));
    // A real +80ns IRQ timer splits a slice, but must not shift the next UART
    // grid boundary by80ns. Exact boundaries always advance to the next tick.
    auto grid=UartClockPolicy::next_global_grid(0,0);
    assert(grid.seconds==0 && grid.attoseconds==1000000000000LL);
    grid=UartClockPolicy::next_global_grid(0,80000000000LL);
    assert(grid.seconds==0 && grid.attoseconds==1000000000000LL);
    grid=UartClockPolicy::next_global_grid(0,1080000000000LL);
    assert(grid.seconds==0 && grid.attoseconds==2000000000000LL);
    grid=UartClockPolicy::next_global_grid(17,999999080000000000LL);
    assert(grid.seconds==18 && grid.attoseconds==0);
    grid=UartClockPolicy::next_global_grid(18,0);
    assert(grid.seconds==18 && grid.attoseconds==1000000000000LL);
    // A virtual UART tick already heads the legacy queue. A new timer tied
    // with it goes behind it; equality must not abort the currently running CPU.
    assert(UartClockPolicy::precedes_global_grid(0,80000000000LL,0,0));
    assert(!UartClockPolicy::precedes_global_grid(0,1000000000000LL,0,0));
    assert(!UartClockPolicy::precedes_global_grid(0,16000000000000LL,0,0));
    assert(UartClockPolicy::precedes_global_grid(0,1160000000000LL,0,1080000000000LL));
    assert(!UartClockPolicy::precedes_global_grid(0,2000000000000LL,0,1080000000000LL));
    assert(UartClockPolicy::precedes_global_grid(17,999999900000000000LL,17,999999080000000000LL));
    assert(!UartClockPolicy::precedes_global_grid(18,0,17,999999080000000000LL));
    // The same local timestamp has different ordering before versus after
    // global timer31. A lagging CPU must also see completed global edges.
    assert(UartClockPolicy::cpu_flush_limit(31,true,32,30,true)==30);
    assert(UartClockPolicy::cpu_flush_limit(31,true,32,31,true)==31);
    assert(UartClockPolicy::cpu_flush_limit(30,false,32,31,true)==31);
    assert(UartClockPolicy::cpu_flush_limit(32,true,32,31,true)==31);
    assert(UartClockPolicy::cpu_flush_limit(31,true,32,31,false)==30);
    assert(!UartClockPolicy::enabled(nullptr,"1",nullptr));
    assert(!UartClockPolicy::enabled("1",nullptr,nullptr));
    assert(!UartClockPolicy::enabled("1","10",nullptr));
    assert(!UartClockPolicy::enabled("1","1",""));
    assert(UartClockPolicy::enabled("","1",nullptr));
}
