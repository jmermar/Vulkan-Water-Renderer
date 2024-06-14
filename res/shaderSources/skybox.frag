#version 450
#extension GL_EXT_nonuniform_qualifier : require
#include "globalData.h"

layout(location = 0) in vec3 coords;

layout(location = 0) out vec4 color;

layout(push_constant) uniform constants {
    mat4 projView;
    vec3 camPos;
    uint skyboxTexture;
};

layout(binding = 0) uniform samplerCube textures[];

void main() { 
    vec3 sun = vec3(1) * pow(clamp(dot(normalize(coords), -lightDir), 0, 1), 500);
    color = texture(textures[skyboxTexture], coords) * lightStrength + vec4(sun, 0) * 10;
}