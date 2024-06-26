#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "pbr.h"
#include "globalData.h"
#include "water.h"

layout (location = 0) in vec3 vNorm;
layout (location = 1) in vec3 worldPos;

layout (location = 0) out vec4 color;

layout(binding = 0) uniform samplerCube textures[];

const float shininess = 50.0;

void main() { 
    vec3 norm = WaterNormal(worldPos);
    vec3 camDir = normalize(camPos - worldPos);
    vec3 halfway = normalize(-lightDir + camDir);

    vec3 reflected = texture(textures[skyboxTexture], reflect(-camDir, norm)).xyz * lightStrength;

    vec3 waterColor = GET(material).diffuseColor.rgb;


    vec3 colorLight = brdfMicrofacet(-lightDir, camDir, norm, 0, GET(material).roughness, waterColor, GET(material).baseReflectivity);
    vec3 colorAmbient = brdfAmbient(-lightDir, camDir, norm, 0, GET(material).roughness, waterColor, reflected, GET(material).baseReflectivity);

    color = vec4(colorLight + colorAmbient, 1);
}