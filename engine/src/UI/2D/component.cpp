
#include "component.h"

#include "platform/platform.h"
#include "renderer/renderer_frontend.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/input/input_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_COMPONENT";

    Component::Component(const SystemManager* systemsManager) : m_pSystemsManager(systemsManager) { m_id.Generate(); }

    bool Component::Initialize(ComponentType _type, const Config& config)
    {
        m_transform = Transform();
        m_transform.SetPosition(vec3(config.position.x, config.position.y, 0));
        m_bounds = Bounds(0.0f, 0.0f, config.size.x, config.size.y);
        m_flags |= FlagVisible;
        type = _type;
        return onInitialize(*this, config);
    }

    void Component::Destroy(const DynamicAllocator* pAllocator)
    {
        m_id.Invalidate();
        // Call the implementation specific destroy method
        onDestroy(*this, pAllocator);
        // Destroy our user handlers struct if it's allocated
        DestroyUserHandlers(pAllocator);
    }

    bool Component::AddChild(Component& child)
    {
        auto childHandle = child.GetID();
        if (std::find(m_children.begin(), m_children.end(), childHandle) != m_children.end())
        {
            ERROR_LOG("Component: {} is already a child of Component: {}.", child.GetID(), m_id);
        }
        m_children.PushBack(childHandle);
        return true;
    }

    bool Component::AddParent(Component& parent)
    {
        if (m_parent.IsValid())
        {
            ERROR_LOG("Component: {} already has a parent.", m_id);
            return false;
        }

        m_parent = parent.GetID();
        m_transform.SetParent(&parent.GetTransform());

        return true;
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

    UUID Component::GetID() const { return m_id; }

    const Transform& Component::GetTransform() const { return m_transform; }

    mat4 Component::GetWorld() const { return m_transform.GetWorld(); }

    bool Component::IsValid() const { return m_id.IsValid(); }

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
        auto& position = m_transform.GetPosition();
        return { position.x, position.y };
    }

    void Component::SetPosition(const u16vec2& position) { m_transform.SetPosition(vec3(position, 0.0f)); }

    f32 Component::GetX() const { return m_transform.GetX(); }

    void Component::SetX(f32 x) { m_transform.SetX(x); }

    f32 Component::GetY() const { return m_transform.GetY(); }

    void Component::SetY(f32 y) { m_transform.SetY(y); }

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

    void Component::SetRotation(const quat& rotation) { m_transform.SetRotation(rotation); }

    bool Component::Contains(const vec2& point) const { return m_bounds.Contains(point); }

    template <>
    GeometrySystem& Component::GetSystem() const
    {
        return Geometric;
    }

    template <>
    UI2DSystem& Component::GetSystem() const
    {
        return UI2D;
    }

    template <>
    RenderSystem& Component::GetSystem() const
    {
        return Renderer;
    }

    template <>
    ShaderSystem& Component::GetSystem() const
    {
        return Shaders;
    }

    template <>
    FontSystem& Component::GetSystem() const
    {
        return Fonts;
    }

    template <>
    InputSystem& Component::GetSystem() const
    {
        return Input;
    }

    template <>
    Platform& Component::GetSystem() const
    {
        return OS;
    }

    bool Component::operator==(const Component& other) const { return m_id == other.m_id; }
    bool Component::operator!=(const Component& other) const { return m_id != other.m_id; }

}  // namespace C3D::UI_2D