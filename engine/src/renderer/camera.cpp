
#include "camera.h"

#include "math/c3d_math.h"

namespace C3D
{
    void Camera::Reset()
    {
        m_needsUpdate   = false;
        m_position      = vec3(0);
        m_eulerRotation = vec3(0);
        m_viewMatrix    = mat4(1);
    }

    vec3 Camera::GetPosition() const { return m_position; }

    void Camera::SetPosition(const vec3& position)
    {
        m_position    = position;
        m_needsUpdate = true;
    }

    vec3 Camera::GetEulerRotation() const { return m_eulerRotation; }

    void Camera::SetEulerRotation(const vec3& eulerRotation)
    {
        m_eulerRotation.x = DegToRad(eulerRotation.x);
        m_eulerRotation.y = DegToRad(eulerRotation.y);
        m_eulerRotation.z = DegToRad(eulerRotation.z);
        m_needsUpdate     = true;
    }

    void Camera::SetViewMatrix(const mat4& viewMatrix) { m_viewMatrix = viewMatrix; }

    mat4 Camera::GetViewMatrix()
    {
        if (m_needsUpdate)
        {
            const mat4 rx = glm::eulerAngleX(m_eulerRotation.x);
            const mat4 ry = glm::eulerAngleY(m_eulerRotation.y);
            const mat4 rz = glm::eulerAngleZ(m_eulerRotation.z);

            const mat4 rotation =
                rz * ry * rx;  // glm::eulerAngleXYZ(m_eulerRotation.x, m_eulerRotation.y, m_eulerRotation.z);

            m_viewMatrix = translate(m_position) * rotation;
            m_viewMatrix = inverse(m_viewMatrix);

            m_needsUpdate = false;
        }
        return m_viewMatrix;
    }

    vec3 Camera::GetForward()
    {
        const mat4 view = GetViewMatrix();
        return normalize(vec3(-view[0][2], -view[1][2], -view[2][2]));
    }

    vec3 Camera::GetBackward()
    {
        const mat4 view = GetViewMatrix();
        return normalize(vec3(view[0][2], view[1][2], view[2][2]));
    }

    vec3 Camera::GetLeft()
    {
        const mat4 view = GetViewMatrix();
        return normalize(vec3(-view[0][0], -view[1][0], -view[2][0]));
    }

    vec3 Camera::GetRight()
    {
        const mat4 view = GetViewMatrix();
        return normalize(vec3(view[0][0], view[1][0], view[2][0]));
    }

    vec3 Camera::GetUp()
    {
        const mat4 view = GetViewMatrix();
        return normalize(vec3(view[0][1], view[1][1], view[2][1]));
    }

    vec3 Camera::GetDown()
    {
        const mat4 view = GetViewMatrix();
        return normalize(vec3(-view[0][1], -view[1][1], -view[2][1]));
    }

    void Camera::MoveForward(const f32 amount)
    {
        vec3 direction = GetForward();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveForward(const f64 amount)
    {
        vec3 direction = GetForward();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveBackward(const f32 amount)
    {
        vec3 direction = GetBackward();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveBackward(const f64 amount)
    {
        vec3 direction = GetBackward();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveLeft(const f32 amount)
    {
        vec3 direction = GetLeft();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveLeft(const f64 amount)
    {
        vec3 direction = GetLeft();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveRight(const f32 amount)
    {
        vec3 direction = GetRight();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveRight(const f64 amount)
    {
        vec3 direction = GetRight();
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveUp(const f32 amount)
    {
        auto direction = vec3(0, 1, 0);
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveUp(const f64 amount)
    {
        auto direction = vec3(0, 1, 0);
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveDown(const f32 amount)
    {
        auto direction = vec3(0, -1, 0);
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::MoveDown(const f64 amount)
    {
        auto direction = vec3(0, -1, 0);
        direction *= amount;

        m_position += direction;
        m_needsUpdate = true;
    }

    void Camera::AddYaw(const f32 amount)
    {
        m_eulerRotation.y += amount;
        m_needsUpdate = true;
    }

    void Camera::AddYaw(const f64 amount)
    {
        m_eulerRotation.y += static_cast<f32>(amount);
        m_needsUpdate = true;
    }

    void Camera::AddPitch(const f32 amount)
    {
        m_eulerRotation.x += amount;

        // Clamp to avoid Gimball lock
        static constexpr f32 limit = DegToRad(89.0f);

        m_eulerRotation.x = C3D_CLAMP(m_eulerRotation.x, -limit, limit);
        m_needsUpdate     = true;
    }

    void Camera::AddPitch(const f64 amount)
    {
        m_eulerRotation.x += static_cast<f32>(amount);

        // Clamp to avoid Gimball lock
        static constexpr f32 limit = DegToRad(89.0f);

        m_eulerRotation.x = C3D_CLAMP(m_eulerRotation.x, -limit, limit);
        m_needsUpdate     = true;
    }
}  // namespace C3D
