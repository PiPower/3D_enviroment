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
    mat4 lightView[LIGHT_COUNT];
} global;


layout(binding = 1) uniform  ObjectTransform
{
    mat4 model;
    uvec4 objInfo; //(x - textureIdx, y - unused, z - unused, w - unused)
} objectTransform;

layout(binding = 2) uniform sampler2D smp[TEXTURE_COUNT];
layout(binding = 3) uniform sampler2D shadowmap;


layout(location = 0) in vec3 faceNormal;
layout(location = 1) in vec2 texCoord; 
layout(location = 2) in vec4 worldPos;
layout(location = 3) in vec4 worldPosLightCoord;


layout(location = 0) out vec4 outColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.xy = clamp(projCoords.xy, 0.0f, 1.0f);
    float closestDepth = texture(shadowmap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    if(projCoords.z > 1.0)
    {
        return  0.0;
    }

    float bias = 0.01;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowmap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowmap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    vec3 lightVec = normalize(global.lights[0].pos.xyz - worldPos.xyz);
    float diffCoeff = max(dot(normalize(faceNormal), lightVec), 0.0);
    vec3 diffuseLight = diffCoeff * global.lights[0].color.rgb;
    vec3 ambientLight = global.lights[0].pos.w * global.lights[0].color.rgb;
    float shadow = ShadowCalculation(worldPosLightCoord);  

    outColor = texture(smp[objectTransform.objInfo.x], texCoord);
    outColor.rgb = ((1.0f - shadow) * diffuseLight + ambientLight) * outColor.rgb;
    outColor.rgb = pow(outColor.rgb, vec3(1.0/GAMMA_F));

}