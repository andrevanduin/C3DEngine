
#pragma once
#include "component.h"
#include "config.h"
#include "internal/text_component.h"

namespace C3D::UI_2D
{
    namespace Label
    {
        struct InternalData
        {
            TextComponent textComponent;
        };

        Component Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const Config& config);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);

        void SetText(Component& self, const String& text);

        void Destroy(Component& self, const DynamicAllocator* pAllocator);

    }  // namespace Label
}  // namespace C3D::UI_2D