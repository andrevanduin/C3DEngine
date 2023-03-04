
#include "text_loader.h"

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
		if (std::strlen(name) == 0 || !outResource) return false;

		// TODO: try different extensions
		auto fullPath = String::FromFormat("{}/{}/{}", Resources.GetBasePath(), typePath, name);

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
