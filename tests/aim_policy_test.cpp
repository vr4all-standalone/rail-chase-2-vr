// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/aim_policy.h"
#include <cassert>
#include <cmath>
int main() {
    using namespace rchase2;
    NativeAimCalibration c;c.valid=c.screen_inverse_valid=true;c.left=c.top=192;c.right=c.bottom=64;
    auto p=native_aim_point(c,130,115);assert(p.valid);
    assert(std::abs(p.x-(-.52978515625))<1e-9 && std::abs(p.y-(-.228515625))<1e-9 && p.z==6);
    c.player=1;p=native_aim_point(c,125,115);
    assert(std::abs(p.x-.552490234375)<1e-9);
    auto raw=native_screen_to_adc(1,(248+p.x*128)/495,(192-p.y*128)/383,c);
    assert(raw.valid && raw.x==125 && raw.y==115);
    assert(!native_screen_to_adc(0,.5,.5,c).valid);
    c.right=128;assert(!native_aim_point(c,128,128).valid);
    NativeAimPolicy policy;CabinetInput input;input.hand={0,-1};input.cabinet.trigger[0]=true;
    Hands hands;hands[0].tracked=true;hands[0].trigger=true;hands[0].aim={.5,.5,true,true};
    std::array<NativeAimCalibration,2> invalid{};policy.apply(input,hands,invalid);
    assert(!input.cabinet.trigger[0]);
}
