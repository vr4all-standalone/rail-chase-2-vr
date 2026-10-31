// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/adaptive_audio_bridge.h"
#include <array>
#include <bit>
#include <cassert>
#include <memory>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// The real shared library's exported ABI, rather than an import library, is
// exercised with the same independent ownership as the SDL engine adapter.
int main(int argc,char** argv) {
    assert(argc==2);
#ifdef _WIN32
    auto handle=LoadLibraryA(argv[1]);assert(handle);
    auto symbol=[&](const char* name){return GetProcAddress(handle,name);};
#else
    auto handle=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(handle);
    auto symbol=[&](const char* name){return dlsym(handle,name);};
#endif
    bool unloaded=false;
    auto module=std::shared_ptr<void>(reinterpret_cast<void*>(handle),[&](void*) {
#ifdef _WIN32
        assert(FreeLibrary(handle));
#else
        assert(dlclose(handle)==0);
#endif
        unloaded=true;
    });
    rchase2::AudioStretchFunctions api;
    api.create=std::bit_cast<decltype(api.create)>(symbol("rchase2_audio_create"));
    api.push=std::bit_cast<decltype(api.push)>(symbol("rchase2_audio_push"));
    api.read=std::bit_cast<decltype(api.read)>(symbol("rchase2_audio_read"));
    api.pause=std::bit_cast<decltype(api.pause)>(symbol("rchase2_audio_pause"));
    api.stats=std::bit_cast<decltype(api.stats)>(symbol("rchase2_audio_stats"));
    api.destroy=std::bit_cast<decltype(api.destroy)>(symbol("rchase2_audio_destroy"));
    rchase2::AudioStretchBridge bridge;
    assert(bridge.open(api,module,48000,2)==rchase2::AudioStretchOpen::Ready);
    module.reset();assert(!unloaded);
    std::array<int16_t,1024> output;output.fill(123);
    assert(bridge.read(output.data(),512,100.,true)==0);
    for(auto sample:output)assert(sample==0);
    Rchase2AudioStats stats;assert(bridge.snapshot(stats));
    assert(stats.state==Rchase2AudioPaused&&stats.paused_frames==512);
    assert(stats.requested_frames==stats.delivered_frames+stats.missing_frames+stats.paused_frames);
    bridge.reset();assert(unloaded&&!bridge.active());
}
