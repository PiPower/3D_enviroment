#pragma once
#include <Windows.h>
#include <shaderc/shaderc.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <string>
struct ShaderOptions
{
	std::vector<std::string> name;
	std::vector<std::string> value;
};

class ShaderCompiler
{
public:
	ShaderCompiler();
	VkShaderModule CompileShaderFromPath(
		VkDevice device, 
		VkAllocationCallbacks* callbacks,
		const char* path, 
		const char* entryName,
		shaderc_shader_kind shaderKind, 
		const ShaderOptions& options);

	VkShaderModule CompileShader(
		VkDevice device,
		VkAllocationCallbacks* callbacks,
		const char* shaderCode,
		size_t shaderCodeSize,
		const char* entryName,
		const char* fileName,
		shaderc_shader_kind shaderKind,
		const ShaderOptions& options);
public:
	shaderc_compiler_t compiler;
};

