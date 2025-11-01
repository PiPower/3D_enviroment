#include "ShaderCompiler.hpp"
#include "errors.hpp"
#include <fstream>

using namespace std;
static vector<char> readFile(const char* filename)
{
    using namespace std;
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open())
    {
        MessageBox(NULL, L"Failed to open file", NULL, MB_OK);
        exit(-1);
    }

    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize + 1);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    buffer[fileSize] = '\0';
    return buffer;

}

ShaderCompiler::ShaderCompiler()
{
	EXIT_ON_NULL(compiler = shaderc_compiler_initialize(), L"Compiler initialization failed\n");
}

VkShaderModule ShaderCompiler::CompileShaderFromPath(
	VkDevice device, 
	VkAllocationCallbacks* callbacks,
	const char* path,
	const char* entryName,
	shaderc_shader_kind shaderKind,
	const ShaderOptions& options)
{
    vector<char> shaderBuffer = readFile(path);
    size_t fileNameOffset = strlen(path) - 1;
    const char* fileName = path + fileNameOffset;

    while (path != fileName && *fileName != '\\' && *fileName != '/')
    {
        fileName--;
    }
    if (*fileName == '\\' || *fileName == '/') { fileName++; }

	return CompileShader(device, callbacks, shaderBuffer.data(), shaderBuffer.size() - 1, entryName, fileName, shaderKind, options);
}

VkShaderModule ShaderCompiler::CompileShader(
    VkDevice device,
    VkAllocationCallbacks* callbacks,
    const char* shaderCode, 
    size_t shaderCodeSize,
    const char* entryName,
    const char* fileName,
    shaderc_shader_kind shaderKind,
    const ShaderOptions& options)
{
    shaderc_compile_options_t shaderOptions = nullptr;
    if (options.name.size() > 0)
    {
        shaderOptions = shaderc_compile_options_initialize();
        for (size_t i = 0; i < options.name.size(); i++)
        {
            const char* name = options.name[i].c_str();
            size_t nameLen = options.name[i].length() - 1;
            const char* value = options.value[i].length() == 0 ? nullptr : options.value[i].c_str();
            size_t valueLen = options.value[i].length() == 0 ? 0 : options.value[i].length();

            shaderc_compile_options_add_macro_definition(shaderOptions, name, nameLen, value, valueLen);
        }

    }

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        compiler, shaderCode, shaderCodeSize,
        shaderKind, fileName, entryName, shaderOptions);

    shaderc_compilation_status errCount = shaderc_result_get_compilation_status(result);
    if (errCount != shaderc_compilation_status_success)
    {
        const char* errMsg = shaderc_result_get_error_message(result);
        MessageBoxA(NULL, errMsg, NULL, MB_OK);
        exit(-1);
    }

    VkShaderModule compiledModule;
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = shaderc_result_get_length(result);
    moduleInfo.pCode = (uint32_t*)shaderc_result_get_bytes(result);
    EXIT_ON_VK_ERROR(vkCreateShaderModule(device, &moduleInfo, callbacks, &compiledModule));

    shaderc_result_release(result);
    shaderc_compile_options_release(shaderOptions);

    return compiledModule;
}
