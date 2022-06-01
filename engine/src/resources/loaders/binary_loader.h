
#pragma once
#include "resource_loader.h"

namespace C3D
{
	class BinaryLoader final : public ResourceLoader
	{
	public:
		BinaryLoader();

		bool Load(const string& name, Resource* outResource) override;
	};
}