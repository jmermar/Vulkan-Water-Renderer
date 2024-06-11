#version 450
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
    float reflectionFactor = fresnelSchlick90(
        clamp(dot(camDir, norm), 0, 1)
        ,0.1, 0.95);

    reflected *= reflectionFactor;
    vec3 waterColor = vec3(0, 0.4, 0.6);


    float specular = pow(max(dot(halfway, norm), 0.0), shininess) * reflectionFactor * lightStrength;
    float diffuse = clamp(dot(-lightDir, norm), 0.4, 1) * lightStrength;
    
    vec3 diffuseColor = waterColor * (1-reflectionFactor);
    color = vec4(reflected + diffuseColor * diffuse + vec3(1) * specular, 1);
}