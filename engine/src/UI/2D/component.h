
#pragma once
#include "config.h"
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"
#include "core/uuid.h"
#include "internal/handlers.h"
#include "renderer/geometry.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"
#include "resources/font.h"
#include "systems/system_manager.h"
#include "ui2d_defines.h"

namespace C3D::UI_2D
{
    class Component;

    /** @brief Function pointers to general funcitonality. */
    using OnInitializeFunc = bool (*)(Component& self, const Config& config);
    using OnDestroyFunc    = void (*)(Component& self, const DynamicAllocator* pAllocator);

    using OnUpdateFunc = void (*)(Component& self);
    using OnRenderFunc = void (*)(Component& self, const FrameData& frameData, const ShaderLocations& locations);
    using OnResizeFunc = void (*)(Component& self);

    /** @brief Function pointers for handling hover. */
    using OnHoverStartFunc = bool (*)(Component& self, const OnHoverEventContext& ctx);
    using OnHoverEndFunc   = bool (*)(Component& self, const OnHoverEventContext& ctx);

    /** @brief Function pointers for handling clicking. */
    using OnMouseDownFunc = bool (*)(Component& self, const MouseButtonEventContext& ctx);
    using OnMoseUpFunc    = bool (*)(Component& self, const MouseButtonEventContext& ctx);
    using OnClickFunc     = bool (*)(Component& self, const MouseButtonEventContext& ctx);

    /** @brief Function pointers for handling keys. */
    using OnKeyDownFunc = bool (*)(Component& self, const KeyEventContext& ctx);

    using ComponentHandle = UUID;

    class Component
    {
    public:
        Component();

        bool Initialize(ComponentType _type, const Config& config);
        void Destroy(const DynamicAllocator* pAllocator);

        bool AddChild(Component& child);
        bool AddParent(Component& parent);

        void MakeUserHandlers(const DynamicAllocator* pAllocator);
        void DestroyUserHandlers(const DynamicAllocator* pAllocator);

        UUID GetID() const;
        const Transform& GetTransform() const;
        mat4 GetWorld() const;

        bool IsValid() const;

        bool IsFlagSet(FlagBit flag) const;
        void SetFlag(FlagBit flag);
        void RemoveFlag(FlagBit flag);
        void ToggleFlag(FlagBit flag);

        f32 GetX() const;
        void SetX(f32 x);

        f32 GetY() const;
        void SetY(f32 y);

        vec2 GetPosition() const;
        void SetPosition(const u16vec2& position);

        u16 GetWidth() const;
        void SetWidth(u16 width);

        u16 GetHeight() const;
        void SetHeight(u16 height);

        u16vec2 GetSize() const;
        void SetSize(const u16vec2& size);

        void SetRotation(const quat& rotation);

        bool Contains(const vec2& point) const;

        template <typename T>
        void MakeInternal(const DynamicAllocator* pAllocator)
        {
            m_pImplData = pAllocator->New<T>(MemoryType::UI);
        }

        template <typename T>
        T& GetInternal() const
        {
            return *static_cast<T*>(m_pImplData);
        }

        void DestroyInternal(const DynamicAllocator* pAllocator)
        {
            if (m_pImplData)
            {
                pAllocator->Delete(m_pImplData);
                m_pImplData = nullptr;
            }
        }

        bool operator==(const Component& other) const;
        bool operator!=(const Component& other) const;

        ComponentType type = ComponentTypeNone;

        // Component implementation specific methods
        OnUpdateFunc onUpdate = nullptr;
        OnRenderFunc onRender = nullptr;
        OnResizeFunc onResize = nullptr;

        OnHoverStartFunc onHoverStart = nullptr;
        OnHoverEndFunc onHoverEnd     = nullptr;

        OnMouseDownFunc onMouseDown = nullptr;
        OnMoseUpFunc onMouseUp      = nullptr;
        OnClickFunc onClick         = nullptr;

        OnKeyDownFunc onKeyDown = nullptr;

        OnInitializeFunc onInitialize = nullptr;
        OnDestroyFunc onDestroy       = nullptr;

        UserHandlers* pUserHandlers = nullptr;

    private:
        UUID m_id;
        Flags m_flags = FlagNone;

        Transform m_transform;
        Bounds m_bounds;

        ComponentHandle m_parent;
        DynamicArray<ComponentHandle> m_children;

        void* m_pImplData = nullptr;
    };
}  // namespace C3D::UI_2D