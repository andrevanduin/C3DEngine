
#pragma once
#include "UI/2D/components.h"
#include "UI/2D/ui_pass.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/ecs/ecs.h"
#include "core/events/event_context.h"
#include "memory/allocators/dynamic_allocator.h"
#include "renderer/rendergraph/rendergraph_pass.h"
#include "resources/textures/texture_map.h"
#include "systems/events/event_system.h"
#include "systems/system.h"

namespace C3D
{
    class Shader;
    class Viewport;
    class Camera;

    struct UI2DSystemConfig
    {
        u64 maxControlCount = 1024;
        u64 memorySize      = MebiBytes(8);
    };

    class C3D_API UI2DSystem final : public SystemWithConfig<UI2DSystemConfig>
    {
    public:
        explicit UI2DSystem(const SystemManager* pSystemsManager);

        bool OnInit(const UI2DSystemConfig& config) override;

        bool OnRun();

        bool OnUpdate(const FrameData& frameData) override;

        bool PrepareFrame(const FrameData& frameData, Viewport* viewport, Camera* camera, UIMesh* meshes,
                          const DynamicArray<UIText*, LinearAllocator>& texts);

        EntityID AddPanel(u16 width, u16 height, u16 cornerWidth, u16 cornerHeight);
        EntityID AddButton(u16 width, u16 height);

        UI2DPass* GetPass() { return &m_pass; }

        void OnShutdown() override;

    private:
        /** @brief Handles OnClick events for all the components managed by the UI2D System.
         * @return True if OnClick event is handled; false otherwise
         */
        bool OnClick(const EventContext& context);

        UI2DPass m_pass;
        Shader* m_shader;

        DynamicArray<UIRenderData, LinearAllocator> m_renderData;

        TextureMap m_textureAtlas;

        RegisteredEventCallback m_onClickEventRegisteredCallback;

        ECS m_ecs;
    };
}  // namespace C3D
