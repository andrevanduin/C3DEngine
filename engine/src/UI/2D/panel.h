
#pragma once
#include "component.h"
#include "config.h"

namespace C3D::UI_2D
{
    namespace Panel
    {
        Component Create(const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const Config& config);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void OnResize(Component& self);

        void Destroy(Component& self, const DynamicAllocator* pAllocator);

    }  // namespace Panel
}  // namespace C3D::UI_2D