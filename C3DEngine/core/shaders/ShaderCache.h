
#pragma once
#include "../VkTypes.h"
#include "ShaderModule.h"

#include <string>
#include <unordered_map>

namespace C3D
{
	class ShaderCache
	{
	public:
		void Init(VkDevice device);

		ShaderModule* GetShader(const std::string& path);

	private:
		VkDevice m_device{ VK_NULL_HANDLE };
		std::unordered_map<std::string, ShaderModule> m_cache;
	};
}