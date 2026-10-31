// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
namespace rchase2 {
inline std::array<float,4> arcade_part_color(unsigned part,unsigned player,const std::array<float,4>& original) {
    if(part>1)return original; // Preserve metal screws and smoked lens.
    return player==1?std::array<float,4>{.04f,.25f,1.f,1.f}:std::array<float,4>{.90f,.04f,.04f,1.f};
}
}
