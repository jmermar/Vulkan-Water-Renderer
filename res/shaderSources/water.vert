#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 norm;
layout(location = 1) out vec3 worldPos;

layout(push_constant) uniform constants {
    mat4 projView;
    vec3 camPos;
    uint skyboxTexture;
    float time;
};

const float BASE_A = 0.1;
const float BASE_W = 1;
const vec2 BASE_D = normalize(vec2(1, 1));

const uint NUM_FREQUS = 24;
const float A_MULT = 0.82;
const float W_MULT = 1.2;

const float k = 4;

float H(vec2 D, vec2 pos, float A, float w, float speed) {
    return pow((sin(dot(D, pos) * w + time * speed) + 1) * 0.5, k) * 2 * A;
}

float DHX(vec2 D, vec2 pos, float A, float w, float speed) {
    return k * D.x * w * A 
        * pow((sin(dot(D, pos) * w + time * speed) + 1) * 0.5, k - 1)
        * cos(dot(D, pos) * w + time * speed);
}

float DHY(vec2 D, vec2 pos, float A, float w, float speed) {
    return k * D.y * w * A 
        * pow((sin(dot(D, pos) * w + time * speed) + 1) * 0.5, k - 1)
        * cos(dot(D, pos) * w + time * speed);
}

float WaterHeight(vec3 position, out vec3 normal) {
    float a = BASE_A;
    float w = BASE_W;
    float speed = 2;
    vec2 d = BASE_D;
    vec2 pos2d = vec2(pos.x, pos.z);
    float h = 0;
    normal = vec3(0, 1, 0);
    float rand = 0;
    for(uint i = 0; i < NUM_FREQUS; i++) {
        h += H(d, pos2d, a, w, speed);
        normal.x += -DHX(d, pos2d, a, w, speed);
        normal.z += -DHY(d, pos2d, a, w, speed);


        rand = fract(sin(rand * 1.130812123312 +3.13873) * 4234234.23423023 + rand);

        d = normalize (-d + vec2(-rand, rand) *0.5);

        a *= A_MULT;
        w *= W_MULT;
    }

    normal = normalize(normal);

    return h;
}

void main() {
    vec4 wp = vec4(pos, 1);
    wp.y = WaterHeight(worldPos.xyz, norm);
    
    gl_Position = projView * wp;

    worldPos = wp.xyz;
}