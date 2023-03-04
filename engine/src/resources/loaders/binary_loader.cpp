
#include "binary_loader.h"

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
		if (std::strlen(name) == 0 || !outResource) return false;

		// TODO: try different extensions
		auto fullPath = String::FromFormat("{}/{}/{}", Resources.GetBasePath(), typePath, name);

		File file;
		if (!file.Open(fullPath, FileModeRead | FileModeBinary))
		{
			m_logger.Error("Unable to open file for binary reading: '{}'", fullPath);
			return false;
		}

		outResource->fullPath = fullPath;

		u64 fileSize = 0;
		if (!file.Size(&fileSize))
		{
			m_logger.Error("Unable to binary read size of file: '{}'", fullPath);
			file.Close();
			return false;
		}

		// TODO: should be using an allocator here
		outResource->data = Memory.Allocate<char>(MemoryType::Array, fileSize);
		outResource->name = name;

		if (!file.ReadAll(outResource->data, &outResource->size))
		{
			m_logger.Error("Unable to read binary file: '{}'", fullPath);
			file.Close();
			return false;
		}

		file.Close();
		return true;
	}

	void ResourceLoader<BinaryResource>::Unload(BinaryResource* resource)
	{
		Memory.Free(MemoryType::Array, resource->data);
		resource->data = nullptr;

		resource->name.Destroy();
		resource->fullPath.Destroy();
	}
}
