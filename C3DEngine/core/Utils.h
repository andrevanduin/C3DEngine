
#pragma once
#include <string>

#include "VkTypes.h"

namespace C3D::Utils
{
	std::string GetVulkanApiVersion(VkPhysicalDeviceProperties properties);

	std::string GetGpuDriverVersion(VkPhysicalDeviceProperties properties);
}