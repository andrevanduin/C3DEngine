
#pragma once
#include <unordered_map>

#include "AssetLoader.h"

namespace Assets
{
	enum class TransparencyMode : uint8_t
	{
		Opaque,
		Transparent,
		Masked
	};

	struct MaterialInfo
	{
		std::string baseEffect;
		std::unordered_map<std::string, std::string> textures;
		std::unordered_map<std::string, std::string> customProperties;
		TransparencyMode transparency;
	};

	MaterialInfo ReadMaterialInfo(AssetFile* file);

	AssetFile PackMaterial(MaterialInfo* info);
}