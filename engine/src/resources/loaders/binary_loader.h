
#pragma once
#include "resource_loader.h"

namespace C3D
{
	struct BinaryResource final : Resource
	{
		char* data;
		u64 size;
	};

	template <>
	class ResourceLoader<BinaryResource> final : public IResourceLoader
	{
	public:
		explicit ResourceLoader(const Engine* engine);

		bool Load(const char* name, BinaryResource& resource) const;
		static void Unload(BinaryResource& resource);
	};
}