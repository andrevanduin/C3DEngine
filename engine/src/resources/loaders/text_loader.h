
#pragma once
#include "resource_loader.h"

namespace C3D
{
	struct TextResource final : Resource
	{
		~TextResource()
		{
			if (text)
			{
				Memory.Free(text, size, MemoryType::Array);
				text = nullptr;
				size = 0;
			}
		}

		char* text;
		u64 size;
	};

	template <>
	class ResourceLoader<TextResource> final : public IResourceLoader
	{
	public:
		ResourceLoader();

		bool Load(const char* name, TextResource* outResource) const;
		static void Unload(const TextResource* resource);
	};
}
