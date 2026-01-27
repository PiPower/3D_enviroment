#version 450                                                                        
                                                                                                
layout(binding = 1) uniform samplerCube gCubemapTexture;                                                
                     
layout(location = 0) in vec3 dirVec;  

layout(location = 0)  out vec4 FragColor;   
void main()                                                                         
{                                                                                   
    FragColor = texture(gCubemapTexture, dirVec);      
    FragColor = vec4(0, 0 ,0, 1);
}