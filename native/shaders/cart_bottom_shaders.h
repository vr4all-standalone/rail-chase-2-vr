// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model2_shaders.h"
#include <string>
#include <stdexcept>
namespace rchase2 {
inline constexpr const char* CartBottomVertexShader=R"(#version 330
layout(location=0) in vec3 local_position;layout(location=1) in vec2 local_uv;
uniform mat4 vp;uniform mat4 cart_to_camera;uniform vec4 lens;uniform vec4 native_lens;
uniform float scale;uniform bool immersive;uniform int luma;uniform int lod;
out vec3 native_position;out vec3 texture_position;
flat out int source_polygon;flat out int world_geometry;flat out int hidden_reticle;
flat out ivec4 header;flat out int polygon_luma;flat out int polygon_lod;
void main(){
    vec3 camera=(cart_to_camera*vec4(local_position,1.0)).xyz;
    vec3 homogeneous=vec3(native_lens.z*camera.z+native_lens.x*camera.x,native_lens.w*camera.z-native_lens.y*camera.y,camera.z);
    vec3 world=immersive?vec3(camera.xy,-camera.z):vec3((homogeneous.x-lens.z*camera.z)/lens.x,(lens.w*camera.z-homogeneous.y)/lens.y,-camera.z);
    gl_Position=vp*vec4(world*scale,1.0);
    native_position=homogeneous*scale;texture_position=vec3(local_uv,1.0)*scale;
    source_polygon=0;world_geometry=2;hidden_reticle=0;
    header=ivec4(16594,0,2200,7424);polygon_luma=luma;polygon_lod=lod;
})";
inline std::string cart_bottom_fragment_shader(){
    // Keep original timber texel/palette/luma/gamma math. This authored mesh
    // has continuous vertex UVs rather than a native raster scanline table.
    std::string shader=Model2Fragment;
    const std::string native="if(world_geometry!=0&&source_record(source_polygon*2+1).w!=0.0)";
    const auto at=shader.find(native);
    if(at==std::string::npos)throw std::runtime_error("Native material shader lacks cart UV seam");
    shader.replace(at,native.size(),"if(world_geometry==2||(world_geometry!=0&&source_record(source_polygon*2+1).w!=0.0))");
    return shader;
}
}
