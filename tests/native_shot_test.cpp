// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/shot_events.h"
#include <cassert>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <memory>
using namespace rchase2;
struct Fixture {
    std::map<uint32_t,uint32_t> ram{{0x27b4,0x90a03000},{0x27b8,0x501d84},
        {0x11fa0,0x90a03000},{0x11fa4,0x501d84},{0x26d90,0x80a03000},{0x26d94,0x501d18},
        {0x120c8,0x92b03000},{0x120cc,0x501d84},{0x501d84,9},{0x501d88,3},
        {0x41c74,0x9a703000},{0x41c78,0x524990},{0x422c8,0x9a703000},{0x422cc,0x524990},
        {0x423c0,0x59084810},{0x423c4,0x90a46048},
        {0x3f570,0x90a03000},{0x3f574,0x501e78},{0x3fac0,0x90a03000},{0x3fac4,0x501e7c}};
    uint32_t operator()(uint32_t a){return ram[a];}
    void object(unsigned p,uint32_t owner,uint32_t child) {
        ram[0x524990]=owner;ram[owner+4]=p?0x3fac0:0x3f570;
        ram[child+4]=0x423c0;ram[owner+0x10]=child;ram[child+0x14]=owner;
    }
    GameState state(uint64_t session=1) {auto s=read_game_state(*this);s.session=session;return s;}
};
int main(int argc,char** argv) {
    // Reproduce a weak handle whose address space retains the callback. A write
    // after the observing scope ends must not reach the destroyed owner.
    struct Space {std::function<void()> callback;void write(){if(callback)callback();}};
    struct Handle {std::weak_ptr<Space> space;void remove(){if(auto s=space.lock())s->callback={};}};
    auto space=std::make_shared<Space>();unsigned notifications{};
    {
        OwnedWriteTap<Handle> tap;tap.handle.space=space;
        space->callback=[&]{++notifications;};space->write();assert(notifications==1);
    }
    space->write();assert(notifications==1);
    Fixture f;NativeShotJournal journal;auto state=f.state();
    journal.publish(state,f);assert(state.shot_counters_valid&&state.counting_shots);
    f.object(0,0x512170,0x51ba70);
    assert(journal.record(0x41c7c,0x524994,0x51ba70,0xffffffff,f));
    f.object(1,0x512cb0,0x51f370);
    assert(journal.record(0x422d0,0x524994,0x51f370,0xffffffff,f));
    journal.publish(state,f);assert(state.players[0].shots==1&&state.players[1].shots==1);
    // Reused allocation addresses are separate real native creation writes.
    assert(journal.record(0x422d0,0x524994,0x51f370,0xffffffff,f));
    journal.publish(state,f);assert(state.players[1].shots==2);
    assert(!journal.record(0x107b34,0x524994,0,0xffffffff,f));
    assert(!journal.record(0x422d0,0x524990,0x512cb0,0xffffffff,f));
    assert(!journal.record(0x422d0,0x524994,0x51f370,0xffff,f));
    assert(!journal.record(0x41c7c,0x524994,0x51f370,0xffffffff,f));
    for(auto bad:{0u,0x400000u,0x600000u,0x51f371u})
        assert(!journal.record(0x422d0,0x524994,bad,0xffffffff,f));
    f.ram[0x51f374]=0x42650;assert(!journal.record(0x422d0,0x524994,0x51f370,0xffffffff,f));
    f.object(1,0x512cb0,0x51f370);f.ram[0x51f384]=0;
    assert(!journal.record(0x422d0,0x524994,0x51f370,0xffffffff,f));
    f.object(1,0x512cb0,0x51f370);f.ram[0x501d88]=1;
    assert(!journal.record(0x422d0,0x524994,0x51f370,0xffffffff,f));
    f.ram[0x501d88]=3;f.ram[0x501d84]=14;
    assert(!journal.record(0x422d0,0x524994,0x51f370,0xffffffff,f));
    state=f.state();journal.publish(state,f);assert(!state.shot_counters_valid);
    f.ram[0x501d84]=9;state=f.state(2);journal.publish(state,f);
    assert(state.shot_counters_valid&&state.players[0].shots==0&&state.players[1].shots==0);
    f.ram[0x41c78]=0;assert(!journal.record(0x422d0,0x524994,0x51f370,0xffffffff,f));
    journal.publish(state,f);assert(!state.shot_counters_valid);
    journal.suspend();state=f.state(3);journal.publish(state,f);assert(!state.shot_counters_valid);
    if(argc>1) {
        Fixture evidence;NativeShotJournal measured;auto current=evidence.state();measured.publish(current,evidence);
        std::ifstream input(argv[1]);assert(input);std::string line;std::getline(input,line);
        unsigned accepted{},writes{};
        while(std::getline(input,line)) {
            std::istringstream row(line);double time;uint32_t pc,address,data,mask,owner,owner_cb,child_cb,mode,active,owner_child,child_owner;
            row>>time>>std::hex>>pc>>address>>data>>mask>>owner>>owner_cb>>child_cb>>std::dec>>mode>>active>>std::hex>>owner_child>>child_owner;
            assert(row);++writes;
            evidence.ram[0x501d84]=mode;evidence.ram[0x501d88]=active;evidence.ram[0x524990]=owner;
            evidence.ram[owner+4]=owner_cb;evidence.ram[owner+0x10]=owner_child;
            if(address==0x524994&&data) {evidence.ram[data+4]=child_cb;evidence.ram[data+0x14]=child_owner;}
            accepted+=measured.record(pc,address,data,mask,evidence);
        }
        current=evidence.state();measured.publish(current,evidence);
        assert(writes==6164&&accepted==517);
        assert(current.players[0].shots==317&&current.players[1].shots==200);
        std::cout<<"Native projectile replay: "<<writes<<" writes, "<<accepted<<" accepted (P1 317 / P2 200), zero cleanup/foreign events\n";
    }
}
