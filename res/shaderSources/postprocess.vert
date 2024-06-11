#version 450
#extension GL_EXT_nonuniform_qualifier : require

#include "postprocess.h"

layout (location = 0) out vec2 uv;

const vec2 vertices[6] = {
    vec2(-1, -1),
    vec2(1, -1),
    vec2(1, 1),

    vec2(-1, -1),
    vec2(1, 1),
    vec2(-1, 1),
};

void main() {
    vec2 vertex = vertices[gl_VertexIndex];
    
    gl_Position = vec4(vertex.x, vertex.y, 0, 1);

    uv = vertex * 0.5 + vec2(0.5);
}