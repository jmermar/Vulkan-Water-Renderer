#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(vertices = 4) out;

#include "water.h"

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    vec4 view00 = view * gl_in[0].gl_Position;
    vec4 view01 = view * gl_in[1].gl_Position;
    vec4 view10 = view * gl_in[2].gl_Position;
    vec4 view11 = view * gl_in[3].gl_Position;

    float len00 = length(view00.xyz);
    float len01 = length(view01.xyz);
    float len10 = length(view10.xyz);
    float len11 = length(view11.xyz);

    const float MIN_DISTANCE = 1;
    const float MAX_DISTANCE = 100;

    float dis00 =
        clamp((len00 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0, 1);
    float dis01 =
        clamp((len01 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0, 1);
    float dis10 =
        clamp((len10 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0, 1);
    float dis11 =
        clamp((len11 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0, 1);

    const int MIN_TESS_LEVEL = 1;
    const int MAX_TESS_LEVEL = 128;

    float level0 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dis10, dis00));
    float level1 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dis00, dis01));
    float level2 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dis01, dis11));
    float level3 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dis11, dis10));

    gl_TessLevelOuter[0] = level0;
    gl_TessLevelOuter[1] = level1;
    gl_TessLevelOuter[2] = level2;
    gl_TessLevelOuter[3] = level3;

    gl_TessLevelInner[0] = max(level1, level3);
    gl_TessLevelInner[1] = max(level0, level2);
}