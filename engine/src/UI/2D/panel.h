
#pragma once
#include "component.h"

namespace C3D::UI_2D
{
    namespace Panel
    {
        Component Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const u16vec2& pos, const u16vec2& size, const u16vec2& cornerSize);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void OnResize(Component& self);

        void Destroy(Component& self, const DynamicAllocator* pAllocator);

    }  // namespace Panel
}  // namespace C3D::UI_2D