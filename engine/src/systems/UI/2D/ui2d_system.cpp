#include "ui2d_system.h"

#include "UI/2D/flags.h"
#include "core/ecs/entity_view.h"
#include "core/frame_data.h"
#include "core/uuid.h"
#include "math/geometry_utils.h"
#include "resources/geometry_config.h"
#include "resources/shaders/shader.h"
#include "systems/geometry/geometry_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr const char* SHADER_NAME   = "Shader.Builtin.UI2D";

    UI2DSystem::UI2DSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager) {}

    bool UI2DSystem::OnInit(const UI2DSystemConfig& config)
    {
        INFO_LOG("Initializing.");

        if (config.maxControlCount == 0)
        {
            ERROR_LOG("Maximum amount of UI2D controls must be > 0.");
            return false;
        }

        if (config.memorySize == 0)
        {
            ERROR_LOG("Allocator size must be > 0.");
            return false;
        }

        if (!m_ecs.Create(m_pSystemsManager, config.memorySize, UI_2D::COMPONENT_COUNT, config.maxControlCount))
        {
            ERROR_LOG("Failed to create internal ECS for UI2D System.");
            return false;
        }

        m_ecs.AddComponentPool<UI_2D::IDComponent>("IDComponentPool");
        m_ecs.AddComponentPool<UI_2D::NameComponent>("NameComponentPool");
        m_ecs.AddComponentPool<UI_2D::TransformComponent>("TransformComponentPool");
        m_ecs.AddComponentPool<UI_2D::FlagsComponent>("FlagsComponentPool");
        m_ecs.AddComponentPool<UI_2D::RenderableComponent>("RenderableComponentPool");
        m_ecs.AddComponentPool<UI_2D::ParentComponent>("ParentComponentPool");
        m_ecs.AddComponentPool<UI_2D::NineSliceComponent>("NineSliceComponentPool");

        m_pass = UI2DPass(m_pSystemsManager);

        return true;
    }

    bool UI2DSystem::OnRun()
    {
        // Get the shader
        m_shader = Shaders.Get(SHADER_NAME);

        // Setup the texture map
        auto atlas = Textures.Acquire("ui_texture_atlas", true);
        if (!atlas)
        {
            WARN_LOG("Failed to Acquire atlas texture: '{}'. Falling back to default.", "ui_texture_atlas");
            atlas = Textures.GetDefault();
        }

        m_textureAtlas         = TextureMap(TextureFilter::ModeNearest, TextureRepeat::ClampToEdge);
        m_textureAtlas.texture = atlas;

        if (!Renderer.AcquireTextureMapResources(m_textureAtlas))
        {
            ERROR_LOG("Failed to Acquire Texture Map Resources.");
            return false;
        }

        return true;
    }

    bool UI2DSystem::OnUpdate(const FrameData& frameData) { return true; }

    bool UI2DSystem::PrepareFrame(const FrameData& frameData, Viewport* viewport, Camera* camera, UIMesh* meshes,
                                  const DynamicArray<UIText*, LinearAllocator>& texts)
    {
        using namespace UI_2D;

        m_renderData.Reset();
        m_renderData.SetAllocator(frameData.allocator);

        for (auto entityId : EntityView<RenderableComponent, TransformComponent, IDComponent>(m_ecs))
        {
            auto& rComponent  = m_ecs.GetComponent<RenderableComponent>(entityId);
            auto& tComponent  = m_ecs.GetComponent<TransformComponent>(entityId);
            auto& idComponent = m_ecs.GetComponent<IDComponent>(entityId);

            if (rComponent.geometry)
            {
                UIRenderData data;
                data.geometryData            = GeometryRenderData(tComponent.transform.GetWorld(), rComponent.geometry, idComponent.id);
                data.properties.diffuseColor = rComponent.diffuseColor;
                data.depth                   = rComponent.depth;
                data.instanceId              = rComponent.instanceId;
                data.pFrameNumber            = &rComponent.frameNumber;
                data.pDrawIndex              = &rComponent.drawIndex;
                m_renderData.EmplaceBack(data);
            }
        }

        // Sort the data such that lowest depth gets rendered first
        std::sort(m_renderData.begin(), m_renderData.end(), [](const UIRenderData& a, const UIRenderData& b) { return a.depth - b.depth; });

        m_pass.Prepare(viewport, camera, &m_textureAtlas, meshes, &texts, &m_renderData);

        return true;
    }

    EntityID UI2DSystem::AddPanel(f32 width, f32 height)
    {
        using namespace UI_2D;

        auto panel = m_ecs.Register();

        auto& idComponent = m_ecs.AddComponent<IDComponent>(panel);
        idComponent.id.Generate();

        auto& nameComponent = m_ecs.AddComponent<NameComponent>(panel);
        nameComponent.name  = "Panel";

        auto& tComponent = m_ecs.AddComponent<TransformComponent>(panel);
        tComponent.transform.SetPosition(vec3(50.0f, 50.0f, 0.0f));

        auto& sliceComponent           = m_ecs.AddComponent<NineSliceComponent>(panel);
        sliceComponent.size            = size;
        sliceComponent.cornerSize      = cornerSize;
        sliceComponent.atlasSize       = atlasSize;
        sliceComponent.cornerAtlasSize = cornerAtlasSize;
        sliceComponent.atlasMin        = atlasMin;
        sliceComponent.atlasMax        = atlasMax;

        auto& renderableComponent = m_ecs.AddComponent<RenderableComponent>(panel);

        constexpr f32 textureSize = 512.0f;

        auto config = GeometryUtils::GenerateUINineSliceConfig("PanelComponent_UI_Geometry", u16vec2(512, 512), u16vec2(32, 32),
                                                               u16vec2(512, 512), u16vec2(32, 32), u16vec2(0, 0), u16vec2(96, 96));
        // Acquire geometry
        renderableComponent.geometry = Geometric.AcquireFromConfig(config, true);
        if (!renderableComponent.geometry)
        {
            ERROR_LOG("Failed to Acquire Geometry.");
            return false;
        }

        // Acquire shader instance resources
        TextureMap* textureMaps[1] = { &m_textureAtlas };
        if (!Renderer.AcquireShaderInstanceResources(*m_shader, 1, textureMaps, &renderableComponent.instanceId))
        {
            ERROR_LOG("Failed to Acquire Shader Instance resources.");
            return false;
        }

        return true;
    }

    void UI2DSystem::OnShutdown()
    {
        INFO_LOG("Shutting down UI2D System.");

        if (m_textureAtlas.texture)
        {
            Textures.Release(m_textureAtlas.texture->name);
            m_textureAtlas.texture = nullptr;
        }

        Renderer.ReleaseTextureMapResources(m_textureAtlas);

        m_ecs.Destroy();
    }
}  // namespace C3D
