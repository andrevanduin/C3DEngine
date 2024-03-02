
#pragma once
#include "UI/2D/components.h"
#include "UI/2D/ui_pass.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/ecs/ecs.h"
#include "memory/allocators/dynamic_allocator.h"
#include "renderer/rendergraph/rendergraph_pass.h"
#include "resources/textures/texture_map.h"
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

        EntityID AddPanel(f32 width, f32 height);

        UI2DPass* GetPass() { return &m_pass; }

        void OnShutdown() override;

    private:
        /** @brief The width and height of the entire nine slice in pixels. */
        u16vec2 size;
        /** @brief The width and height of the actual corner in pixels. */
        u16vec2 cornerSize;
        /** @brief The x and y min values in the atlas. */
        u16vec2 atlasMin;
        /** @brief The x and y max values in the atlas. */
        u16vec2 atlasMax;
        /** @brief The size of the atlas. */
        u16vec2 atlasSize;
        /** @brief The width and height of the corner in the atlas. */
        u16vec2 cornerAtlasSize;

        void AddNineSlice(EntityID entity, const u16vec2& size, const u16vec2& cornerSize, const u16vec2& atlasSize,
                          const u16vec2& cornerAtlasSize, const u16vec2& atlasMin, const u16vec2& atlasMax);

        UI2DPass m_pass;
        Shader* m_shader;

        DynamicArray<UIRenderData, LinearAllocator> m_renderData;

        TextureMap m_textureAtlas;

        ECS m_ecs;
    };
}  // namespace C3D
