
#pragma once

namespace C3D
{
	struct VkObjects
	{
		VkInstance instance;
		VkSurfaceKHR surface;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
	};
}