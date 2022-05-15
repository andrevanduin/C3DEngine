
#pragma once
#include "../VkTypes.h"
#include "../shaders/Shader.h"
#include "MaterialAsset.h"

#include <vector>

namespace C3D
{
	struct SampledTexture
	{
		VkSampler sampler;
		VkImageView view;
	};

	struct MaterialData
	{
		std::vector<SampledTexture> textures;
		ShaderParameters* parameters;
		std::string baseTemplate;

		bool operator==(const MaterialData& other) const;

		[[nodiscard]] size_t Hash() const;
	};

	struct EffectTemplate
	{
		PerPassData<ShaderPass*> passShaders;

		ShaderParameters* defaultParameters;
		Assets::TransparencyMode transparency;
	};

	struct Material
	{
		EffectTemplate* original;
		PerPassData<VkDescriptorSet> passSets;

		std::vector<SampledTexture> textures;

		ShaderParameters* parameters;

		Material& operator=(const Material& other) = default;
	};
}

