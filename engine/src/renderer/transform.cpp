
#include "transform.h"

#include "math/c3d_math.h"

namespace C3D
{

    Transform::Transform(const vec3& position) : m_position(position) {}

    Transform::Transform(const quat& rotation) : m_rotation(rotation) {}

    Transform::Transform(const vec3& position, const quat& rotation) : m_position(position), m_rotation(rotation) {}

    Transform::Transform(const vec3& position, const quat& rotation, const vec3& scale)
        : m_position(position), m_scale(scale), m_rotation(rotation)
    {}

    const Transform* Transform::GetParent() const { return m_parent; }

    void Transform::SetParent(const Transform* parent)
    {
        m_parent      = parent;
        m_needsUpdate = true;
    }

    const vec3& Transform::GetPosition() const { return m_position; }

    void Transform::SetPosition(const vec3& position)
    {
        m_position    = position;
        m_needsUpdate = true;
    }

    f32 Transform::GetX() const { return m_position.x; }

    void Transform::SetX(f32 x)
    {
        m_position.x  = x;
        m_needsUpdate = true;
    }

    f32 Transform::GetY() const { return m_position.y; }

    void Transform::SetY(f32 y)
    {
        m_position.y  = y;
        m_needsUpdate = true;
    }

    f32 Transform::GetZ() const { return m_position.z; }

    void Transform::SetZ(f32 z)
    {
        m_position.z  = z;
        m_needsUpdate = true;
    }

    void Transform::Translate(const vec3& translation)
    {
        m_position += translation;
        m_needsUpdate = true;
    }

    const quat& Transform::GetRotation() const { return m_rotation; }

    void Transform::SetRotation(const quat& rotation)
    {
        m_rotation    = rotation;
        m_needsUpdate = true;
    }

    void Transform::SetEulerRotation(const vec3& rotation)
    {
        // TODO: Check this conversion ???
        m_rotation.x = DegToRad(rotation.x);
        m_rotation.y = DegToRad(rotation.y);
        m_rotation.z = DegToRad(rotation.z);
        m_rotation.w = 1.0;
    }

    void Transform::Rotate(const quat& rotation)
    {
        m_rotation *= rotation;
        m_needsUpdate = true;
    }

    const vec3& Transform::GetScale() const { return m_scale; }

    void Transform::SetScale(const vec3& scale)
    {
        m_scale       = scale;
        m_needsUpdate = true;
    }

    void Transform::Scale(const vec3& scale)
    {
        m_scale *= scale;
        m_needsUpdate = true;
    }

    void Transform::SetPositionRotation(const vec3& position, const quat& rotation)
    {
        m_position    = position;
        m_rotation    = rotation;
        m_needsUpdate = true;
    }

    void Transform::SetPositionRotation(const vec3& position, const vec3& rotation)
    {
        m_position    = position;
        m_rotation    = glm::quat(rotation);
        m_needsUpdate = true;
    }

    void Transform::SetPositionRotationScale(const vec3& position, const quat& rotation, const vec3& scale)
    {
        m_position    = position;
        m_rotation    = rotation;
        m_scale       = scale;
        m_needsUpdate = true;
    }

    void Transform::SetPositionRotationScale(const vec3& position, const vec3& rotation, const vec3& scale)
    {
        m_position    = position;
        m_rotation    = glm::quat(rotation);
        m_scale       = scale;
        m_needsUpdate = true;
    }

    void Transform::SetPositionScale(const vec3& position, const vec3& scale)
    {
        m_position    = position;
        m_scale       = scale;
        m_needsUpdate = true;
    }

    void Transform::SetRotationScale(const quat& rotation, const vec3& scale)
    {
        m_rotation    = rotation;
        m_scale       = scale;
        m_needsUpdate = true;
    }

    void Transform::SetRotationScale(const vec3& rotation, const vec3& scale)
    {
        m_rotation.x  = DegToRad(rotation.x);
        m_rotation    = glm::quat(rotation);
        m_scale       = scale;
        m_needsUpdate = true;
    }

    void Transform::TranslateRotate(const vec3& translation, const quat& rotation)
    {
        m_position += translation;
        m_rotation *= rotation;
        m_needsUpdate = true;
    }

    mat4 Transform::GetLocal() const
    {
        if (m_needsUpdate)
        {
            const mat4 t = translate(m_position);
            const mat4 r = mat4_cast(m_rotation);
            const mat4 s = scale(m_scale);

            m_needsUpdate = false;
            m_local       = t * r * s;
            return m_local;
        }

        return m_local;
    }

    mat4 Transform::GetWorld() const
    {
        const mat4 local = GetLocal();
        if (m_parent)
        {
            const mat4 parent = m_parent->GetWorld();
            const mat4 ret    = parent * local;
            m_determinant     = glm::determinant(ret);
            return ret;
        }
        // If we have no parent get the determinant from the local matrix
        m_determinant = glm::determinant(local);
        return local;
    }
}  // namespace C3D
