
#pragma once
#include "resource_loader.h"

namespace C3D
{
	class TextLoader final : public ResourceLoader
	{
	public:
		TextLoader();

		bool Load(const string& name, Resource* outResource) override;
	};
}