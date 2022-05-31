
#pragma once
#include "core/defines.h"

#include "material.h"
#include "geometry.h"
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
		Custom
	};

	struct Resource
	{
		u32 loaderId;

		const char* name;
		char* fullPath;

		u64 dataSize;
		void* data;
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
		bool autoRelease;
		vec4 diffuseColor;
		char diffuseMapName[TEXTURE_NAME_MAX_LENGTH];
	};
}