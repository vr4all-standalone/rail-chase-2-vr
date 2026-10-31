// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../native/foveation_policy.h"
#include <cassert>
#include <vector>
using namespace rchase2;
int main() {
    assert(foveation_mode(nullptr)==FoveationMode::Raw);
    assert(foveation_mode("MEDIUM")==FoveationMode::Raw);
    assert(foveation_mode("medium")==FoveationMode::Medium);
    assert(foveation_mode("none")==FoveationMode::None);
    assert(foveation_mode("cycle")==FoveationMode::Cycle);
    assert(foveation_mode("high")==FoveationMode::High);
    assert(foveation_mode("cycle-high")==FoveationMode::CycleHigh);
    assert(foveation_mode("HIGH")==FoveationMode::Raw);
    assert(foveation_mode("cycle-high ")==FoveationMode::Raw);
    auto chain=foveation_chain_info();
    assert(chain.type==XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB&&!chain.next&&!chain.flags);
    FoveationCycle cycle;
    assert(cycle.advance(1.,true,true,true)==-1); // splash cannot start the clock
    assert(cycle.advance(2.,false,false,true)==-1); // neither can an unfocused frame
    assert(cycle.advance(1000.,false,true,false)==-1); // playing indefinitely cannot start it
    assert(cycle.advance(1010.,false,true,true)==0);
    assert(cycle.advance(1039.999,false,true,true)==-1);
    assert(cycle.advance(1040.,false,true,false)==1); // menu closing does not pause the wall clock
    assert(cycle.advance(1070.,false,true,true)==2); // second B is a distinct logged phase
    assert(cycle.advance(1100.,false,true,true)==3);
    assert(cycle.advance(1130.,false,true,true)==4); // completion, no repeated experiment
    assert(cycle.advance(2000.,false,true,true)==-1);
    for(auto mode:{FoveationMode::Cycle,FoveationMode::CycleHigh}) {
        assert(foveation_cycle_mode(mode));
        assert(foveation_requested_level(mode,-1)==XR_FOVEATION_LEVEL_NONE_FB);
        assert(foveation_requested_level(mode,0)==XR_FOVEATION_LEVEL_NONE_FB);
        const auto selected=mode==FoveationMode::Cycle?XR_FOVEATION_LEVEL_MEDIUM_FB:XR_FOVEATION_LEVEL_HIGH_FB;
        assert(foveation_requested_level(mode,1)==selected&&foveation_requested_level(mode,2)==selected);
        assert(foveation_requested_level(mode,3)==XR_FOVEATION_LEVEL_NONE_FB);
        assert(foveation_requested_level(mode,4)==(mode==FoveationMode::Cycle?XR_FOVEATION_LEVEL_NONE_FB:XR_FOVEATION_LEVEL_HIGH_FB));
    }
    assert(foveation_requested_level(FoveationMode::High)==XR_FOVEATION_LEVEL_HIGH_FB);
    assert(foveation_requested_level(FoveationMode::Medium)==XR_FOVEATION_LEVEL_MEDIUM_FB);
    assert(foveation_requested_level(FoveationMode::Raw)==XR_FOVEATION_LEVEL_NONE_FB);
    FoveationCycle disabled;disabled.disable();assert(disabled.advance(1.,false,true,true)==-1);
    FoveationCycle interrupted;assert(interrupted.advance(0.,false,true,true)==0);
    assert(interrupted.advance(90.,true,false,false)==3); // wall time once started, despite focus/menu changes

    const XrSwapchain eyes[]={reinterpret_cast<XrSwapchain>(1),reinterpret_cast<XrSwapchain>(2)};
    const auto profile=reinterpret_cast<XrFoveationProfileFB>(3);
    for(auto requested:{XR_FOVEATION_LEVEL_NONE_FB,XR_FOVEATION_LEVEL_MEDIUM_FB,XR_FOVEATION_LEVEL_HIGH_FB})for(int failure:{-1,0,1,2,3}) {
        int creates{},destroys{};std::vector<int> updates;
        auto create=[&](XrSession,const XrFoveationProfileCreateInfoFB* info,XrFoveationProfileFB* out) {
            ++creates;assert(info->type==XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB);
            const auto* level=static_cast<const XrFoveationLevelProfileCreateInfoFB*>(info->next);
            assert(level&&level->type==XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB&&!level->next);
            assert(level->level==requested&&level->verticalOffset==0.f&&level->dynamic==XR_FOVEATION_DYNAMIC_DISABLED_FB);
            if(failure==0)return XR_ERROR_RUNTIME_FAILURE;*out=profile;return XR_SUCCESS;
        };
        auto update=[&](XrSwapchain eye,const XrSwapchainStateBaseHeaderFB* base) {
            const auto* state=reinterpret_cast<const XrSwapchainStateFoveationFB*>(base);
            assert(state->type==XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB&&!state->next&&!state->flags&&state->profile==profile);
            const int which=eye==eyes[0]?1:2;updates.push_back(which);
            return failure==which?XR_ERROR_RUNTIME_FAILURE:XR_SUCCESS;
        };
        auto destroy=[&](XrFoveationProfileFB handle){assert(handle==profile);++destroys;return failure==3?XR_ERROR_RUNTIME_FAILURE:XR_SUCCESS;};
        auto result=apply_static_foveation(XR_NULL_HANDLE,eyes,requested,create,update,destroy);
        assert(creates==1&&destroys==(failure==0?0:1));
        assert(updates.size()==size_t(failure==0?0:failure==1?1:2));
        assert((XR_SUCCEEDED(result))==(failure==-1));
    }
}
