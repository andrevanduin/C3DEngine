
#include "binary_loader.h"

#include "core/c3d_string.h"
#include "core/logger.h"

#include "platform/filesystem.h"

#include "services/services.h"
#include "systems/resource_system.h"

namespace C3D
{
	ResourceLoader<BinaryResource>::ResourceLoader()
		: IResourceLoader("BINARY_LOADER", MemoryType::Array, ResourceType::Binary, nullptr, "")
	{}

	bool ResourceLoader<BinaryResource>::Load(const char* name, BinaryResource* outResource) const
	{
		if (StringLength(name) == 0 || !outResource) return false;

		char fullPath[512];
		const auto formatStr = "%s/%s/%s";

		// TODO: try different extensions
		StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name);

		File file;
		if (!file.Open(fullPath, FileModeRead | FileModeBinary))
		{
			m_logger.Error("Unable to open file for binary reading: '{}'", fullPath);
			return false;
		}

		// TODO: Should really use an allocator
		outResource->fullPath = StringDuplicate(fullPath);

		u64 fileSize = 0;
		if (!file.Size(&fileSize))
		{
			m_logger.Error("Unable to binary read size of file: '{}'", fullPath);
			file.Close();
			return false;
		}

		// TODO: should be using an allocator here
		outResource->data = Memory.Allocate<char>(fileSize, MemoryType::Array);
		outResource->name = StringDuplicate(name);

		if (!file.ReadAll(outResource->data, &outResource->size))
		{
			m_logger.Error("Unable to read binary file: '{}'", fullPath);
			file.Close();
			return false;
		}

		file.Close();
		return true;
	}

	void ResourceLoader<BinaryResource>::Unload(const BinaryResource* resource)
	{
		Memory.Free(resource->data, resource->size, MemoryType::Array);
	}
}
