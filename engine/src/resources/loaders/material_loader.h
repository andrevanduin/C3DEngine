
#pragma once
#include "resource_loader.h"

namespace C3D
{
	struct MaterialResource final : Resource
	{
		MaterialConfig config;
	};

	template <>
	class ResourceLoader<MaterialResource> final : public IResourceLoader
	{
	public:
		ResourceLoader();

		bool Load(const char* name, MaterialResource* outResource) const;
		static void Unload(const MaterialResource* resource);
	};
}