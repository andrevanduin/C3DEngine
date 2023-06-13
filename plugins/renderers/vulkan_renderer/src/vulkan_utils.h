
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanUtils
{
	bool IsSuccess(VkResult result);

	const char* ResultString(VkResult result, bool getExtended = true);

	VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
}