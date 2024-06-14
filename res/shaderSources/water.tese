#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "water.h"

layout(quads, equal_spacing, ccw) in;

layout(location = 0) out vec3 norm;
layout(location = 1) out vec3 worldPos;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;

    vec4 p = (p1 - p0) * v + p0;

    p.y = WaterHeight(p.xyz, norm);
    worldPos = p.xyz;
    gl_Position = projView * p;
}