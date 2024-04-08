
#pragma once
#include "component.h"

namespace C3D::UI_2D
{
    namespace Label
    {
        Component Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator);
        void Destroy(Component& self, const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const u16vec2& pos, const String& text, FontHandle font);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);

    }  // namespace Label
}  // namespace C3D::UI_2D