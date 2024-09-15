
#pragma once
#include "UI/2D/component.h"
#include "UI/2D/config.h"
#include "UI/2D/internal/ui_pass.h"
#include "UI/2D/ui2d_defines.h"
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "defines.h"
#include "memory/allocators/dynamic_allocator.h"
#include "renderer/rendergraph/renderpass.h"
#include "resources/geometry_config.h"
#include "resources/textures/texture_map.h"
#include "systems/events/event_context.h"
#include "systems/events/event_system.h"
#include "systems/system.h"

namespace C3D
{
    class Shader;
    class Viewport;
    class Camera;

    struct UI2DSystemConfig
    {
        u64 maxControls = 1024;
        u64 memorySize  = MebiBytes(8);
    };

    class C3D_API UI2DSystem final : public SystemWithConfig<UI2DSystemConfig>
    {
    public:
        bool OnInit(const CSONObject& config) override;

        bool OnRun();

        bool OnUpdate(const FrameData& frameData) override;
        bool OnPrepareRender(FrameData& frameData) override;

        /* ------ Components Start ------ */

        Handle<UI_2D::Component> AddPanel(const UI_2D::Config& config);
        Handle<UI_2D::Component> AddButton(const UI_2D::Config& config);
        Handle<UI_2D::Component> AddLabel(const UI_2D::Config& config);
        Handle<UI_2D::Component> AddTextbox(const UI_2D::Config& config);

        /* ------- Components End -------  */

        /* ------ Generic Handlers Start ------ */

        bool MakeVisible(Handle<UI_2D::Component> handle, bool visible);

        bool ToggleVisible(Handle<UI_2D::Component> handle);

        bool SetParent(Handle<UI_2D::Component> child, Handle<UI_2D::Component> parent);

        vec2 GetPosition(Handle<UI_2D::Component> handle) const;
        bool SetPosition(Handle<UI_2D::Component> handle, const vec2& position);

        vec2 GetSize(Handle<UI_2D::Component> handle) const;
        bool SetSize(Handle<UI_2D::Component> handle, u16 width, u16 height);

        u16 GetWidth(Handle<UI_2D::Component> handle) const;
        bool SetWidth(Handle<UI_2D::Component> handle, u16 width);

        u16 GetHeight(Handle<UI_2D::Component> handle) const;
        bool SetHeight(Handle<UI_2D::Component> handle, u16 height);

        f32 GetRotation(Handle<UI_2D::Component> handle) const;
        bool SetRotation(Handle<UI_2D::Component> handle, f32 angle);

        /**
         * @brief Sets the provided component to active or inactive.
         * Only one component at the time can be active. Calling this method on another component while there is already a component active
         * will set the previously active componet to inactive.
         *
         * @param handle A handle to the component that you want to change activity for
         * @param active A boolean inidicating if you want active(true) or inactive(false)
         * @return True if successful; false otherwise
         */
        bool SetActive(Handle<UI_2D::Component> Handle, bool active);

        /**
         * @brief Add an OnClickHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnClick handler to
         * @param handler The handler function that needs to be called on a click
         * @return True if successful; false otherwise
         */
        bool AddOnClickHandler(Handle<UI_2D::Component> handle, const UI_2D::OnClickEventHandler& handler);

        /**
         * @brief Add an OnHoverStartHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnHoverStart handler to
         * @param handler The handler function that needs to be called when hover starts
         * @return True if successful; false otherwise
         */
        bool AddOnHoverStartHandler(Handle<UI_2D::Component> handle, const UI_2D::OnHoverStartEventHandler& handler);

        /**
         * @brief Add an OnHoverEndHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnHoverEnd handler to
         * @param handler The handler function that needs to be called when hover ends
         * @return True if successful; false otherwise
         */
        bool AddOnHoverEndHandler(Handle<UI_2D::Component> handle, const UI_2D::OnHoverEndEventHandler& handler);

        /**
         * @brief Add an OnEndTextInputHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnEndTextInput handler to
         * @param handler The handler function that needs to be called when text input ends
         * @return True if successful; false otherwise
         */
        bool AddOnEndTextInputHandler(Handle<UI_2D::Component> handle, const UI_2D::OnEndTextInputEventHandler& handler);

        /**
         * @brief Add an OnFlagsChangedEventHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnFlagsChanged handler to
         * @param handler The handler function that needs to be called when flags change
         */
        // bool AddOnFlagsChangedHandler(Handle handle, const UI_2D::OnFlagsChangedEventHandler& handler);

        /**
         * @brief Sets the text for the provided entity.
         *
         * @param entity The entity that you want to set the text for
         * @param text The text you want to set
         * @return True if successful; false otherwise
         */
        bool SetText(Handle<UI_2D::Component> handle, const char* text);
        bool SetText(Handle<UI_2D::Component> handle, const String& text);

        u16 GetTextMaxX(Handle<UI_2D::Component> handle) const;
        u16 GetTextMaxY(Handle<UI_2D::Component> handle) const;

        const UI_2D::AtlasDescriptions& GetAtlasDescriptions(UI_2D::AtlasID id) const;

        Shader& GetShader();
        TextureMap& GetAtlas();

        /* ------- Generic Handlers End -------  */

        const UI_2D::Component& GetComponent(Handle<UI_2D::Component> handle) { return m_components[handle.index]; }
        UI_2D::Component* GetComponents() const { return m_components; }
        u32 GetNumberOfComponents() const { return m_componentIndexMax; }

        void OnShutdown() override;

        bool SetActive(UI_2D::Component& component, bool active);

    private:
        Handle<UI_2D::Component> MakeHandle(UI_2D::Component&& component);

        /** @brief Handles OnClick events for all the components managed by the UI2D System.
         * @return True if OnClick event is handled; false otherwise
         */
        bool OnClick(const EventContext& context);

        /** @brief Handles OnMouseMoved events for all components managed by the UI2D System. */
        bool OnMouseMoved(const EventContext& context);

        /** @brief Handles OnKeyDown events for all components managed by the UI2D System. */
        bool OnKeyDown(const EventContext& context);

        // void RegenerateNineSliceGeometry(Entity entity);

        // bool KeyDownTextInput(Entity entity, const UI_2D::KeyEventContext& ctx);

        DynamicAllocator m_allocator;
        void* m_memoryBlock = nullptr;

        Shader* m_shader = nullptr;

        HierarchyGraph m_graph;

        u32 m_componentIndexMax              = 0;
        UI_2D::Component* m_components       = nullptr;
        UI_2D::Component* m_pActiveComponent = nullptr;

        TextureMap m_textureAtlas;

        DynamicArray<RegisteredEventCallback> m_callbacks;

        UI_2D::AtlasDescriptions m_atlasBank[UI_2D::AtlasIDMax];
    };
}  // namespace C3D
