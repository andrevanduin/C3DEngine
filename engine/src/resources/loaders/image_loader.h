
#pragma once
#include "resource_loader.h"

namespace C3D
{
	constexpr auto IMAGE_LOADER_EXTENSION_COUNT = 4;

	class ImageLoader final : public ResourceLoader
	{
	public:
		ImageLoader();

		bool Load(const char* name, Resource* outResource) override;

		void Unload(Resource* resource) override;
	};
}