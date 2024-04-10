
#pragma once
#include "component.h"
#include "text_component.h"

namespace C3D::UI_2D
{
    namespace Label
    {
        struct InternalData
        {
            TextComponent textComponent;
        };

        Component Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const u16vec2& pos, const String& text, FontHandle font);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);

        void SetText(Component& self, const String& text);

        void Destroy(Component& self, const DynamicAllocator* pAllocator);

    }  // namespace Label
}  // namespace C3D::UI_2D