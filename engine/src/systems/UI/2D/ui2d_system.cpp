#include "ui2d_system.h"

#include "UI/2D/button.h"
#include "UI/2D/label.h"
#include "UI/2D/panel.h"
#include "UI/2D/textbox.h"
#include "core/ecs/entity_view.h"
#include "core/frame_data.h"
#include "core/uuid.h"
#include "math/geometry_utils.h"
#include "resources/geometry_config.h"
#include "resources/shaders/shader.h"
#include "systems/fonts/font_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/textures/texture_system.h"

#define ASSERT_VALID(handle)                        \
    if (!handle.IsValid())                          \
    {                                               \
        ERROR_LOG("Handle provided is not valid."); \
        return false;                               \
    }

namespace C3D
{
    using namespace UI_2D;

    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr const char* SHADER_NAME   = "Shader.Builtin.UI2D";
    constexpr u16vec2 atlasSize         = u16vec2(512, 512);  // Size of the entire UI atlas

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

        if (config.memorySize < MebiBytes(8))
        {
            ERROR_LOG("UI2D requires at least 8 MebiBytes of memory.");
            return false;
        }

        // Allocate enough space for our control allocator
        u64 neededMemory = DynamicAllocator::GetMemoryRequirements(config.memorySize);

        // Create our own dynamic allocator
        m_memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);
        if (!m_allocator.Create(m_memoryBlock, neededMemory, config.memorySize))
        {
            ERROR_LOG("Failed to create Dynamic allocator.");
            return false;
        }

        // Use our own allocator to allocate components
        m_components.SetAllocator(&m_allocator);
        m_components.Reserve(512);

        // Create our hashmap
        m_componentMap.Create(1024);

        m_pass = UI2DPass();

        m_callbacks.PushBack(Event.Register(
            EventCodeButtonClicked, [this](const u16 code, void* sender, const C3D::EventContext& context) { return OnClick(context); }));
        m_callbacks.PushBack(Event.Register(
            EventCodeMouseMoved, [this](const u16 code, void* sender, const C3D::EventContext& context) { return OnMouseMoved(context); }));
        m_callbacks.PushBack(Event.Register(
            EventCodeKeyDown, [this](const u16 code, void* sender, const C3D::EventContext& context) { return OnKeyDown(context); }));

        // TODO: Get from config

        // Panel configuration
        auto& panelAtlasses = m_atlasBank[AtlasIDPanel];
        // Default
        panelAtlasses.defaultMin = u16vec2(0, 0);
        panelAtlasses.defaultMax = u16vec2(8, 8);
        // Size
        panelAtlasses.size       = u16vec2(512, 512);
        panelAtlasses.cornerSize = u16vec2(1, 1);

        // Button configuration
        auto& buttonAtlasses = m_atlasBank[AtlasIDButton];
        // Default
        buttonAtlasses.defaultMin = u16vec2(96, 0);
        buttonAtlasses.defaultMax = u16vec2(112, 17);
        // Hover
        buttonAtlasses.hoverMin = u16vec2(96, 18);
        buttonAtlasses.hoverMax = u16vec2(112, 35);
        // Size
        buttonAtlasses.size       = u16vec2(512, 512);
        buttonAtlasses.cornerSize = u16vec2(8, 8);

        // Textbox configuration
        auto& textboxAtlasses = m_atlasBank[AtlasIDTextboxNineSlice];
        // Default
        textboxAtlasses.defaultMin = u16vec2(0, 32);
        textboxAtlasses.defaultMax = u16vec2(3, 35);
        // Active
        textboxAtlasses.activeMin = u16vec2(3, 32);
        textboxAtlasses.activeMax = u16vec2(6, 35);
        // Size
        textboxAtlasses.size       = u16vec2(512, 512);
        textboxAtlasses.cornerSize = u16vec2(1, 1);

        auto& textboxCursorAtlasses = m_atlasBank[AtlasIDTextboxCursor];
        // Default
        textboxCursorAtlasses.defaultMin = u16vec2(0, 35);
        textboxCursorAtlasses.defaultMax = u16vec2(1, 36);
        // Size
        textboxCursorAtlasses.size = u16vec2(512, 512);

        auto& textboxHighlightAtlasses = m_atlasBank[AtlasIDTextboxHighlight];
        // Default
        textboxHighlightAtlasses.defaultMin = u16vec2(1, 35);
        textboxHighlightAtlasses.defaultMax = u16vec2(2, 36);
        // Size
        textboxHighlightAtlasses.size = u16vec2(512, 512);

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

    void UI2DSystem::Prepare(Viewport* viewport) { m_pass.Prepare(viewport, &m_components); }

    bool UI2DSystem::OnUpdate(const FrameData& frameData)
    {
        for (auto& component : m_components)
        {
            if (component.onUpdate)
            {
                component.onUpdate(component);
            }
        }
        return true;
    }

    ComponentHandle UI2DSystem::AddPanel(const Config& config)
    {
        auto panel = Panel::Create(&m_allocator);
        if (!panel.Initialize(ComponentTypePanel, config))
        {
            ERROR_LOG("Failed to Initialize Panel.");
            return UUID::Invalid();
        }
        return SetComponent(panel);
    }

    ComponentHandle UI2DSystem::AddButton(const Config& config)
    {
        auto button = Button::Create(&m_allocator);
        if (!button.Initialize(ComponentTypeButton, config))
        {
            ERROR_LOG("Failed to Initialize Button.");
            return UUID::Invalid();
        }
        return SetComponent(button);
    }

    Handle UI2DSystem::AddLabel(const Config& config)
    {
        auto label = Label::Create(&m_allocator);
        if (!label.Initialize(ComponentTypeLabel, config))
        {
            ERROR_LOG("Failed to Initialize Label.");
            return UUID::Invalid();
        }
        return SetComponent(label);
    }

    Handle UI2DSystem::AddTextbox(const Config& config)
    {
        auto textbox = Textbox::Create(&m_allocator);
        if (!textbox.Initialize(ComponentTypeTextbox, config))
        {
            ERROR_LOG("Failed to Initialize Textbox.");
            return UUID::Invalid();
        }
        return SetComponent(textbox);
    }

    bool UI2DSystem::MakeVisible(Handle handle, bool visible)
    {
        ASSERT_VALID(handle);
        auto& component = GetComponent(handle);

        if (visible)
        {
            component.SetFlag(FlagVisible);
        }
        else
        {
            component.RemoveFlag(FlagVisible);
        }

        return true;
    }

    bool UI2DSystem::ToggleVisible(Handle handle)
    {
        ASSERT_VALID(handle);
        auto& component = GetComponent(handle);
        component.ToggleFlag(FlagVisible);
        return true;
    }

    bool UI2DSystem::SetParent(Handle childHandle, Handle parentHandle)
    {
        ASSERT_VALID(childHandle);
        ASSERT_VALID(parentHandle);

        auto& child  = GetComponent(childHandle);
        auto& parent = GetComponent(parentHandle);

        if (!parent.AddChild(child))
        {
            ERROR_LOG("Failed to add component: {} as child of: {}.", childHandle, parentHandle);
            return false;
        }

        if (!child.AddParent(parent))
        {
            ERROR_LOG("Failed to add component: {} as parent of: {}.", parentHandle, childHandle);
            return false;
        }

        return true;
    }

    vec2 UI2DSystem::GetPosition(ComponentHandle handle) const
    {
        if (handle.IsValid())
        {
            auto& component = GetComponent(handle);
            return component.GetPosition();
        }
        return VEC2_ZERO;
    }

    bool UI2DSystem::SetPosition(ComponentHandle handle, const vec2& position)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        component.SetPosition(position);

        return true;
    }

    vec2 UI2DSystem::GetSize(ComponentHandle handle) const
    {
        if (handle.IsValid())
        {
            auto& component = GetComponent(handle);
            return component.GetSize();
        }

        ERROR_LOG("Invalid component. Returning vec2(0, 0).");
        return VEC2_ZERO;
    }

    bool UI2DSystem::SetSize(ComponentHandle handle, u16 width, u16 height)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        component.SetSize(vec2(width, height));
        return true;
    }

    u16 UI2DSystem::GetWidth(Handle handle) const
    {
        if (handle.IsValid())
        {
            auto& component = GetComponent(handle);
            return component.GetWidth();
        }

        ERROR_LOG("Invalid component. Returning 0.");
        return 0;
    }

    bool UI2DSystem::SetWidth(ComponentHandle handle, u16 width)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        component.SetWidth(width);

        return true;
    }

    u16 UI2DSystem::GetHeight(Handle handle) const
    {
        if (handle.IsValid())
        {
            auto& component = GetComponent(handle);
            return component.GetHeight();
        }

        ERROR_LOG("Invalid component. Returning 0.");
        return 0;
    }

    bool UI2DSystem::SetHeight(ComponentHandle handle, u16 height)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        component.SetHeight(height);

        return true;
    }

    f32 UI2DSystem::GetRotation(ComponentHandle handle) const
    {
        if (handle.IsValid())
        {
            FATAL_LOG("Not implemented yet!");
        }
        return 0;
    }

    bool UI2DSystem::SetRotation(ComponentHandle handle, f32 angle)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);

        auto rotation = glm::rotate(quat(1.0f, 0.0f, 0.0f, 0.0f), angle, vec3(0.0f, 0.0f, 1.0f));
        component.SetRotation(rotation);

        return true;
    }

    bool UI2DSystem::SetActive(ComponentHandle handle, bool active)
    {
        ASSERT_VALID(handle);
        auto& component = GetComponent(handle);
        return SetActive(component, active);
    }

    bool UI2DSystem::SetActive(Component& component, bool active)
    {
        if (active)
        {
            component.SetFlag(FlagActive);

            // TODO: Set nine slice to active if it's needed

            if (m_pActiveComponent)
            {
                // We have an active component already. So let's deactive it since we can only have one at once.
                m_pActiveComponent->RemoveFlag(FlagActive);
            }

            // Set the component we just activated as our current active component
            m_pActiveComponent = &component;
        }
        else
        {
            component.RemoveFlag(FlagActive);

            // TODO: Set nine slice to inactive if it's needed

            // Only if the provided component is actually the active component we unset our active component
            if (m_pActiveComponent && component == *m_pActiveComponent)
            {
                m_pActiveComponent = nullptr;
            }
        }

        return true;
    }

    bool UI2DSystem::AddOnClickHandler(Handle handle, const UI_2D::OnClickEventHandler& handler)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        if (!component.pUserHandlers)
        {
            INFO_LOG("Component: {} did not have any User defined handlers yet. Allocating memory for the handlers first.", handle);
            component.MakeUserHandlers(&m_allocator);
        }

        component.pUserHandlers->onClickHandler = handler;
        return true;
    }

    bool UI2DSystem::AddOnHoverStartHandler(Handle handle, const UI_2D::OnHoverStartEventHandler& handler)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        if (!component.pUserHandlers)
        {
            INFO_LOG("Component: {} did not have any User defined handlers yet. Allocating memory for the handlers first.", handle);
            component.MakeUserHandlers(&m_allocator);
        }

        component.pUserHandlers->onHoverStartHandler = handler;
        return true;
    }

    bool UI2DSystem::AddOnHoverEndHandler(Handle handle, const UI_2D::OnHoverEndEventHandler& handler)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        if (!component.pUserHandlers)
        {
            INFO_LOG("Component: {} did not have any User defined handlers yet. Allocating memory for the handlers first.", handle);
            component.MakeUserHandlers(&m_allocator);
        }

        component.pUserHandlers->onHoverEndHandler = handler;
        return true;
    }

    bool UI2DSystem::AddOnEndTextInputHandler(Handle handle, const UI_2D::OnEndTextInputEventHandler& handler)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        if (!component.pUserHandlers)
        {
            INFO_LOG("Component: {} did not have any User defined handlers yet. Allocating memory for the handlers first.", handle);
            component.MakeUserHandlers(&m_allocator);
        }

        component.pUserHandlers->onTextInputEndHandler = handler;
        return true;
    }

    // bool UI2DSystem::AddOnFlagsChangedHandler(Handle handle, const UI_2D::OnFlagsChangedEventHandler& handler) {}

    bool UI2DSystem::SetText(Handle handle, const char* text)
    {
        ASSERT_VALID(handle);

        auto& component = GetComponent(handle);
        switch (component.type)
        {
            case ComponentTypeLabel:
            {
                auto& data = component.GetInternal<Label::InternalData>();
                data.textComponent.SetText(component, text);
            }
            break;
            case ComponentTypeTextbox:
                Textbox::SetText(component, text);
                break;
        }

        return true;
    }

    bool UI2DSystem::SetText(Handle handle, const String& text) { return SetText(handle, text.Data()); }

    u16 UI2DSystem::GetTextMaxX(Handle handle) const
    {
        auto& component = GetComponent(handle);
        switch (component.type)
        {
            case ComponentTypeTextbox:
            {
                auto& data = component.GetInternal<Textbox::InternalData>();
                return data.textComponent.maxX;
            }
            case ComponentTypeLabel:
            {
                auto& data = component.GetInternal<Label::InternalData>();
                return data.textComponent.maxX;
            }
        }

        ERROR_LOG("Tried to get TextMaxX for component that does not have it.");
        return 0;
    }

    u16 UI2DSystem::GetTextMaxY(Handle handle) const
    {
        auto& component = GetComponent(handle);
        switch (component.type)
        {
            case ComponentTypeTextbox:
            {
                auto& data = component.GetInternal<Textbox::InternalData>();
                return data.textComponent.maxY;
            }
            case ComponentTypeLabel:
            {
                auto& data = component.GetInternal<Label::InternalData>();
                return data.textComponent.maxY;
            }
        }

        ERROR_LOG("Tried to get TextMaxY for component that does not have it.");
        return 0;
    }

    UI_2D::Component& UI2DSystem::GetComponent(ComponentHandle handle)
    {
        auto index = m_componentMap.Get(handle);
        return m_components[index];
    }

    const UI_2D::Component& UI2DSystem::GetComponent(Handle handle) const
    {
        auto index = m_componentMap.Get(handle);
        return m_components[index];
    }

    Handle UI2DSystem::SetComponent(const Component& component)
    {
        auto id = component.GetID();

        u32 i = 0;
        for (; i < m_components.Size(); i++)
        {
            if (!m_components[i].IsValid())
            {
                m_components[i] = component;
                m_componentMap.Set(id, i);
                break;
            }
        }

        if (i == m_components.Size())
        {
            // We did not find an empty spot in the array so let's append
            m_componentMap.Set(id, m_components.Size());
            m_components.PushBack(component);
        }

        return id;
    }

    bool UI2DSystem::OnClick(const EventContext& context)
    {
        auto ctx = MouseButtonEventContext(context.data.i16[0], context.data.i16[1], context.data.i16[2]);

        for (auto& component : m_components)
        {
            if (component.onClick)
            {
                // This component handles onClick events
                auto model          = component.GetWorld();
                auto inverse        = glm::inverse(model);
                auto transformedPos = inverse * vec4(static_cast<f32>(ctx.x), static_cast<f32>(ctx.y), 0.0f, 1.0f);

                if (component.Contains(vec2(transformedPos.x, transformedPos.y)))
                {
                    // Set this entity to active to indicate that this is our currently active entity
                    return component.onClick(component, ctx);
                }
            }
        }

        // We clicked, but none of our component were hit. So we should unset our currently active component since we did not click any
        if (m_pActiveComponent)
        {
            SetActive(*m_pActiveComponent, false);
        }

        // Let's return false since if we reach this point the click event was not handled yet
        return false;
    }

    bool UI2DSystem::OnMouseMoved(const EventContext& context)
    {
        auto ctx = OnHoverEventContext(context.data.u16[0], context.data.u16[1]);

        for (auto& component : m_components)
        {
            if (component.onHoverStart && component.onHoverEnd)
            {
                // This component handles onHoverStart and onHoverEnd events
                auto inverseModel   = glm::inverse(component.GetWorld());
                auto transformedPos = inverseModel * vec4(ctx.x, ctx.y, 0.0f, 1.0f);

                if (component.IsFlagSet(FlagHovered))
                {
                    // We are already hovering this component. Let's check if we are moving out now.
                    if (!component.Contains(transformedPos))
                    {
                        // We have stopped hovering this component
                        component.RemoveFlag(FlagHovered);
                        return component.onHoverEnd(component, ctx);
                    }
                }
                // Otherwise we check if we started hovering
                else if (component.Contains(transformedPos))
                {
                    // We have started hovering this component
                    component.SetFlag(FlagHovered);
                    return component.onHoverStart(component, ctx);
                }
            }
        }

        // Return false to let other mouse moved handlers potentially handle this hover event
        return false;
    }

    bool UI2DSystem::OnKeyDown(const EventContext& context)
    {
        auto ctx = KeyEventContext(context.data.u16[0]);

        // We only handle key down events for the active component
        if (m_pActiveComponent && m_pActiveComponent->onKeyDown)
        {
            return m_pActiveComponent->onKeyDown(*m_pActiveComponent, ctx);
        }

        return false;
    }

    const AtlasDescriptions& UI2DSystem::GetAtlasDescriptions(AtlasID id) const { return m_atlasBank[id]; }

    Shader& UI2DSystem::GetShader() { return *m_shader; }

    TextureMap& UI2DSystem::GetAtlas() { return m_textureAtlas; }

    void UI2DSystem::OnShutdown()
    {
        INFO_LOG("Shutting down.");

        for (auto cb : m_callbacks)
        {
            Event.Unregister(cb);
        }

        for (auto& component : m_components)
        {
            component.Destroy(&m_allocator);
        }
        m_components.Destroy();
        m_pActiveComponent = nullptr;

        m_componentMap.Destroy();

        if (m_textureAtlas.texture)
        {
            Textures.Release(m_textureAtlas.texture->name);
            m_textureAtlas.texture = nullptr;
        }

        Renderer.ReleaseTextureMapResources(m_textureAtlas);

        if (!m_allocator.Destroy())
        {
            ERROR_LOG("Failed to destroy allocator.");
        }
        Memory.Free(m_memoryBlock);
    }
}  // namespace C3D
