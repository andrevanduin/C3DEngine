
#pragma once
#include "renderer/passes/scene_pass.h"
#include "renderer/passes/shadow_map_pass.h"
#include "renderer/passes/skybox_pass.h"
#include "rendergraph.h"

namespace C3D
{
    class LinearAllocator;

    struct ForwardRendergraphConfig
    {
        u16 shadowMapResolution                = 4096;
        const LinearAllocator* pFrameAllocator = nullptr;
    };

    class C3D_API ForwardRendergraph : public Rendergraph<ForwardRendergraphConfig>
    {
    public:
        bool Create(const String& name, const ForwardRendergraphConfig& config) override;

        bool OnPrepareRender(FrameData& frameData, const Viewport& currentViewport, Camera* currentCamera, const SimpleScene& scene,
                             u32 renderMode, const DynamicArray<DebugLine3D>& debugLines, const DynamicArray<DebugBox3D>& debugBoxes);

    private:
        SkyboxPass m_skyboxPass;
        ShadowMapPass m_shadowMapPass;
        ScenePass m_scenePass;
    };
}  // namespace C3D