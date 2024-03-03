
#pragma once
#include "UI/2D/components.h"
#include "UI/2D/ui2d_defines.h"
#include "UI/2D/ui_pass.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/ecs/ecs.h"
#include "core/events/event_context.h"
#include "memory/allocators/dynamic_allocator.h"
#include "renderer/rendergraph/rendergraph_pass.h"
#include "resources/geometry_config.h"
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

        /* ------ Components Start ------ */

        Entity AddPanel(u16 x, u16 y, u16 width, u16 height, u16 cornerWidth, u16 cornerHeight);
        Entity AddButton(u16 x, u16 y, u16 width, u16 height);

        /* ------- Components End -------  */

        /* ------ Generic Handlers Start ------ */

        bool SetParent(Entity child, Entity parent);

        bool SetPosition(Entity entity, const vec2& translation);

        bool SetSize(Entity entity, u16 width, u16 height);

        bool SetRotation(Entity entity, f32 angle);

        /**
         * @brief Makes the provided entity clickable.
         *
         * @param entity The entity that you want to make clickable.
         * @return True if successful; false otherwise.
         */
        bool MakeClickable(Entity entity);

        /**
         * @brief Makes the provided entity clickable within the provided bounds.
         *
         * @param entity The entity that you want to make clickable.
         * @param bounds The bounds within the entity that you want to make clickable.
         *
         * @return True if successful; false otherwise.
         */
        bool MakeClickable(Entity entity, const Bounds& bounds);

        /**
         * @brief Add a OnClickHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnClick handler to.
         * @param handler The handler function that needs to be called on a click.
         * @return True if successful; false otherwise.
         */
        bool AddOnClickHandler(Entity entity, const UI_2D::OnClickEventHandler& handler);

        /* ------- Generic Handlers End -------  */

        UI2DPass* GetPass() { return &m_pass; }

        void OnShutdown() override;

    private:
        /** @brief Handles OnClick events for all the components managed by the UI2D System.
         * @return True if OnClick event is handled; false otherwise
         */
        bool OnClick(const EventContext& context);

        void SetupBaseComponent(Entity entity, const char* name, u16 x, u16 y, u16 width, u16 height);

        void SetupNineSliceComponent(Entity entity, const u16vec2& cornerSize, const u16vec2& cornerAtlasSize, const u16vec2& atlasMin,
                                     const u16vec2& atlasMax);
        void RegenerateNineSliceGeometry(Entity entity);

        bool SetupRenderableComponent(Entity entity, const UIGeometryConfig& config);

        void SetupClickableComponent(Entity entity, u16 width, u16 height);

        UI2DPass m_pass;
        Shader* m_shader;

        DynamicArray<UIRenderData, LinearAllocator> m_renderData;

        TextureMap m_textureAtlas;

        RegisteredEventCallback m_onClickEventRegisteredCallback;

        ECS m_ecs;
    };
}  // namespace C3D
