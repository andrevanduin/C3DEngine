
#pragma once
#include "UI/2D/component.h"

namespace C3D::UI_2D
{
    struct ClippingComponent
    {
        Geometry* geometry = nullptr;
        GeometryRenderData renderData;
        u8 id = 0;

        u16vec2 size;
        f32 offsetX = 0.0f;
        f32 offsetY = 0.0f;

        bool Initialize(Component& self, const char* name, const u16vec2& _size);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void ResetClipping(Component& self);

        void OnResize(Component& self, const u16vec2& _size);

        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D