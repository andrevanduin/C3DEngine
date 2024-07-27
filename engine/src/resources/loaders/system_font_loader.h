
#pragma once
#include "resource_loader.h"
#include "resources/resource_types.h"
#include "resources/font.h"

namespace C3D
{
	enum class SystemFontFileType
	{
		NotFound,
		CSF,
		FontConfig,
	};

	struct SupportedSystemFontFileType
	{
		const char* extension;
		SystemFontFileType type;
		bool isBinary;
	};

	struct SystemFontResource : Resource
	{
		DynamicArray<SystemFontFace> fonts;
		u64 binarySize;
		void* fontBinary;
	};

	template <>
	class ResourceLoader<SystemFontResource> final : public IResourceLoader
	{
	public:
		ResourceLoader();

		bool Load(const char* name, SystemFontResource& outResource) const;
		static void Unload(SystemFontResource& resource);
	};
}