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
} global;

layout(binding = 1) uniform  ObjectTransform
{
    mat4 model;
} objectTransform;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;


void main() 
{
    vec4 worldPos = objectTransform.model * vec4(inPosition, 1.0);
    gl_Position = global.proj * global.view * worldPos;
}