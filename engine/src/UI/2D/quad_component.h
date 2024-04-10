
#pragma once
#include "component.h"

namespace C3D::UI_2D
{
    /** @brief Describes the internal data needed for a Component that is a quad */
    struct QuadComponent
    {
        Geometry* geometry = nullptr;
        RenderableComponent renderable;

        u16vec2 atlasMin;
        u16vec2 atlasMax;
        AtlasID atlasID;

        f32 offsetX = 0.0f;
        f32 offsetY = 0.0f;

        bool Initialize(Component& self, const char* name, AtlasID _atlasID, const u16vec2& size);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void OnResize(Component& self, const u16vec2& size);

        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D