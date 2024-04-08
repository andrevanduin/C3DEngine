
#include "component.h"

#include "renderer/renderer_frontend.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_COMPONENT";

    Component::Component(const SystemManager* systemsManager) : m_pSystemsManager(systemsManager) { m_id.Generate(); }

    void Component::Initialize(const u16vec2& pos, const u16vec2& size)
    {
        m_transform = Transform();
        m_transform.SetPosition(vec3(pos.x, pos.y, 0));
        m_bounds = Bounds(pos.x, pos.y, size.x, size.y);
    }

    void Component::Destroy(const DynamicAllocator* pAllocator)
    {
        m_id.Invalidate();
        onDestroy(*this, pAllocator);
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

    UUID Component::GetID() const { return m_id; }

    const Transform& Component::GetTransform() const { return m_transform; }

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

    vec2 Component::GetPosition() const { return vec2(m_bounds.x, m_bounds.y); }

    void Component::SetPosition(const u16vec2& position)
    {
        m_bounds.x = position.x;
        m_bounds.y = position.y;
        m_transform.SetPosition(vec3(position, 0.0f));
    }

    f32 Component::GetX() const { return m_bounds.x; }

    void Component::SetX(f32 x)
    {
        m_bounds.x = x;
        m_transform.SetX(x);
    }

    f32 Component::GetY() const { return m_bounds.y; }

    void Component::SetY(f32 y)
    {
        m_bounds.y = y;
        m_transform.SetY(y);
    }

    u16 Component::GetWidth() const { return m_bounds.width; }

    void Component::SetWidth(u16 width) { m_bounds.width = width; };

    u16 Component::GetHeight() const { return m_bounds.height; }

    void Component::SetHeight(u16 height) { m_bounds.height = height; }

    u16vec2 Component::GetSize() const { return u16vec2(m_bounds.width, m_bounds.height); }

    void Component::SetSize(const u16vec2& size)
    {
        m_bounds.width  = size.x;
        m_bounds.height = size.y;
    }

    void Component::SetRotation(const quat& rotation) { m_transform.SetRotation(rotation); }

    bool Component::Contains(const vec2& point) const { return m_bounds.Contains(point); }

    mat4 Component::GetWorld() const { return m_transform.GetWorld(); }

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

    bool Component::operator==(const Component& other) const { return m_id == other.m_id; }
    bool Component::operator!=(const Component& other) const { return m_id != other.m_id; }

}  // namespace C3D::UI_2D