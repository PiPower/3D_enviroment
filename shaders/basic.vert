#version 450
#ifndef LIGHT_COUNT
    #define LIGHT_COUNT
#endif


struct Light
{
    vec4 color; // (R, G, B, Intensity)
    vec4 pos;   // (x, y, z, unused)
};

layout(binding = 0) uniform Globals
{
    mat4 view;
    mat4 proj;
    Light lights[LIGHT_COUNT]; // (R, G, B, Intensity);
    mat4 lightViewProj[LIGHT_COUNT];
} global;

layout(binding = 1) uniform  ObjectTransform
{
    mat4 model;
} objectTransform;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;


layout(location = 0) out vec3 faceNormal;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec4 worldPos;
layout(location = 3) out vec4 worldPosLightCoord;

void main() 
{
    worldPos = objectTransform.model * vec4(inPosition, 1.0);
    worldPosLightCoord = global.lightViewProj[0] * worldPos;
    faceNormal =  transpose(inverse(mat3(objectTransform.model))) * inNormal;
    texCoord = inTex;

    gl_Position = global.proj * global.view * worldPos;
}