#include "bindUtils.h"

SSB(material, {
    vec4 diffuseColor;
    vec2 baseD;


    float baseA;
    float baseW;

    uint numFreqs;
    float aMult;
    float wMult;

    float speed;

    float baseReflectivity;

    float roughness;
});

layout (push_constant) uniform constants {
    mat4 projView;
    mat4 view;
    vec3 camPos;
    uint skyboxTexture;
    uint materialBind;
    float time;
};

const float k = 7;

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
    float a = GET(material).baseA;
    float w = GET(material).baseW;
    vec2 d = GET(material).baseD;
    vec2 pos2d = vec2(position.x, position.z);
    float h = 0;
    normal = vec3(0, 1, 0);
    float rand = 0;
    vec2 prevDerivative = vec2(0);
    for(uint i = 0; i < GET(material).numFreqs; i++) {
        h += H(d, pos2d, a, w, GET(material).speed);
        prevDerivative.x = DHX(d, pos2d, a, w, GET(material).speed);
        prevDerivative.y = DHY(d, pos2d, a, w, GET(material).speed);


        normal.x -= prevDerivative.x;
        normal.z -= prevDerivative.y;

        rand = fract(sin(rand * 1.130812123312 +3.13873) * 4234234.23423023 + rand);

        d = normalize (-d + vec2(-rand, rand) *0.5);

        a *= GET(material).aMult;
        w *= GET(material).wMult;
    }

    normal = normalize(normal);

    return h;
}

vec3 WaterNormal(vec3 position) {
    float a = GET(material).baseA;
    float w = GET(material).baseW;
    vec2 d = GET(material).baseD;
    vec2 pos2d = vec2(position.x, position.z);
    vec3 normal = vec3(0, 1, 0);
    float rand = 0;
    vec2 prevDerivative = vec2(0);
    for(uint i = 0; i < GET(material).numFreqs; i++) {
        prevDerivative.x = DHX(d, pos2d, a, w, GET(material).speed);
        prevDerivative.y = DHY(d, pos2d, a, w, GET(material).speed);

        normal.x -= prevDerivative.x;
        normal.z -= prevDerivative.y;

        rand = fract(sin(rand * 1.130812123312 +3.13873) * 4234234.23423023 + rand);

        d = normalize (-d + vec2(-rand, rand) *0.5);

        a *= GET(material).aMult;
        w *= GET(material).wMult;
    }

    return normal = normalize(normal);
}