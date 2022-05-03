
#include <nlohmann/json.hpp>

#include "MaterialAsset.h"

Assets::MaterialInfo Assets::ReadMaterialInfo(AssetFile* file)
{
	MaterialInfo info;

	auto materialMetadata = nlohmann::json::parse(file->json);
	info.baseEffect = materialMetadata["baseEffect"];

	for (auto& [key, value] : materialMetadata["textures"].items())
	{
		info.textures[key] = value;
	}

	for (auto& [key, value] : materialMetadata["customProperties"].items())
	{
		info.customProperties[key] = value;
	}

	info.transparency = TransparencyMode::Opaque;

	if (const auto it = materialMetadata.find("transparency"); it != materialMetadata.end())
	{
		const auto &val = *it;
		if (val == "transparent")
		{
			info.transparency = TransparencyMode::Transparent;
		}
		else if (val == "masked")
		{
			info.transparency = TransparencyMode::Masked;
		}
		else if (val == "opaque")
		{
			info.transparency = TransparencyMode::Opaque;
		}
	}

	return info;
}

Assets::AssetFile Assets::PackMaterial(MaterialInfo* info)
{
	nlohmann::json materialData;
	materialData["baseEffect"] = info->baseEffect;
	materialData["textures"] = info->textures;
	materialData["customProperties"] = info->customProperties;

	switch (info->transparency)
	{
		case TransparencyMode::Transparent:
			materialData["transparency"] = "transparent";
			break;
		case TransparencyMode::Masked:
			materialData["transparency"] = "masked";
			break;
		case TransparencyMode::Opaque:
			materialData["transparency"] = "opaque";
			break;
	}

	AssetFile file;
	file.type[0] = 'M';
	file.type[1] = 'A';
	file.type[2] = 'T';
	file.type[3] = 'X';

	file.json = materialData.dump();
	return file;
}
