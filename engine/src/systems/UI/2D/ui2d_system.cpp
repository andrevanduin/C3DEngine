#include "ui2d_system.h"

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
        auto ctx = OnClickEventContext(context.data.u16[0], context.data.u16[1], context.data.u16[2]);

        for (auto entity : EntityView<ClickableComponent>(m_ecs))
        {
            // Iterate over all clickable entities and check if we are clicking them
            auto& clickableComponent = m_ecs.GetComponent<ClickableComponent>(entity);
            auto& bounds             = clickableComponent.bounds;

            auto& transformComponent = m_ecs.GetComponent<TransformComponent>(entity);
            auto model               = transformComponent.transform.GetWorld();
            auto inverse             = glm::inverse(model);

            auto transformedPos = inverse * vec4(ctx.x, ctx.y, 0.0f, 1.0f);

            if (transformedPos.x >= bounds.x && transformedPos.x <= bounds.x + bounds.width && transformedPos.y >= bounds.y &&
                transformedPos.y <= bounds.y + bounds.height)
            {
                return clickableComponent.handler(ctx);
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

    Entity UI2DSystem::AddPanel(u16 x, u16 y, u16 width, u16 height, u16 cornerWidth, u16 cornerHeight)
    {
        auto panel = m_ecs.Register();

        SetupBaseComponent(panel, "Panel", x, y, width, height);

        // TODO: Get from config
        constexpr u16vec2 panelCornerAtlasSize = u16vec2(32, 32);  // Size of one corner of the panel nine slice
        constexpr u16vec2 panelAtlasMin        = u16vec2(0, 0);    // The min values for the panel in the UI atlas
        constexpr u16vec2 panelAtlasMax        = u16vec2(96, 96);  // The max values for the panel in the UI atlas
        auto panelCornerSize                   = u16vec2(cornerWidth, cornerHeight);

        SetupNineSliceComponent(panel, panelCornerSize, panelCornerAtlasSize, panelAtlasMin, panelAtlasMax);

        auto config = GeometryUtils::GenerateUINineSliceConfig("Panel_UI_Geometry", u16vec2(width, height), panelCornerSize, atlasSize,
                                                               panelCornerAtlasSize, panelAtlasMin, panelAtlasMax);

        if (!SetupRenderableComponent(panel, config))
        {
            ERROR_LOG("Failed to setup RenderableComponent.");
            return Entity::Invalid();
        }

        return panel;
    }

    Entity UI2DSystem::AddButton(u16 x, u16 y, u16 width, u16 height)
    {
        auto button = m_ecs.Register();

        SetupBaseComponent(button, "Button", x, y, width, height);

        // TODO: Get from config
        constexpr u16vec2 buttonCornerAtlasSize = u16vec2(8, 8);     // Size of one corner of the button nine slice
        constexpr u16vec2 buttonAtlasMin        = u16vec2(120, 24);  // The min values for the button in the UI atlas
        constexpr u16vec2 buttonAtlasMax        = u16vec2(168, 72);  // The max values for the button in the UI atlas
        constexpr u16vec2 buttonCornerSize      = u16vec2(8, 8);

        SetupNineSliceComponent(button, buttonCornerSize, buttonCornerAtlasSize, buttonAtlasMin, buttonAtlasMax);

        auto config = GeometryUtils::GenerateUINineSliceConfig("Button_UI_Geometry", u16vec2(width, height), buttonCornerSize, atlasSize,
                                                               buttonCornerAtlasSize, buttonAtlasMin, buttonAtlasMax);

        if (!SetupRenderableComponent(button, config))
        {
            ERROR_LOG("Failed to setup RenderableComponent.");
            return Entity::Invalid();
        }

        SetupClickableComponent(button, width, height);

        return button;
    }

    bool UI2DSystem::SetParent(Entity child, Entity parent)
    {
        if (!child.IsValid())
        {
            ERROR_LOG("Provided child is not valid.");
            return false;
        }

        if (!parent.IsValid())
        {
            ERROR_LOG("Provided parent is not valid.");
            return false;
        }

        if (!m_ecs.HasComponent<ParentComponent>(parent))
        {
            INFO_LOG("Entity: '{}' does not yet have a ParentComponent. Adding it first.", parent);
            m_ecs.AddComponent<ParentComponent>(parent);
        }

        auto& pComponent = m_ecs.GetComponent<ParentComponent>(parent);
        pComponent.children.PushBack(child);

        if (m_ecs.HasComponent<TransformComponent>(child) && m_ecs.HasComponent<TransformComponent>(parent))
        {
            INFO_LOG("Since both Child and Parent have a Transform component we will set the parent inside the child's Transform.");

            auto& tcComponent = m_ecs.GetComponent<TransformComponent>(child);
            auto& tpComponent = m_ecs.GetComponent<TransformComponent>(parent);

            tcComponent.transform.SetParent(&tpComponent.transform);
        }

        return true;
    }

    bool UI2DSystem::SetPosition(Entity entity, const vec2& translation)
    {
        if (!entity.IsValid())
        {
            ERROR_LOG("Entity provided is not valid.");
            return false;
        }

        if (!m_ecs.HasComponent<TransformComponent>(entity))
        {
            ERROR_LOG("Entity: '{}' does not have a transform component to translate.", entity);
            return false;
        }

        m_ecs.GetComponent<TransformComponent>(entity).transform.SetPosition(vec3(translation, 0.0f));

        return true;
    }

    bool UI2DSystem::SetSize(Entity entity, u16 width, u16 height)
    {
        if (!entity.IsValid())
        {
            ERROR_LOG("Entity provided is not valid.");
            return false;
        }

        if (!m_ecs.HasComponent<TransformComponent>(entity))
        {
            ERROR_LOG("Entity: '{}' does not have a transform component to resize.", entity);
            return false;
        }

        auto& tComponent  = m_ecs.GetComponent<TransformComponent>(entity);
        tComponent.width  = width;
        tComponent.height = height;

        if (m_ecs.HasComponent<NineSliceComponent>(entity))
        {
            // If we have a nine slice component we should regenerate it
            RegenerateNineSliceGeometry(entity);
        }

        return true;
    }

    bool UI2DSystem::SetRotation(Entity entity, f32 angle)
    {
        if (!entity.IsValid())
        {
            ERROR_LOG("Entity provided is not valid.");
            return false;
        }

        if (!m_ecs.HasComponent<TransformComponent>(entity))
        {
            ERROR_LOG("Entity: '{}' does not have a transform component to rotate.", entity);
            return false;
        }

        auto rotation = glm::rotate(quat(1.0f, 0.0f, 0.0f, 0.0f), angle, vec3(0.0f, 0.0f, 1.0f));
        m_ecs.GetComponent<TransformComponent>(entity).transform.SetRotation(rotation);

        return true;
    }

    bool UI2DSystem::MakeClickable(Entity entity)
    {
        if (!entity.IsValid())
        {
            ERROR_LOG("Entity provided is not valid.");
            return false;
        }

        if (!m_ecs.HasComponent<TransformComponent>(entity))
        {
            ERROR_LOG("Entity: '{}' does not have a transform component to determine the bounds from. Please provide bounds manually.",
                      entity);
            return false;
        }

        auto& tComponent = m_ecs.GetComponent<TransformComponent>(entity);
        return MakeClickable(entity, Bounds(0.0f, 0.0f, tComponent.width, tComponent.height));
    }

    bool UI2DSystem::MakeClickable(Entity entity, const Bounds& bounds)
    {
        if (!entity.IsValid())
        {
            ERROR_LOG("Entity provided is not valid.");
            return false;
        }

        auto& cComponent  = m_ecs.AddComponent<ClickableComponent>(entity);
        cComponent.bounds = bounds;

        return true;
    }

    bool UI2DSystem::AddOnClickHandler(Entity entity, const OnClickEventHandler& handler)
    {
        if (!entity.IsValid())
        {
            ERROR_LOG("Entity provided is not valid.");
            return false;
        }

        if (!m_ecs.HasComponent<ClickableComponent>(entity))
        {
            INFO_LOG("Entity: '{}' does not have a Clickable Component by default. Adding it first (with default bounds).", entity);
            MakeClickable(entity);
        }

        auto& clickableComponent   = m_ecs.GetComponent<ClickableComponent>(entity);
        clickableComponent.handler = handler;
        return true;
    }

    void UI2DSystem::SetupBaseComponent(Entity entity, const char* name, u16 x, u16 y, u16 width, u16 height)
    {
        auto& idComponent = m_ecs.AddComponent<IDComponent>(entity);
        idComponent.id.Generate();

        auto& nameComponent = m_ecs.AddComponent<NameComponent>(entity);
        nameComponent.name  = name;

        auto& tComponent = m_ecs.AddComponent<TransformComponent>(entity);
        tComponent.transform.SetPosition(vec3(x, y, 0.0f));
        tComponent.width  = width;
        tComponent.height = height;
    }

    void UI2DSystem::SetupNineSliceComponent(Entity entity, const u16vec2& cornerSize, const u16vec2& cornerAtlasSize,
                                             const u16vec2& atlasMin, const u16vec2& atlasMax)
    {
        auto& sliceComponent           = m_ecs.AddComponent<NineSliceComponent>(entity);
        sliceComponent.cornerSize      = cornerSize;
        sliceComponent.atlasSize       = atlasSize;
        sliceComponent.cornerAtlasSize = cornerAtlasSize;
        sliceComponent.atlasMin        = atlasMin;
        sliceComponent.atlasMax        = atlasMax;
    }

    void UI2DSystem::RegenerateNineSliceGeometry(Entity entity)
    {
        auto& transformComponent  = m_ecs.GetComponent<TransformComponent>(entity);
        auto& sliceComponent      = m_ecs.GetComponent<NineSliceComponent>(entity);
        auto& renderableComponent = m_ecs.GetComponent<RenderableComponent>(entity);

        auto& geometry = *renderableComponent.geometry;

        GeometryUtils::RegenerateUINineSliceGeometry(reinterpret_cast<Vertex2D*>(geometry.vertices),
                                                     u16vec2(transformComponent.width, transformComponent.height),
                                                     sliceComponent.cornerSize, sliceComponent.atlasSize, sliceComponent.cornerAtlasSize,
                                                     sliceComponent.atlasMin, sliceComponent.atlasMax);

        Renderer.UpdateGeometryVertices(geometry, 0, geometry.vertexCount, geometry.vertices);
    }

    bool UI2DSystem::SetupRenderableComponent(Entity entity, const UIGeometryConfig& config)
    {
        auto& renderableComponent = m_ecs.AddComponent<RenderableComponent>(entity);
        renderableComponent.depth = 1;

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

    void UI2DSystem::SetupClickableComponent(Entity entity, u16 width, u16 height)
    {
        auto& clickableComponent         = m_ecs.AddComponent<ClickableComponent>(entity);
        clickableComponent.bounds.x      = 0.0f;
        clickableComponent.bounds.y      = 0.0f;
        clickableComponent.bounds.width  = width;
        clickableComponent.bounds.height = height;
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
