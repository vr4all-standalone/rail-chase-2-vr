// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
namespace rchase2 {
inline constexpr const char* CustomReticleVertexShader=R"(#version 330
layout(location=0) in vec3 shape;
uniform mat4 vp;uniform mat4 model;uniform vec2 half_size;uniform vec3 player_color;
out vec3 vertex_color;
void main(){
    gl_Position=vp*model*vec4(shape.xy*half_size,0,1);
    vertex_color=player_color;
})";
inline constexpr const char* CustomReticleFragmentShader=R"(#version 330
in vec3 vertex_color;out vec4 color;
void main(){color=vec4(vertex_color,1);})";
}
