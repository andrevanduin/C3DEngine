
#pragma once
#include "resource_loader.h"

namespace C3D
{
	constexpr auto IMAGE_LOADER_EXTENSION_COUNT = 4;

	struct ImageResource final : Resource
	{
		ImageResourceData data;
	};

	template <>
	class ResourceLoader<ImageResource> final : public IResourceLoader
	{
	public:
		ResourceLoader();

		bool Load(const char* name, ImageResource* outResource) const;
		static void Unload(const ImageResource* resource);
	};
}