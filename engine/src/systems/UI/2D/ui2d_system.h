
#pragma once
#include "UI/2D/component.h"
#include "UI/2D/config.h"
#include "UI/2D/internal/ui_pass.h"
#include "UI/2D/ui2d_defines.h"
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "core/defines.h"
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
        void Prepare(Viewport* viewport);

        bool OnUpdate(const FrameData& frameData) override;

        /* ------ Components Start ------ */

        Handle AddPanel(const UI_2D::Config& config);
        Handle AddButton(const UI_2D::Config& config);
        Handle AddLabel(const UI_2D::Config& config);
        Handle AddTextbox(const UI_2D::Config& config);

        /* ------- Components End -------  */

        /* ------ Generic Handlers Start ------ */

        bool MakeVisible(Handle handle, bool visible);

        bool ToggleVisible(Handle handle);

        bool SetParent(Handle child, Handle parent);

        vec2 GetPosition(Handle handle) const;
        bool SetPosition(Handle handle, const vec2& position);

        vec2 GetSize(Handle handle) const;
        bool SetSize(Handle handle, u16 width, u16 height);

        u16 GetWidth(Handle handle) const;
        bool SetWidth(Handle handle, u16 width);

        u16 GetHeight(Handle handle) const;
        bool SetHeight(Handle handle, u16 height);

        f32 GetRotation(Handle handle) const;
        bool SetRotation(Handle handle, f32 angle);

        /**
         * @brief Sets the provided component to active or inactive.
         * Only one component at the time can be active. Calling this method on another component while there is already a component active
         * will set the previously active componet to inactive.
         *
         * @param handle A handle to the component that you want to change activity for
         * @param active A boolean inidicating if you want active(true) or inactive(false)
         * @return True if successful; false otherwise
         */
        bool SetActive(Handle Handle, bool active);

        /**
         * @brief Add an OnClickHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnClick handler to
         * @param handler The handler function that needs to be called on a click
         * @return True if successful; false otherwise
         */
        bool AddOnClickHandler(Handle handle, const UI_2D::OnClickEventHandler& handler);

        /**
         * @brief Add an OnHoverStartHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnHoverStart handler to
         * @param handler The handler function that needs to be called when hover starts
         * @return True if successful; false otherwise
         */
        bool AddOnHoverStartHandler(Handle handle, const UI_2D::OnHoverStartEventHandler& handler);

        /**
         * @brief Add an OnHoverEndHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnHoverEnd handler to
         * @param handler The handler function that needs to be called when hover ends
         * @return True if successful; false otherwise
         */
        bool AddOnHoverEndHandler(Handle handle, const UI_2D::OnHoverEndEventHandler& handler);

        /**
         * @brief Add an OnEndTextInputHandler to the provided Entity.
         *
         * @param entity The entity that you want to add the OnEndTextInput handler to
         * @param handler The handler function that needs to be called when text input ends
         * @return True if successful; false otherwise
         */
        bool AddOnEndTextInputHandler(Handle handle, const UI_2D::OnEndTextInputEventHandler& handler);

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
        bool SetText(Handle handle, const char* text);
        bool SetText(Handle handle, const String& text);

        u16 GetTextMaxX(Handle handle) const;
        u16 GetTextMaxY(Handle handle) const;

        const UI_2D::AtlasDescriptions& GetAtlasDescriptions(UI_2D::AtlasID id) const;

        Shader& GetShader();
        TextureMap& GetAtlas();

        /* ------- Generic Handlers End -------  */

        UI2DPass* GetPass() { return &m_pass; }

        void OnShutdown() override;

    private:
        UI_2D::Component& GetComponent(Handle handle);
        const UI_2D::Component& GetComponent(Handle handle) const;

        Handle SetComponent(const UI_2D::Component& component);

        bool SetActive(UI_2D::Component& component, bool active);

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

        UI2DPass m_pass;
        Shader* m_shader;

        HashMap<UUID, u32> m_componentMap;
        DynamicArray<UI_2D::Component> m_components;
        UI_2D::Component* m_pActiveComponent;

        TextureMap m_textureAtlas;

        DynamicArray<RegisteredEventCallback> m_callbacks;

        UI_2D::AtlasDescriptions m_atlasBank[UI_2D::AtlasIDMax];
    };
}  // namespace C3D
