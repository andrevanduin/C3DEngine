
#include "image_loader.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/c3d_string.h"

#include "resources/resource_types.h"

#include "services/services.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace C3D
{
	ImageLoader::ImageLoader()
		: ResourceLoader("IMAGE_LOADER", MemoryType::Texture, ResourceType::Image, nullptr, "textures")
	{}

	bool ImageLoader::Load(const string& name, Resource* outResource)
	{
		if (name.empty() || !outResource) return false;

		constexpr i32 requiredChannelCount = 4;
		stbi_set_flip_vertically_on_load(true);

		char fullPath[512];
		const auto formatStr = "%s/%s/%s.%s";

		// TODO: try different extensions
		StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name.data(), "png");

		i32 width;
		i32 height;
		i32 channelCount;

		// For now, assume 8 bits per channel and 4 channels
		// TODO: extend this to make it configurable
		u8* data = stbi_load(fullPath, &width, &height, &channelCount, requiredChannelCount);

		if (auto failReason = stbi_failure_reason())
		{
			m_logger.Error("Failed to load file '{}': {}", fullPath, failReason);
			// Clear the error so the next load does not fail
			stbi__err(nullptr, 0);

			// Free the image data if we have any
			if (data) stbi_image_free(data);

			return false;
		}

		if (!data)
		{
			m_logger.Error("Failed to load file '{}': {}", fullPath);
			return false;
		}

		outResource->fullPath = StringDuplicate(fullPath);

		auto* resourceData = Memory.Allocate<ImageResourceData>(MemoryType::Texture);
		resourceData->pixels = data;
		resourceData->width = width;
		resourceData->height = height;
		resourceData->channelCount = requiredChannelCount;

		outResource->data = resourceData;
		outResource->dataSize = sizeof(ImageResourceData);
		outResource->name = name.data();

		return true;
	}
}
