// SPDX-License-Identifier: GPL-2.0-or-later
// Diagnostic only: native capture with no XR rendering or synthetic inputs.
#include "../native/shared_runtime.h"
#ifdef _WIN32
#define BENCH_EXPORT extern "C" __declspec(dllexport)
#else
#define BENCH_EXPORT extern "C" __attribute__((visibility("default")))
#endif
static rchase2::SharedRuntime* runtime;
BENCH_EXPORT bool rchase2_xr_start(rchase2::SharedRuntime* shared) {
    if(!shared||shared->abi!=rchase2::SharedRuntime::Abi)return false;
    runtime=shared;runtime->host_ready=true;runtime->pause_requested=false;return true;
}
BENCH_EXPORT void rchase2_xr_stop(){if(runtime)runtime->host_ready=false;runtime=nullptr;}
