#include <Windows.h>
#include "errors.hpp"

void exitOnError(
	const wchar_t* errMsg)
{
	MessageBox(NULL, errMsg, NULL, MB_OK);
	exit(-1);
}

void exitOnNull(
	const void* ptr,
	const wchar_t* errMsg)
{
	if (ptr != NULL)
	{
		return;
	}
	MessageBox(NULL, errMsg, NULL, MB_OK);
	exit(-1);
}

void exitOnVkError(
	VkResult result)
{
	if (result == VK_SUCCESS)
	{
		return;
	}
	MessageBox(NULL, L"vkResult is error", NULL, MB_OK);
	exit(-1);
}