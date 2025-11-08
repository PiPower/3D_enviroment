#version 450

#define GAMMA_F 2.2f

layout(location = 0) in vec3 faceNormal;
layout(location = 1) in vec2 texCoord; 
layout(location = 2) in vec4 worldPos;


layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(0.9f, 0.3f, 0.3f, 1.0f);
    outColor.rgb = pow(outColor.rgb, vec3(1.0/GAMMA_F));

}