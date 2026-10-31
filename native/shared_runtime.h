// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_capture.h"
#include "snapshot_slot.h"
#include "input_policy.h"
#include "game_state.h"
#include "native_weapon.h"
#include <atomic>
#include <mutex>
namespace rchase2 {
struct NativeSnapshot {
    GameState state;
    std::array<NativeAimCalibration,2> calibration;
    ShotOwnership ownership;
};
struct SharedRuntime {
    static constexpr uint32_t Abi=0x52433201;
    uint32_t abi{Abi};
    SnapshotSlot<const NativeFrame> scene;
    std::mutex mutex;
    GameState state;
    ShotTracker shot_tracker;
    ShotOwnership shot_ownership;
    std::array<NativeAimCalibration,2> calibration;
    std::array<std::shared_ptr<const NativeWeapon>,7> weapons;
    CabinetInput input,engine_input;
    CabinetLatch input_latch;OperatorLatch operator_latch;
    uint64_t session{1};
    std::atomic<bool> host_ready{},pause_requested{},quit_requested{},service_requested{},service_active{};
    std::atomic<uint16_t> operator_pending{},operator_latched{};
    NativeSnapshot native_snapshot(){std::lock_guard lock(mutex);return {state,calibration,shot_ownership};}
    void release_inputs(){std::lock_guard lock(mutex);input={};engine_input={};input_latch={};operator_latch={};operator_pending=0;operator_latched=0;}
    void reset(){scene.store({});weapons={};{std::lock_guard lock(mutex);state={};calibration={};shot_tracker.suspend();shot_ownership={};input={};engine_input={};input_latch={};operator_latch={};++session;}host_ready=false;pause_requested=false;quit_requested=false;service_requested=false;service_active=false;operator_pending=0;operator_latched=0;}
};
inline SharedRuntime& shared_runtime(){static SharedRuntime runtime;return runtime;}
}
