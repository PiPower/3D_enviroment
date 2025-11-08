#version 450


layout(binding = 0) uniform CameraUbo
{
    mat4 view;
    mat4 proj;
} cameraTransform;

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

void main() 
{
    worldPos = objectTransform.model * vec4(inPosition, 1.0);
    gl_Position = cameraTransform.proj * cameraTransform.view * worldPos;
    faceNormal =  transpose(inverse(mat3(objectTransform.model))) * inNormal;
    texCoord = inTex;
}