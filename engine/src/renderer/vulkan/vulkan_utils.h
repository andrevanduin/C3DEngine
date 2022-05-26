
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanUtils
{
	bool ResultIsSuccess(VkResult result);

	const char* ResultString(VkResult result, bool getExtended);
}