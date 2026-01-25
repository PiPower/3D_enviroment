#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(binding = 0) uniform Globals
{
    mat4 view;
    mat4 proj;
} global;

const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),		// 0
	vec3( 1.0,-1.0, 1.0),		// 1
	vec3( 1.0, 1.0, 1.0),		// 2
	vec3(-1.0, 1.0, 1.0),		// 3

	vec3(-1.0,-1.0,-1.0),		// 4
	vec3( 1.0,-1.0,-1.0),		// 5
	vec3( 1.0, 1.0,-1.0),		// 6
	vec3(-1.0, 1.0,-1.0)		// 7
);

const int indices[36] = int[36](
	1, 0, 2, 3, 2, 0,	// front
	5, 1, 6, 2, 6, 1,	// right 
	6, 7, 5, 4, 5, 7,	// back
	0, 4, 3, 7, 3, 4,	// left
	5, 4, 1, 0, 1, 4,	// bottom
	2, 3, 6, 7, 6, 3	// top
);

layout(location = 0) out vec3 dirVec;
void main() 
{

	int idx = indices[gl_VertexIndex];
	vec3 inPosition = pos[idx];
    dirVec = inPosition;

    mat4 viewMatrix = mat4(mat3(global.view)); // remove translation part
    vec4 ndcPos = global.proj * viewMatrix * vec4(inPosition, 1.0);
    gl_Position = ndcPos.xyww;
}