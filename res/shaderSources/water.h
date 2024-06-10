const float BASE_A = 0.1;
const float BASE_W = 1;
const vec2 BASE_D = normalize(vec2(1, 0));

const uint NUM_FREQUS = 50;
const float A_MULT = 0.8;
const float W_MULT = 1.2;

layout (push_constant) uniform constants {
    mat4 projView;
    mat4 view;
    vec3 camPos;
    uint skyboxTexture;
    float time;
};

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
    vec2 pos2d = vec2(position.x, position.z);
    float h = 0;
    normal = vec3(0, 1, 0);
    float rand = 0;
    for(uint i = 0; i < NUM_FREQUS; i++) {
        h += H(d, pos2d, a, w, speed);
        normal.x -= DHX(d, pos2d, a, w, speed);
        normal.z -= DHY(d, pos2d, a, w, speed);


        rand = fract(sin(rand * 1.130812123312 +3.13873) * 4234234.23423023 + rand);

        d = normalize (-d + vec2(-rand, rand) *0.5);

        a *= A_MULT;
        w *= W_MULT;
    }

    normal = normalize(normal);

    return h;
}

vec3 WaterNormal(vec3 position) {
    float a = BASE_A;
    float w = BASE_W;
    float speed = 2;
    vec2 d = BASE_D;
    vec2 pos2d = vec2(position.x, position.z);
    vec3 normal = vec3(0, 1, 0);
    float rand = 0;
    for(uint i = 0; i < NUM_FREQUS; i++) {
        normal.x -= DHX(d, pos2d, a, w, speed);
        normal.z -= DHY(d, pos2d, a, w, speed);


        rand = fract(sin(rand * 1.130812123312 +3.13873) * 4234234.23423023 + rand);

        d = normalize (-d + vec2(-rand, rand) *0.5);

        a *= A_MULT;
        w *= W_MULT;
    }

    return normal = normalize(normal);
}