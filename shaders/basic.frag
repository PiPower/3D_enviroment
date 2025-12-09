#version 450
#ifndef LIGHT_COUNT
    #define LIGHT_COUNT
#endif

#define GAMMA_F 2.2f

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
    vec4 color;
} objectTransform;

layout(location = 0) in vec3 faceNormal;
layout(location = 1) in vec2 texCoord; 
layout(location = 2) in vec4 worldPos;


layout(location = 0) out vec4 outColor;
void main()
{
    outColor = objectTransform.color;
    outColor.xyz = outColor.xyz * global.lights[0].color.xyz;
    outColor.rgb = pow(outColor.rgb, vec3(1.0/GAMMA_F));

}