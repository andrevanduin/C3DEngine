
#pragma once
#include "resource_loader.h"

namespace C3D
{
	class MaterialLoader final : public ResourceLoader
	{
	public:
		MaterialLoader();

		bool Load(const string& name, Resource* outResource) override;
	};
}