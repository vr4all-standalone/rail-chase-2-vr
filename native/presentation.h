// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "shared_runtime.h"
#include "gun_pose.h"
#include <glm/glm.hpp>
#include <memory>
namespace rchase2 {
struct PresentationInput {
    Hands hands;
    std::array<std::optional<Vec3>,2> aim_targets{};
    std::array<glm::mat4,2> poses{glm::mat4(1),glm::mat4(1)};
    std::array<std::optional<glm::vec3>,2> grips{};
    glm::mat4 gun_pose(int hand) const{return rchase2::gun_pose(poses[hand],grips[hand]);}
    glm::mat4 head{1};glm::vec2 stick{};
    bool toggle{};
    bool focused{true};
};
class Presentation {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    Presentation();~Presentation();
    bool open() const;
    bool service() const;
    bool service_guns() const;
    void toggle_service_guns();
    void prepare(const Settings&,SharedRuntime* runtime=nullptr);
    void suspend();
    bool recenter_requested();
    glm::vec3 muzzle(const glm::mat4& aim_pose,const Settings&,int hand=1) const;
    void update(const PresentationInput&,Settings&,SharedRuntime&,double now);
    bool reticles_active(const GameState&) const;
    std::array<std::optional<Vec3>,2> reticle_targets(const GameState&) const;
    void feedback(const CabinetInput&,double now);
    void draw(const glm::mat4& vp,const Settings&,const GameState&,bool scene_ready,double now,bool splash=false);
};
}
