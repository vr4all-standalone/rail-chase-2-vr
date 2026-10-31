// SPDX-License-Identifier: GPL-2.0-or-later
// Shared physical pose policy adapted from VR4ALL Virtua Cop 2, with no weapon data.
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <optional>
namespace rchase2 {
inline glm::mat4 gun_pose(glm::mat4 aim,const std::optional<glm::vec3>& grip) {
    if(grip)aim[3]=glm::vec4(*grip,1);
    return aim;
}
inline glm::mat4 pitched_gun_pose(const glm::mat4& pose,float displayed_angle) {
    const auto angle=std::isfinite(displayed_angle)?std::clamp(displayed_angle,-30.f,30.f):0.f;
    return pose*glm::rotate(glm::mat4(1),glm::radians(angle-10.f),glm::vec3(1,0,0));
}
inline glm::mat4 gun_model_transform(const glm::mat4& pose,float scale,int hand,const glm::mat4& placement) {
    return pose*glm::scale(glm::mat4(1),glm::vec3(hand==0?-scale:scale,scale,scale))*placement;
}
}
