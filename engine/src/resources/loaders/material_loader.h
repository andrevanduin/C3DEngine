
#pragma once
#include "resource_loader.h"
#include "resources/material.h"

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
		explicit ResourceLoader(const Engine* engine);

		bool Load(const char* name, MaterialResource& resource) const;
		static void Unload(MaterialResource& resource);
	};
}
