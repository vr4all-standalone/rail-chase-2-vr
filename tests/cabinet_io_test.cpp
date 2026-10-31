// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/cabinet_io.h"
#include <cassert>
#include <limits>
int main() {
    using namespace rchase2;
    Cabinet c;
    assert(c.x[0]==0x82 && c.x[1]==0x7d && c.y[0]==0x73 && c.y[1]==0x73);
    c.x={0x11,0x22};c.y={0x33,0x44};
    assert(adc(0,c)==0x22 && adc(1,c)==0x11 && adc(2,c)==0x44 && adc(3,c)==0x33);
    assert(adc(4,c)==0xff);
    c.trigger={true,false};assert(in1(0xff,c)==0xfe);
    c.trigger={false,true};assert(in1(0xff,c)==0xfd);
    c.trigger={true,true};assert(in1(0xfc,c)==0xfc);
    c.coins=2;c.starts=1;c.test=true;c.service=true;assert(in0(0xff,c)==0xe1);
    assert(in0(0,c)==0);
    auto tl=map_aim(0,0,0),br=map_aim(0,1,1);
    assert(tl.valid && tl.x==0xca && tl.y==0xcb);
    assert(br.valid && br.x==0x3a && br.y==0x1c);
    tl=map_aim(1,-10,-10);br=map_aim(1,10,10);
    assert(tl.x==0xc7 && tl.y==0xcb && br.x==0x34 && br.y==0x1c);
    assert(!map_aim(2,.5,.5).valid);
    assert(!map_aim(0,std::numeric_limits<double>::quiet_NaN(),.5).valid);
    assert(!map_aim(0,.5,std::numeric_limits<double>::infinity()).valid);
    assert(!map_aim(0,.5,.5,{1,1,2,3}).valid);
}
