// SPDX-License-Identifier: GPL-2.0-or-later
// Diagnostic executable only. Run the exact ARM64 engine without an Activity.
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
extern "C" const char* SDL_AndroidGetExternalStoragePath(){return std::getenv("RCHASE2_BENCH_ROOT");}
extern "C" const char* SDL_AndroidGetInternalStoragePath(){return SDL_AndroidGetExternalStoragePath();}
extern "C" void* SDL_AndroidGetJNIEnv(){return nullptr;}
int main(int argc,char** argv) {
    setenv("SDL_VIDEODRIVER","dummy",1);setenv("SDL_AUDIODRIVER","dummy",1);
    void* module=dlopen(std::getenv("RCHASE2_BENCH_LIBRARY"),RTLD_NOW|RTLD_GLOBAL);
    if(!module){std::puts(dlerror());return 2;}
    auto ready=reinterpret_cast<void(*)()>(dlsym(module,"SDL_SetMainReady"));
    auto run=reinterpret_cast<int(*)(int,char**)>(dlsym(module,"SDL_main"));
    if(!ready||!run){std::puts(dlerror());return 3;}
    ready();return run(argc,argv);
}
