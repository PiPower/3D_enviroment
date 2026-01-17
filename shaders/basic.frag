#version 450
#ifndef LIGHT_COUNT
    #define LIGHT_COUNT
#endif

#ifndef TEXTURE_COUNT
    #define TEXTURE_COUNT
#endif

#define GAMMA_F 2.2f

struct Light
{
    vec4 color; // (R, G, B, Intensity)
    vec4 pos;   // (x, y, z, Ambient factor)
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

layout(binding = 2) uniform sampler2D smp[TEXTURE_COUNT];

layout(location = 0) in vec3 faceNormal;
layout(location = 1) in vec2 texCoord; 
layout(location = 2) in vec4 worldPos;


layout(location = 0) out vec4 outColor;
void main()
{
    vec4 color = texture(smp[0], texCoord);

    vec3 lightVec = normalize(global.lights[0].pos.xyz - worldPos.xyz);
    float diffCoeff = max(dot(normalize(faceNormal), lightVec), 0.0);
    vec3 diffuseLight = diffCoeff * global.lights[0].color.rgb;
    vec3 ambientLight = global.lights[0].pos.w * global.lights[0].color.rgb;

    outColor.rgb = (diffuseLight + ambientLight) * color.rgb;
    outColor.rgb = pow(color.rgb, vec3(1.0/GAMMA_F));
}