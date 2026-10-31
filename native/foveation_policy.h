// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <openxr/openxr.h>
#include <cstring>
namespace rchase2 {
enum class FoveationMode {Raw,None,Medium,High,Cycle,CycleHigh};
inline FoveationMode foveation_mode(const char* value) {
    if(value&&std::strcmp(value,"none")==0)return FoveationMode::None;
    if(value&&std::strcmp(value,"medium")==0)return FoveationMode::Medium;
    if(value&&std::strcmp(value,"high")==0)return FoveationMode::High;
    if(value&&std::strcmp(value,"cycle")==0)return FoveationMode::Cycle;
    if(value&&std::strcmp(value,"cycle-high")==0)return FoveationMode::CycleHigh;
    return FoveationMode::Raw;
}
inline const char* foveation_mode_name(FoveationMode mode) {
    switch(mode){case FoveationMode::None:return "none";case FoveationMode::Medium:return "medium";case FoveationMode::High:return "high";case FoveationMode::Cycle:return "cycle";case FoveationMode::CycleHigh:return "cycle-high";default:return "raw";}
}
inline bool foveation_cycle_mode(FoveationMode mode){return mode==FoveationMode::Cycle||mode==FoveationMode::CycleHigh;}
inline XrFoveationLevelFB foveation_requested_level(FoveationMode mode,int phase=-1) {
    if(mode==FoveationMode::High||(mode==FoveationMode::CycleHigh&&(phase==1||phase==2||phase==4)))return XR_FOVEATION_LEVEL_HIGH_FB;
    if(mode==FoveationMode::Medium||(mode==FoveationMode::Cycle&&(phase==1||phase==2)))return XR_FOVEATION_LEVEL_MEDIUM_FB;
    return XR_FOVEATION_LEVEL_NONE_FB;
}
inline const char* foveation_level_name(XrFoveationLevelFB level){return level==XR_FOVEATION_LEVEL_HIGH_FB?"high":level==XR_FOVEATION_LEVEL_MEDIUM_FB?"medium":"none";}
inline XrSwapchainCreateInfoFoveationFB foveation_chain_info() {
    // Meta's GLES reference leaves flags zero, selecting the runtime's native
    // method. Explicit SCALED_BIN requires supported AND enabled QCOM state.
    return {XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB,nullptr,0};
}
// One bounded 4 x 30-wall-second ABBA experiment. Phase 4 is completion;
// each mode decides its completion profile and never starts another cycle.
class FoveationCycle {
    double start_{};int phase_{-1};bool disabled_{};
public:
    void disable(){disabled_=true;}
    int phase()const{return phase_;}
    int advance(double now,bool splash,bool focused,bool menu_open) {
        if(disabled_||phase_==4)return -1;
        if(phase_<0){if(splash||!focused||!menu_open)return -1;start_=now;phase_=0;return 0;}
        int next=now-start_>=120.?4:now-start_>=90.?3:now-start_>=60.?2:now-start_>=30.?1:0;
        if(next<=phase_)return -1;phase_=next;return phase_;
    }
};
// The caller must recreate BOTH ordinary chains before drawing if any stage
// fails: an update is not a stereo transaction and may alter just one eye.
// Profiles may be destroyed immediately after their swapchain updates (spec).
template<class Create,class Update,class Destroy>
XrResult apply_static_foveation(XrSession session,const XrSwapchain (&eyes)[2],XrFoveationLevelFB requested,
                               Create create,Update update,Destroy destroy) {
    XrFoveationLevelProfileCreateInfoFB level{XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB};
    level.level=requested;
    level.verticalOffset=0.f;level.dynamic=XR_FOVEATION_DYNAMIC_DISABLED_FB;
    XrFoveationProfileCreateInfoFB info{XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB};info.next=&level;
    XrFoveationProfileFB profile{};auto result=create(session,&info,&profile);
    if(XR_FAILED(result))return result;
    XrSwapchainStateFoveationFB state{XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB};state.profile=profile;
    for(auto eye:eyes){result=update(eye,reinterpret_cast<const XrSwapchainStateBaseHeaderFB*>(&state));if(XR_FAILED(result))break;}
    const auto destroyed=destroy(profile);
    return XR_FAILED(result)?result:destroyed;
}
}
