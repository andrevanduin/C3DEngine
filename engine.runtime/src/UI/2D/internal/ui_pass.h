
#pragma once
#include "UI/2D/component.h"
#include "UI/2D/ui2d_defines.h"
#include "containers/dynamic_array.h"
#include "containers/handle_table.h"
#include "defines.h"
#include "memory/allocators/linear_allocator.h"
#include "renderer/renderer_types.h"
#include "renderer/rendergraph/renderpass.h"

namespace C3D
{
    class Shader;

    class C3D_API UI2DPass : public Renderpass
    {
    public:
        UI2DPass();

        bool Initialize(const LinearAllocator* frameAllocator) override;
        void Prepare(const Viewport& viewport, UI_2D::Component* components, u32 numberOfComponents);
        bool Execute(const FrameData& frameData) override;

    private:
        Shader* m_shader = nullptr;
        UI_2D::ShaderLocations m_locations;

        UI_2D::Component* m_pComponents = nullptr;
        u32 m_numberOfComponents        = 0;
    };
}  // namespace C3D