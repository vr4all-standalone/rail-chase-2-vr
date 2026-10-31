// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/gun_pose.h"
#include "../native/arcade_color.h"
#include <cassert>
#include <cmath>
int main() {
    using namespace rchase2;
    auto anchored=gun_pose(glm::translate(glm::mat4(1),glm::vec3(1,2,3)),glm::vec3(4,5,6));
    assert(anchored[3]==glm::vec4(4,5,6,1));
    const auto angle=pitched_gun_pose(glm::mat4(1),0);
    const auto tip=angle*glm::vec4(0,0,-1,0);
    assert(std::abs(tip.y+std::sin(glm::radians(10.f)))<1e-6);
    const auto clamped=pitched_gun_pose(glm::mat4(1),100);
    const auto maximum=pitched_gun_pose(glm::mat4(1),30);
    assert(glm::length(clamped[2]-maximum[2])<1e-6);
    auto left=gun_model_transform(glm::mat4(1),1,0,glm::mat4(1));
    auto right=gun_model_transform(glm::mat4(1),1,1,glm::mat4(1));
    assert(left[0].x==-1 && right[0].x==1 && left[2]==right[2]);
    std::array<float,4> metal{.15f,.15f,.17f,1};
    assert(arcade_part_color(2,0,metal)==metal && arcade_part_color(3,1,metal)==metal);
    assert(arcade_part_color(0,0,metal)[0]==.90f && arcade_part_color(1,0,metal)[0]==.90f);
    assert(arcade_part_color(0,1,metal)[2]==1 && arcade_part_color(1,1,metal)[2]==1);
}
