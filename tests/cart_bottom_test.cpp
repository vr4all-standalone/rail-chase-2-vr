// SPDX-License-Identifier: GPL-2.0-or-later
#include "cart_bottom_fixture.h"
#include "../native/cart_bottom.h"
#include "../native/shared_runtime.h"
#include <cassert>
#include <iostream>
using namespace rchase2;
#ifdef _WIN32
static_assert(sizeof(SharedRuntime)==456&&sizeof(NativeFrame)==208&&sizeof(CabinetInput)==28);
#elif defined(__ANDROID__)
static_assert(sizeof(SharedRuntime)==520&&sizeof(NativeFrame)==208&&sizeof(CabinetInput)==28);
#endif
int main(){
    auto f=cart_fixture();auto pose=cart_bottom_pose(f);assert(pose.valid);
    NativeLens lens{496,384,384,384,248,192};
    auto point=cart_bottom_world({6.8,9.5,9.8},pose,f,lens,.1f);
    assert(std::abs(point.x-1.36)<1e-5&&std::abs(point.y+.86)<1e-5&&std::abs(point.z+.98)<1e-5);
    point=cart_bottom_world({0,9.5,-9.8},pose,f,lens,.1f);
    assert(std::abs(point.z-.98)<1e-5&&std::abs(point.y+.86)<1e-5); // Signed rear half survives.
    auto reject=[&](auto change){auto bad=f;change(bad);assert(!cart_bottom_pose(bad).valid);};
    reject([](auto& b){b.complete=false;});reject([](auto& b){b.rejected=1;});
    reject([](auto& b){b.polygons.pop_back();});reject([](auto& b){b.polygons[0].object=123;});
    reject([](auto& b){b.polygons[50].matrix[9]+=.1f;});
    reject([](auto& b){b.polygons[0].matrix[0]=NAN;});
    reject([](auto& b){b.polygons[0].vertices[0].x+=100;});
    reject([](auto& b){b.polygons[40].header[2]++;});
    reject([](auto& b){b.polygons[0].focus[0]=1;});
    reject([](auto& b){b.polygons[99].vertices[0].z=INFINITY;});
    reject([](auto& b){for(auto& p:b.polygons)p.matrix[0]=2;});
    assert(cart_bottom_mesh().size()==96);
    for(const auto& v:cart_bottom_mesh())assert(v.y>=1.3f&&v.y<=9.51f&&std::abs(v.x)<=6.81f&&std::abs(v.z)<=9.81f);
    // A recessed base plus continuous vertical timber makes an interior,
    // rather than a flat platform meeting the original shell's lower edge.
    const auto& mesh=cart_bottom_mesh();
    for(size_t edge=0;edge<8;edge++){
        size_t at=edge*12;
        assert(std::abs(mesh[at].y-1.5f)<1e-5f);
        assert(std::abs(mesh[at+3].y-1.3f)<1e-5f);
        assert(std::abs(mesh[at+6].y-9.51f)<1e-5f&&std::abs(mesh[at+11].y-9.51f)<1e-5f);
        assert(std::abs(mesh[at+7].y-1.3f)<1e-5f&&std::abs(mesh[at+8].y-1.3f)<1e-5f);
    }
    // Preserve the floor texture which the user physically accepted.
    assert(std::abs(mesh[0].u-61.74f)<1e-4f&&mesh[0].v==0);
    assert(std::abs(mesh[1].u-2.457f)<1e-4f&&std::abs(mesh[1].v+233.2425f)<1e-4f);
    std::cout<<"Cart identity, shell fit, signed presentation mapping and fail-closed checks pass\n";
}
