
#pragma once
#include "UI/2D/component.h"
#include "UI/2D/ui2d_defines.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
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
        void Prepare(const Viewport& viewport, const DynamicArray<UI_2D::Component>& components);
        bool Execute(const FrameData& frameData) override;

    private:
        Shader* m_shader = nullptr;
        UI_2D::ShaderLocations m_locations;

        const DynamicArray<UI_2D::Component>* m_pComponents = nullptr;
    };
}  // namespace C3D