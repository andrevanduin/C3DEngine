
#include "component.h"

#include "renderer/renderer_frontend.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/input/input_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/transforms/transform_system.h"

namespace C3D::UI_2D
{
    bool Component::Initialize(ComponentType _type, const Config& config)
    {
        m_transform = Transforms.Acquire(vec3(config.position.x, config.position.y, 0));
        m_bounds    = Bounds(0.0f, 0.0f, config.size.x, config.size.y);
        m_flags |= FlagVisible;
        type = _type;
        return onInitialize(*this, config);
    }

    void Component::Destroy(const DynamicAllocator* pAllocator)
    {
        // Call the implementation specific destroy method
        onDestroy(*this, pAllocator);
        // Destroy our user handlers struct if it's allocated
        DestroyUserHandlers(pAllocator);
    }

    void Component::MakeUserHandlers(const DynamicAllocator* pAllocator)
    {
        if (!pUserHandlers)
        {
            pUserHandlers = pAllocator->New<UserHandlers>(MemoryType::UI);
        }
    }

    void Component::DestroyUserHandlers(const DynamicAllocator* pAllocator)
    {
        if (pUserHandlers)
        {
            pAllocator->Delete(pUserHandlers);
            pUserHandlers = nullptr;
        }
    }

    bool Component::IsFlagSet(FlagBit flag) const { return m_flags & flag; }

    void Component::SetFlag(FlagBit flag) { m_flags |= flag; }

    void Component::RemoveFlag(FlagBit flag) { m_flags &= ~flag; }

    void Component::ToggleFlag(FlagBit flag)
    {
        if (m_flags & flag)
        {
            m_flags &= ~flag;
        }
        else
        {
            m_flags |= flag;
        }
    }

    vec2 Component::GetPosition() const
    {
        const auto& pos = Transforms.GetPosition(m_transform);
        return { pos.x, pos.y };
    }

    f32 Component::GetX() const
    {
        const auto& pos = Transforms.GetPosition(m_transform);
        return pos.x;
    }

    f32 Component::GetY() const
    {
        const auto& pos = Transforms.GetPosition(m_transform);
        return pos.y;
    }

    u16 Component::GetWidth() const { return m_bounds.width; }

    void Component::SetWidth(u16 width)
    {
        m_bounds.width = width;
        if (onResize) onResize(*this);
    };

    u16 Component::GetHeight() const { return m_bounds.height; }

    void Component::SetHeight(u16 height)
    {
        m_bounds.height = height;
        if (onResize) onResize(*this);
    }

    u16vec2 Component::GetSize() const { return u16vec2(m_bounds.width, m_bounds.height); }

    void Component::SetSize(const u16vec2& size)
    {
        m_bounds.width  = size.x;
        m_bounds.height = size.y;
        if (onResize) onResize(*this);
    }

    bool Component::Contains(const vec2& point) const { return m_bounds.Contains(point); }

    bool Component::operator==(const Component& other) const { return uuid == other.uuid; }
    bool Component::operator!=(const Component& other) const { return uuid != other.uuid; }

}  // namespace C3D::UI_2D