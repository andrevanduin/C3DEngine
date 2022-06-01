
#include "binary_loader.h"

#include "core/c3d_string.h"
#include "core/logger.h"
#include "core/memory.h"
#include "platform/filesystem.h"
#include "services/services.h"

namespace C3D
{
	BinaryLoader::BinaryLoader()
		: ResourceLoader("BINARY_LOADER", MemoryType::Array, ResourceType::Binary, nullptr, "")
	{}

	bool BinaryLoader::Load(const string& name, Resource* outResource)
	{
		if (name.empty() || !outResource) return false;

		char fullPath[512];
		const auto formatStr = "%s/%s/%s";

		// TODO: try different extensions
		StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name.data());

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
		char* resourceData = Memory::Allocate<char>(fileSize, MemoryType::Array);
		u64 readSize = 0;
		if (!file.ReadAll(resourceData, &readSize))
		{
			m_logger.Error("Unable to read binary file: '{}'", fullPath);
			file.Close();
			return false;
		}

		file.Close();

		outResource->data = resourceData;
		outResource->dataSize = readSize;
		outResource->name = name.data();

		return true;
	}
}
