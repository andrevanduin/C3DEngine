
#pragma once
#include "math/math_types.h"
#include "texture.h"

namespace C3D
{
	constexpr auto MATERIAL_NAME_MAX_LENGTH = 256;

	enum class MaterialType
	{
		World,
		Ui
	};

	struct Material
	{
		u32 id, generation, internalId;
		MaterialType type;
		char name[MATERIAL_NAME_MAX_LENGTH];
		vec4 diffuseColor;
		TextureMap diffuseMap;
	};	
}