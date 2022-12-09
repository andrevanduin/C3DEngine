
#pragma once
#include "core/defines.h"
#include "containers/string.h"

namespace C3D
{
	constexpr auto BINARY_RESOURCE_FILE_MAGIC_NUMBER = 0xC3DC3D;

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
		BitmapFont,
		SystemFont,
		Custom,
		MaxValue
	};

	/* @brief The header for our proprietary resource files. */
	struct ResourceHeader
	{
		/* @brief A magic number indicating this file is a C3D binary file. */
		u32 magicNumber;
		/* @brief The type of this resource, maps to our ResourceType enum. */
		u8 resourceType;
		/* @brief The format version the resource file uses. */
		u8 version;
		/* @brief Some reserved space for future header data. */
		u16 reserved;
	};

	struct Resource
	{
		Resource()
			: loaderId(INVALID_ID)
		{}

		Resource(const Resource& other) noexcept = default;

		Resource(Resource&& other) noexcept
			: loaderId(other.loaderId), name(std::move(other.name)), fullPath(std::move(other.fullPath))
		{
			other.loaderId = INVALID_ID;
		}

		Resource& operator=(const Resource& other)
		{
			if (this != &other)
			{
				loaderId = other.loaderId;
				if (other.name) name = other.name;
				if (other.fullPath) fullPath = other.fullPath;
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

		virtual ~Resource()
		{
			name.Destroy();
			fullPath.Destroy();
		}

		u32 loaderId;

		String name;
		String fullPath;
	};
}
