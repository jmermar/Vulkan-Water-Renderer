#version 450
#extension GL_EXT_nonuniform_qualifier : require

#include "postprocess.h"
#include "globalData.h"

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D textures[];

const float density = 0.025;
const float gradient = 1;

const float minHeightFog = 0.5;
const float maxHeightFog = 100;

const vec4 fogColor = vec4(1) * lightStrength;

void main() {
    float rawDepth = texture(textures[depth], uv).r;
    vec4 viewCoords = invProj * vec4(uv.x * 2 - 1, uv.y * 2 -1, rawDepth, 1);
    viewCoords /= viewCoords.w;

    float fogVisibility = clamp(exp(-pow(-viewCoords.z*density, gradient)), 0, 1);
    float fogFactor = pow(clamp((viewCoords.y - minHeightFog) / (maxHeightFog - minHeightFog), 0, 1), 2);

    float visibility = mix(fogVisibility, 1, fogFactor);

    vec4 t = texture(textures[source], uv);
    vec3 hdrColor = mix(fogColor, t, visibility).rgb;

    //Tonemapping

    vec3 mapped = vec3(1) -exp(-hdrColor * 0.1);

    color = vec4(mapped, 1);
}