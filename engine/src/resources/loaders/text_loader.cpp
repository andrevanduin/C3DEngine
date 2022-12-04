
#include "text_loader.h"

#include "core/c3d_string.h"
#include "core/logger.h"

#include "platform/filesystem.h"
#include "services/services.h"

#include "systems/resource_system.h"

namespace C3D
{
	ResourceLoader<TextResource>::ResourceLoader()
		: IResourceLoader("TEXT_LOADER", MemoryType::String, ResourceType::Text, nullptr, "")
	{}

	bool ResourceLoader<TextResource>::Load(const char* name, TextResource* outResource) const
	{
		if (StringLength(name) == 0 || !outResource) return false;

		char fullPath[512];
		const auto formatStr = "%s/%s/%s";

		// TODO: try different extensions
		StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name);

		File file;
		if (!file.Open(fullPath, FileModeRead))
		{
			m_logger.Error("Unable to open file for text reading: '{}'", fullPath);
			return false;
		}

		outResource->fullPath = fullPath;

		u64 fileSize = 0;
		if (!file.Size(&fileSize))
		{
			m_logger.Error("Unable to read size of file: '{}'", fullPath);
			file.Close();
			return false;
		}

		// TODO: should be using an allocator here
		outResource->text.Reserve(fileSize);
		outResource->name = name;

		if (!file.ReadAll(outResource->text))
		{
			m_logger.Error("Unable to read text file: '{}'", fullPath);
			file.Close();
			return false;
		}

		file.Close();
		return true;
	}

	void ResourceLoader<TextResource>::Unload(TextResource* resource)
	{
		resource->text.Destroy();
		resource->name.Destroy();
		resource->fullPath.Destroy();
	}
}
