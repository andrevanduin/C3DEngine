
#pragma once
#include "resource_loader.h"
#include "resources/texture.h"

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
		bool Load(const char* name, ImageResource* outResource, const ImageResourceParams& params) const;

		static void Unload(ImageResource* resource);
	};
}
