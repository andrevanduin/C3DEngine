
#pragma once
#include "component.h"

namespace C3D::UI_2D
{
    /** @brief Describes the internal data needed for a Component that has a nine slice. */
    struct NineSliceComponent
    {
        NineSliceComponent() { Logger::Info("NineSliceComponent ctor called"); }

        Geometry* geometry = nullptr;
        u16vec2 cornerSize;
        u16vec2 atlasMin;
        u16vec2 atlasMax;

        RenderableComponent renderable;

        bool Initialize(Component& self, const char* name, ComponentType type, const u16vec2& _cornerSize);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D