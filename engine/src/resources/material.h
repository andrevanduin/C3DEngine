
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
		/* @brief The id of the material. */
		u32 id;
		/* @brief The material generation. Gets incremented every time the material is changed. */
		u32 generation;
		/* @brief The material internal id. Used by the renderer backend to map to internal resources. */
		u32 internalId;
		/* @brief The material's associated shader id. */
		u32 shaderId;
		/* @brief The material name. */
		char name[MATERIAL_NAME_MAX_LENGTH];
		/* @brief The material diffuse color. */
		vec4 diffuseColor;
		/* @brief The material diffuse texture map. */
		TextureMap diffuseMap;
		/* @brief The material specular texture map. */
		TextureMap specularMap;
		/* @brief The material normal texture map. */
		TextureMap normalMap;
		/* @brief The material shininess, determines how bright the specular lighting will be. */
		float shininess;

		/* @brief Synced to the renderer current frame number when the material has been applied that frame. */
		u32 renderFrameNumber;
	};	
}