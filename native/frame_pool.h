// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_capture.h"
#include <mutex>
namespace rchase2 {
class NativeFramePool {
    struct State {
        std::mutex mutex;
        std::vector<std::unique_ptr<NativeFrame>> free;
        State(){free.reserve(4);}
    };
    std::shared_ptr<State> state_=std::make_shared<State>();
public:
    NativeFrame acquire(){
        std::unique_ptr<NativeFrame> frame;
        {std::lock_guard lock(state_->mutex);if(state_->free.empty())return {};frame=std::move(state_->free.back());state_->free.pop_back();}
        return std::move(*frame);
    }
    std::shared_ptr<const NativeFrame> publish(std::unique_ptr<NativeFrame> frame){
        if(!frame)return {};
        return {frame.release(),[state=state_](const NativeFrame* data)noexcept{
            std::unique_ptr<NativeFrame> owned(const_cast<NativeFrame*>(data));
            std::lock_guard lock(state->mutex);
            if(state->free.size()<4)state->free.push_back(std::move(owned));
        }};
    }
};
}
