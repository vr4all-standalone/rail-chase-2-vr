// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/launch_policy.h"
#include <cassert>
using namespace rchase2;
using namespace std::chrono_literals;
int main(){
    NativeFrame boot;boot.width=496;boot.height=384;boot.complete=true;
    boot.native.assign(size_t(boot.width)*boot.height,0xff000000);
    // A full native raster is presentable before the game accepts Start. A
    // nonblack warning/credits pixel dismisses branding on that same frame.
    assert(native_frame_presentable(&boot));
    LaunchPolicy content;content.observe(&boot);assert(content.active());
    boot.native[100*boot.width+200]=0xffffffff;
    content.observe(&boot);assert(!content.active());
    content.presented(10ms);content.observe(nullptr);assert(!content.active());
    LaunchPolicy incomplete;boot.complete=false;
    assert(!native_frame_presentable(&boot));incomplete.observe(&boot);assert(incomplete.active());
    assert(!native_frame_presentable(nullptr));
    boot.complete=true;boot.native.clear();assert(!native_frame_presentable(&boot));
    boot.foreground.assign(size_t(boot.width)*boot.height,0);boot.foreground[10]=0xff123456;
    LaunchPolicy foreground;foreground.observe(&boot);assert(!foreground.active());
    boot.foreground.clear();NativePolygon world;world.count=3;boot.polygons={world};
    LaunchPolicy geometry;geometry.observe(&boot);assert(!geometry.active());
    LaunchPolicy hardcap;hardcap.presented(0ns);hardcap.presented(-1ms);assert(hardcap.active());
    hardcap.presented(999ms);assert(hardcap.active());hardcap.presented(1ms);assert(!hardcap.active());
    LaunchPolicy delayed;delayed.presented(2s);assert(!delayed.active());
}
