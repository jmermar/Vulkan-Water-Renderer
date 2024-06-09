#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 ocoords;

layout(push_constant) uniform constants {
    mat4 projView;
    vec3 camPos;
    uint skyboxTexture;
};

void main() {
    gl_Position = projView * vec4(camPos + pos, 1);
    ocoords = pos;
}