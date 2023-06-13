
#pragma once
#include "dynamic_library.h"
#include "core/application.h"

namespace C3D
{
	class C3D_API GameLibrary final : public DynamicLibrary
	{
	public:
		GameLibrary();

		ApplicationState* CreateState();

		Application* Create(ApplicationState* state);
	};
}
