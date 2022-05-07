
#include "Utils.h"

namespace C3D
{
	std::string Utils::GetVulkanApiVersion(const VkPhysicalDeviceProperties properties)
	{
		const auto majorVersion = VK_API_VERSION_MAJOR(properties.apiVersion);
		const auto minorVersion = VK_API_VERSION_MINOR(properties.apiVersion);
		const auto patchVersion = VK_API_VERSION_PATCH(properties.apiVersion);
		return std::to_string(majorVersion) + "." + std::to_string(minorVersion) + "." + std::to_string(patchVersion);
	}

	std::string Utils::GetGpuDriverVersion(VkPhysicalDeviceProperties properties)
	{
		const auto vendorId = properties.vendorID;
		const auto version = properties.driverVersion;

		if (vendorId == NVIDIA_VENDOR_ID)
		{
			const auto major = version >> 22 & 0x3ff;
			const auto mid = version >> 14 & 0x0ff;
			const auto minor = version >> 6 & 0x0ff;
			const auto patch = version & 0x003f;
			return std::to_string(major) + "." + std::to_string(mid) + "." + std::to_string(minor) + "." + std::to_string(patch);
		}

		#ifdef _WIN32
		if (vendorId == INTEL_VENDOR_ID)
		{
			const auto major = version >> 14;
			const auto minor = version & 0x3fff;

			return std::to_string(major) + "." + std::to_string(minor);
		}
		#endif

		const auto major = version >> 22;
		const auto minor = version >> 12 & 0x3ff;
		const auto patch = version & 0xfff;

		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
	}
}


