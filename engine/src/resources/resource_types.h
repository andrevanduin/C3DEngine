
#pragma once
#include "core/defines.h"

#include "material.h"
#include "texture.h"

#include "services/services.h"
#include "core/memory.h"

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
		Mesh,
		Shader,
		Custom,
		MaxValue
	};

	struct Resource
	{
		Resource()
			: loaderId(INVALID_ID), name(nullptr), fullPath(nullptr)
		{}

		Resource(const Resource& other) noexcept
			: loaderId(other.loaderId), name(nullptr), fullPath(nullptr)
		{
			if (other.name) name = StringDuplicate(other.name);
			if (other.fullPath) fullPath = StringDuplicate(other.fullPath);
		}

		Resource(Resource&& other) noexcept
			: loaderId(other.loaderId), name(other.name), fullPath(other.fullPath)
		{
			other.loaderId = INVALID_ID;
			other.name = nullptr;
			other.fullPath = nullptr;
		}

		Resource& operator=(const Resource& other)
		{
			if (this != &other)
			{
				loaderId = other.loaderId;
				if (other.name) name = StringDuplicate(other.name);
				if (other.fullPath) fullPath = StringDuplicate(other.fullPath);
			}
			return *this;
		}

		Resource& operator=(Resource&& other) noexcept
		{
			std::swap(loaderId, other.loaderId);
			std::swap(name, other.name);
			std::swap(fullPath, other.fullPath);
			return *this;
		}

		~Resource()
		{
			if (name)
			{
				const auto size = StringLength(name) + 1;
				Memory.Free(name, size, MemoryType::String);
				name = nullptr;
			}
			if (fullPath)
			{
				const auto size = StringLength(fullPath) + 1;
				Memory.Free(fullPath, size, MemoryType::String);
				fullPath = nullptr;
			}
		}

		u32 loaderId;

		char* name;
		char* fullPath;
	}; 

	struct ImageResourceData
	{
		u8 channelCount;
		u32 width, height;
		u8* pixels;
	};

	struct MaterialConfig
	{
		/* @brief Name of the Material. */
		char name[MATERIAL_NAME_MAX_LENGTH];
		/* @brief Name of the shader associated with this material. */
		char* shaderName;
		/* @brief Indicates if this material should automatically be release when no references to it remain. */
		bool autoRelease;
		/* @brief The diffuse color of the material. */
		vec4 diffuseColor;
		/* @brief How shiny this material is. */
		float shininess;
		/* @brief The diffuse map name. */
		char diffuseMapName[TEXTURE_NAME_MAX_LENGTH];
		/* @brief The specular map name. */
		char specularMapName[TEXTURE_NAME_MAX_LENGTH];
		/* @brief The normal map name. */
		char normalMapName[TEXTURE_NAME_MAX_LENGTH];
	};
}
