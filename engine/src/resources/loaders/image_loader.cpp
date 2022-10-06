
#include "image_loader.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/c3d_string.h"
#include "platform/filesystem.h"

#include "resources/resource_types.h"

#include "services/services.h"
#include "systems/resource_system.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

namespace C3D
{
	ResourceLoader<ImageResource>::ResourceLoader()
		: IResourceLoader("IMAGE_LOADER", MemoryType::Texture, ResourceType::Image, nullptr, "textures")
	{}

	bool ResourceLoader<ImageResource>::Load(const char* name, ImageResource* outResource) const
	{
		return Load(name, outResource, {});
	}

	bool ResourceLoader<ImageResource>::Load(const char* name, ImageResource* outResource, const ImageResourceParams& params) const
	{
		if (StringLength(name) == 0 || !outResource) return false;

		constexpr i32 requiredChannelCount = 4;
		stbi_set_flip_vertically_on_load_thread(params.flipY);

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

		// Take a copy of the resource path and name
		outResource->fullPath = StringDuplicate(fullPath);
		outResource->name = StringDuplicate(name);

		if (!found)
		{
			m_logger.Error("Load() - Failed to find file '{}' with any supported extension.", name);
			return false;
		}

		File f;
		f.Open(fullPath, FileModeRead | FileModeBinary);
		
		u64 fileSize;
		if (!f.Size(&fileSize))
		{
			m_logger.Error("Load() - Failed to get file size for '{}'.", fullPath);
			f.Close();
			return false;
		}

		i32 width;
		i32 height;
		i32 channelCount;

		// Allocate enough space for everything in the file
		const auto rawData = Memory.Allocate<u8>(fileSize, MemoryType::Texture);
		if (!rawData)
		{
			m_logger.Error("Load() - Unable to allocate memory to store raw data for '{}'.", fullPath);
			f.Close();
			return false;
		}

		// Actually read our file and store the result in rawData
		u64 bytesRead = 0;
		const auto result = f.ReadAll(reinterpret_cast<char*>(rawData), &bytesRead);
		f.Close();

		// Check if we could actually successfully read the data
		if (!result)
		{
			m_logger.Error("Load() - Unable to read data for '{}'.", fullPath);
			return false;
		}

		u8* data = stbi_load_from_memory(rawData, static_cast<i32>(fileSize), &width, &height, &channelCount, requiredChannelCount);
		if (!data)
		{
			m_logger.Error("Load() - STBI failed to load data from memory for '{}'.", fullPath);
			return false;
		}
		
		outResource->data.pixels = data;
		outResource->data.width = width;
		outResource->data.height = height;
		outResource->data.channelCount = requiredChannelCount;

		return true;
	}

	void ResourceLoader<ImageResource>::Unload(ImageResource* resource)
	{
		// Free the pixel data loaded in by STBI
		stbi_image_free(resource->data.pixels);

		if (resource->fullPath)
		{
			const auto size = StringLength(resource->fullPath) + 1;
			Memory.Free(resource->fullPath, size, MemoryType::String);
			resource->fullPath = nullptr;
		}

		if (resource->name)
		{
			const auto size = StringLength(resource->name) + 1;
			Memory.Free(resource->name, size, MemoryType::String);
			resource->name = nullptr;
		}
	}
}
