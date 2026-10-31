// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_scene.h"
#include "reticle_mode.h"
#include <glm/glm.hpp>
#include <memory>
namespace rchase2 {
class GLScene {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    GLScene();~GLScene();
    void set_hud_size(float size,bool gameplay,bool service=false);
    // Optional camera-space prototype. Native/reference presentation is default.
    void set_immersive(bool enabled);
    // Compatibility switch for diagnostic callers; gameplay context defaults Off.
    void set_native_reticles_enabled(bool enabled);
    void set_reticle_mode(ReticleMode mode,bool gameplay);
    void set_reticle_size(float minimum_ratio,float shrink_distance);
    void set_reticle_sizes(float ingame_min,float ingame_max,float custom_min,float custom_max,float shrink_distance);
    // Game-only decoration; reference/replay callers retain the native image.
    void set_cart_bottom_enabled(bool enabled);
    void prepare(std::shared_ptr<const NativeFrame> frame,bool wider_view=false);
    void set_presentation_lens(const NativeLens& lens);
    void set_reticle_targets(const std::array<std::optional<Vec3>,2>& targets,const glm::mat4& head);
    SceneAim trace_aim(Ray ray,float world_scale,float panel_distance) const;
    void draw(const glm::mat4& view_projection,float world_scale,float panel_distance);
};
}
