
#pragma once
#include "../VkTypes.h"

#include <array>

namespace C3D
{
	class ShaderEffect;

	struct ShaderPass
	{
		ShaderEffect* effect{ nullptr };

		VkPipeline pipeline{ VK_NULL_HANDLE };
		VkPipelineLayout layout{ VK_NULL_HANDLE };

		bool operator==(const ShaderPass& other) const
		{
			return other.effect == effect && other.pipeline == pipeline && other.layout == layout;
		}
	};

	struct ShaderParameters
	{

	};

	template<typename T>
	struct PerPassData
	{
		T& operator[](const MeshPassType pass)
		{
			switch (pass)
			{
			case MeshPassType::Forward:
				return data[0];
			case MeshPassType::Transparency:
				return data[1];
			case MeshPassType::DirectionalShadow:
				return data[2];
			default:
				Logger::Error("Unknown MeshPassType provided: {}", static_cast<uint8_t>(pass));
				return data[0];
			}
		}

		void Clear(T&& val)
		{
			for (int i = 0; i < 3; i++) data[i] = val;
		}
	private:
		std::array<T, 3> data;
	};
}

