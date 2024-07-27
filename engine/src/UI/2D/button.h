
#pragma once
#include "component.h"
#include "config.h"
#include "internal/nine_slice_component.h"

namespace C3D::UI_2D
{
    struct InternalData
    {
        NineSliceComponent nineSlice;
    };

    namespace Button
    {
        Component Create(const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const Config& config);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);

        void Destroy(Component& self, const DynamicAllocator* pAllocator);

    }  // namespace Button
}  // namespace C3D::UI_2D