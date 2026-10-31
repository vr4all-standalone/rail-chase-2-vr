// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace rchase2 {
// Common owned-mesh format only. No VC2 model table or decoder is used here.
struct NativeWeaponVertex {float x{},y{},z{},u{},v{};};
struct NativeWeaponPolygon {unsigned count{};uint32_t attr{};std::array<uint16_t,4> header{};std::array<float,3> normal{};std::array<NativeWeaponVertex,4> vertices{};};
struct NativeWeapon {bool valid{};uint32_t weapon{},model{},object{};std::vector<NativeWeaponPolygon> polygons;};
}
