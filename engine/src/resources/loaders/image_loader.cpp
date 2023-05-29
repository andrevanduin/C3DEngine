
#include "image_loader.h"

#include "core/logger.h"
#include "platform/filesystem.h"

#include "resources/resource_types.h"

#include "services/system_manager.h"
#include "systems/resource_system.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

namespace C3D
{
	ResourceLoader<ImageResource>::ResourceLoader()
		: IResourceLoader("IMAGE_LOADER", MemoryType::Texture, ResourceType::Image, nullptr, "textures")
	{}

	bool ResourceLoader<ImageResource>::Load(const char* name, ImageResource& resource) const
	{
		return Load(name, resource, {});
	}

	bool ResourceLoader<ImageResource>::Load(const char* name, ImageResource& resource, const ImageResourceParams& params) const
	{
		if (std::strlen(name) == 0) return false;

		constexpr i32 requiredChannelCount = 4;
		stbi_set_flip_vertically_on_load_thread(params.flipY);

		String fullPath(512);

		// Try different extensions
		const char* extensions[IMAGE_LOADER_EXTENSION_COUNT] = { "tga", "png", "jpg", "bmp" };
		bool found = false;

		for (const auto extension : extensions)
		{
			const auto formatStr = "{}/{}/{}.{}";
			fullPath = String::FromFormat(formatStr, Resources.GetBasePath(), typePath, name, extension);
			// Check if the requested file exists with the current extension
			if (File::Exists(fullPath))
			{
				// It exists so we break out of the loop
				found = true;
				break;
			}
		}

		// Take a copy of the resource path and name
		resource.fullPath = fullPath;
		resource.name = name;

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
		const auto rawData = Memory.Allocate<u8>(MemoryType::Texture, fileSize);
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

		Memory.Free(MemoryType::Texture, rawData);
		
		resource.data.pixels = data;
		resource.data.width = width;
		resource.data.height = height;
		resource.data.channelCount = requiredChannelCount;

		return true;
	}

	void ResourceLoader<ImageResource>::Unload(ImageResource& resource)
	{
		// Free the pixel data loaded in by STBI
		stbi_image_free(resource.data.pixels);

		resource.fullPath.Destroy();
		resource.name.Destroy();
	}
}
