
#pragma once
#include "systems/resource_system.h"

namespace C3D
{
	class ImageLoader final : public ResourceLoader
	{
	public:
		ImageLoader();

		bool Load(const string& name, Resource* outResource) override;

		void Unload(Resource* resource) override;
	};
}