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
    using namespace UI_2D;

    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr const char* SHADER_NAME   = "Shader.Builtin.UI2D";
    constexpr u16vec2 atlasSize         = u16vec2(512, 512);  // Size of the entire UI atlas

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

        m_ecs.AddComponentPool<IDComponent>("IDComponentPool");
        m_ecs.AddComponentPool<NameComponent>("NameComponentPool");
        m_ecs.AddComponentPool<TransformComponent>("TransformComponentPool");
        m_ecs.AddComponentPool<FlagsComponent>("FlagsComponentPool");
        m_ecs.AddComponentPool<RenderableComponent>("RenderableComponentPool");
        m_ecs.AddComponentPool<ParentComponent>("ParentComponentPool");
        m_ecs.AddComponentPool<NineSliceComponent>("NineSliceComponentPool");
        m_ecs.AddComponentPool<ClickableComponent>("ClickableComponentPool");

        m_pass = UI2DPass(m_pSystemsManager);

        m_onClickEventRegisteredCallback = Event.Register(
            EventCodeButtonClicked, [this](const u16 code, void* sender, const C3D::EventContext& context) { return OnClick(context); });

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

    bool UI2DSystem::OnClick(const EventContext& context)
    {
        auto x = context.data.u16[1];
        auto y = context.data.u16[2];

        for (auto entity : EntityView<ClickableComponent>(m_ecs))
        {
            // Iterate over all clickable entities and check if we are clicking them
            auto& bounds = m_ecs.GetComponent<ClickableComponent>(entity).bounds;

            if (x >= bounds.x && x <= bounds.x + bounds.width && y >= bounds.y && y <= bounds.y + bounds.height)
            {
                INFO_LOG("Button clicked");
                return true;  // Click handled
            }
        }

        // Let's return false since if we reach this point the click event was not handled yet
        return false;
    }

    bool UI2DSystem::PrepareFrame(const FrameData& frameData, Viewport* viewport, Camera* camera, UIMesh* meshes,
                                  const DynamicArray<UIText*, LinearAllocator>& texts)
    {
        m_renderData.Reset();
        m_renderData.SetAllocator(frameData.allocator);

        for (auto entity : EntityView<RenderableComponent, TransformComponent, IDComponent>(m_ecs))
        {
            auto& rComponent  = m_ecs.GetComponent<RenderableComponent>(entity);
            auto& tComponent  = m_ecs.GetComponent<TransformComponent>(entity);
            auto& idComponent = m_ecs.GetComponent<IDComponent>(entity);

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
        std::sort(m_renderData.begin(), m_renderData.end(), [](const UIRenderData& a, const UIRenderData& b) { return a.depth < b.depth; });

        m_pass.Prepare(viewport, camera, &m_textureAtlas, meshes, &texts, &m_renderData);

        return true;
    }

    EntityID UI2DSystem::AddPanel(u16 width, u16 height, u16 cornerWidth, u16 cornerHeight)
    {
        auto panel = m_ecs.Register();

        auto& idComponent = m_ecs.AddComponent<IDComponent>(panel);
        idComponent.id.Generate();

        auto& nameComponent = m_ecs.AddComponent<NameComponent>(panel);
        nameComponent.name  = "Panel";

        auto& tComponent = m_ecs.AddComponent<TransformComponent>(panel);
        tComponent.transform.SetPosition(vec3(50.0f, 50.0f, 0.0f));

        // TODO: Get from config
        constexpr u16vec2 panelCornerAtlasSize = u16vec2(32, 32);  // Size of one corner of the panel nine slice
        constexpr u16vec2 panelAtlasMin        = u16vec2(0, 0);    // The min values for the panel in the UI atlas
        constexpr u16vec2 panelAtlasMax        = u16vec2(96, 96);  // The max values for the panel in the UI atlas

        auto& sliceComponent           = m_ecs.AddComponent<NineSliceComponent>(panel);
        sliceComponent.size            = u16vec2(width, height);
        sliceComponent.cornerSize      = u16vec2(cornerWidth, cornerHeight);
        sliceComponent.atlasSize       = atlasSize;
        sliceComponent.cornerAtlasSize = panelCornerAtlasSize;
        sliceComponent.atlasMin        = panelAtlasMin;
        sliceComponent.atlasMax        = panelAtlasMax;

        auto& renderableComponent = m_ecs.AddComponent<RenderableComponent>(panel);
        renderableComponent.depth = 0;

        auto config = GeometryUtils::GenerateUINineSliceConfig("Panel_UI_Geometry", sliceComponent.size, sliceComponent.cornerSize,
                                                               atlasSize, panelCornerAtlasSize, panelAtlasMin, panelAtlasMax);
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

    EntityID UI2DSystem::AddButton(u16 width, u16 height)
    {
        auto button = m_ecs.Register();

        auto& idComponent = m_ecs.AddComponent<IDComponent>(button);
        idComponent.id.Generate();

        auto& nameComponent = m_ecs.AddComponent<NameComponent>(button);
        nameComponent.name  = "Button";

        auto& tComponent = m_ecs.AddComponent<TransformComponent>(button);
        tComponent.transform.SetPosition(vec3(150.0f, 150.0f, 0.0f));

        // TODO: Get from config
        constexpr u16vec2 buttonCornerAtlasSize = u16vec2(8, 8);     // Size of one corner of the button nine slice
        constexpr u16vec2 buttonAtlasMin        = u16vec2(120, 24);  // The min values for the button in the UI atlas
        constexpr u16vec2 buttonAtlasMax        = u16vec2(168, 72);  // The max values for the button in the UI atlas

        auto& sliceComponent           = m_ecs.AddComponent<NineSliceComponent>(button);
        sliceComponent.size            = u16vec2(width, height);
        sliceComponent.cornerSize      = u16vec2(8, 8);
        sliceComponent.atlasSize       = atlasSize;
        sliceComponent.cornerAtlasSize = buttonCornerAtlasSize;
        sliceComponent.atlasMin        = buttonAtlasMin;
        sliceComponent.atlasMax        = buttonAtlasMax;

        auto& clickableComponent         = m_ecs.AddComponent<ClickableComponent>(button);
        clickableComponent.bounds.x      = 150.0f;
        clickableComponent.bounds.y      = 150.0f;
        clickableComponent.bounds.width  = width;
        clickableComponent.bounds.height = height;

        auto& renderableComponent = m_ecs.AddComponent<RenderableComponent>(button);
        renderableComponent.depth = 1;

        auto config = GeometryUtils::GenerateUINineSliceConfig("Button_UI_Geometry", sliceComponent.size, sliceComponent.cornerSize,
                                                               atlasSize, buttonCornerAtlasSize, buttonAtlasMin, buttonAtlasMax);
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

        Event.Unregister(m_onClickEventRegisteredCallback);

        if (m_textureAtlas.texture)
        {
            Textures.Release(m_textureAtlas.texture->name);
            m_textureAtlas.texture = nullptr;
        }

        Renderer.ReleaseTextureMapResources(m_textureAtlas);

        m_ecs.Destroy();
    }
}  // namespace C3D
