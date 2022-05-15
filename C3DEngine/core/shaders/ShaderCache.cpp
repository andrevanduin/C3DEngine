
#include "ShaderCache.h"
#include "../Logger.h"

namespace C3D
{
	void ShaderCache::Init(VkDevice device)
	{
		m_device = device;
	}

	ShaderModule* ShaderCache::GetShader(const std::string& path)
	{
		if (m_cache.find(path) == m_cache.end())
		{
			ShaderModule shader;
			if (!LoadShaderModule(m_device, path, &shader))
			{
				Logger::Error("Failed to compile Shader {}", path);
				return nullptr;
			}

			m_cache[path] = shader;
		}

		return &m_cache[path];
	}
}
