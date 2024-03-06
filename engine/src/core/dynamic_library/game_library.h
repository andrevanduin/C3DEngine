
#pragma once
#include "core/application.h"
#include "dynamic_library.h"

namespace C3D
{
    class C3D_API GameLibrary final : public DynamicLibrary
    {
    public:
        ApplicationState* CreateState();

        Application* Create(ApplicationState* state);
    };
}  // namespace C3D
