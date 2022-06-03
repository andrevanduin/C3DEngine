
#pragma once
#include "core/defines.h"

#include "material.h"
#include "texture.h"

namespace C3D
{
	// Pre-defined resource types
	enum class ResourceType : u8
	{
		None,
		Text,
		Binary,
		Image,
		Material,
		StaticMesh,
		Shader,
		Custom
	};

	struct Resource
	{
		u32 loaderId;

		const char* name;
		char* fullPath;

		u64 dataSize;
		void* data;

		template<class T>
		T GetData()
		{
			return static_cast<T>(data);
		}
	};

	struct ImageResourceData
	{
		u8 channelCount;
		u32 width, height;
		u8* pixels;
	};

	struct MaterialConfig
	{
		char name[MATERIAL_NAME_MAX_LENGTH];
		MaterialType type;
		bool autoRelease;
		vec4 diffuseColor;
		char diffuseMapName[TEXTURE_NAME_MAX_LENGTH];
	};
}