
#pragma once
#include "renderer/rendergraph/rendergraph.h"
#include "ui_pass.h"

namespace C3D
{
    struct UI2DRendergraphConfig
    {
        const LinearAllocator* pFrameAllocator = nullptr;
    };

    class C3D_API UI2DRendergraph : public Rendergraph<UI2DRendergraphConfig>
    {
    public:
        bool Create(const C3D::String& name, const UI2DRendergraphConfig& config) override;

        bool OnPrepareRender(FrameData& frameData, const Viewport& viewport);

    private:
        UI2DPass m_uiPass;
    };
}  // namespace C3D