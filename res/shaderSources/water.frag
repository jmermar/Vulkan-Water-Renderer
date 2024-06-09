#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 norm;
layout (location = 1) in vec3 worldPos;

layout (location = 0) out vec4 color;

layout (push_constant) uniform constants {
    mat4 projView;
    vec3 camPos;
    uint skyboxTexture;
    float time;
};

layout(binding = 0) uniform samplerCube textures[];

const float shininess = 50.0;

const vec3 lightDir = normalize(vec3(0, -1, -10));
const vec3 waterColor = vec3(0.2, 0.3, 0.8);
void main() { 
    vec3 camDir = normalize(camPos - worldPos);
    vec3 halfway = normalize(-lightDir + camDir);

    vec3 reflected = texture(textures[skyboxTexture], reflect(-camDir, norm)).xyz;
    float reflectionFactor = clamp(pow(1 - dot(camDir, norm), 5), 0, 1);
    reflected *= reflectionFactor;
    vec3 waterColor = vec3(0.4, 0.5, 1);


    float specular = pow(max(dot(halfway, norm), 0.0), shininess);
    float diffuse = clamp(dot(-lightDir, norm), 0.5, 1);
    
    vec3 diffuseColor = waterColor * (1-reflectionFactor);
    color = vec4(reflected + diffuseColor * diffuse + vec3(1) * specular, 1);
}