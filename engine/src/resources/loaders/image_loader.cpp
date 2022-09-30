
#include "image_loader.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/c3d_string.h"
#include "platform/filesystem.h"

#include "resources/resource_types.h"

#include "services/services.h"
#include "systems/resource_system.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace C3D
{
	ResourceLoader<ImageResource>::ResourceLoader()
		: IResourceLoader("IMAGE_LOADER", MemoryType::Texture, ResourceType::Image, nullptr, "textures")
	{}

	bool ResourceLoader<ImageResource>::Load(const char* name, ImageResource* outResource) const
	{
		if (StringLength(name) == 0 || !outResource) return false;

		constexpr i32 requiredChannelCount = 4;
		stbi_set_flip_vertically_on_load(true);

		char fullPath[512];

		// Try different extensions
		const char* extensions[IMAGE_LOADER_EXTENSION_COUNT] = { "tga", "png", "jpg", "bmp" };
		bool found = false;

		for (const auto extension : extensions)
		{
			const auto formatStr = "%s/%s/%s.%s";
			StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name, extension);
			// Check if the requested file exists with the current extension
			if (File::Exists(fullPath))
			{
				// It exists so we break out of the loop
				found = true;
				break;
			}
		}

		if (!found)
		{
			m_logger.Error("Load() - Failed to find file '{}' with any supported extension", name);
			return false;
		}

		i32 width;
		i32 height;
		i32 channelCount;

		// For now, assume 8 bits per channel and 4 channels
		// TODO: extend this to make it configurable
		u8* data = stbi_load(fullPath, &width, &height, &channelCount, requiredChannelCount);

		/*
		if (auto failReason = stbi_failure_reason())
		{
			m_logger.Error("Failed to load file '{}': {}", fullPath, failReason);
			// Clear the error so the next load does not fail
			stbi__err(nullptr, 0);

			// Free the image data if we have any
			if (data) stbi_image_free(data);

			return false;
		}*/

		if (!data)
		{
			m_logger.Error("Failed to load file '{}': {}", fullPath);
			return false;
		}

		outResource->fullPath = StringDuplicate(fullPath);
		outResource->data.pixels = data;
		outResource->data.width = width;
		outResource->data.height = height;
		outResource->data.channelCount = requiredChannelCount;

		return true;
	}

	void ResourceLoader<ImageResource>::Unload(const ImageResource* resource)
	{
		// Free the pixel data loaded in by STBI
		stbi_image_free(resource->data.pixels);
	}
}
