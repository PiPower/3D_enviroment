#include <vulkan/vulkan.h>

#define EXIT_ON_VK_ERROR(result) exitOnVkError(result)
#define EXIT_ON_NULL(ptr) exitOnNull(ptr, NULL);
#define EXIT_ON_NULL(ptr, text) exitOnNull(ptr, text);


void exitOnError(
	const wchar_t* errMsg);

void exitOnNull(
	const void* ptr,
	const wchar_t* errMsg);

void exitOnVkError(
	VkResult result);