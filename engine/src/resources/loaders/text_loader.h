
#pragma once
#include "resource_loader.h"

namespace C3D
{
	struct TextResource final : Resource
	{
		String text;
	};

	template <>
	class ResourceLoader<TextResource> final : public IResourceLoader
	{
	public:
		explicit ResourceLoader(const Engine* engine);

		bool Load(const char* name, TextResource& resource) const;
		static void Unload(TextResource& resource);
	};
}
