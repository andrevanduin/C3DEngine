
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

namespace C3D
{
    class C3D_API Transform
    {
    public:
        Transform() = default;

        explicit Transform(const vec3& position);
        explicit Transform(const quat& rotation);
        Transform(const vec3& position, const quat& rotation);
        Transform(const vec3& position, const quat& rotation, const vec3& scale);

        [[nodiscard]] Transform* GetParent() const;
        void SetParent(Transform* parent);

        const vec3& GetPosition() const;
        void SetPosition(const vec3& position);
        void Translate(const vec3& translation);

        const quat& GetRotation() const;
        void SetRotation(const quat& rotation);
        void SetEulerRotation(const vec3& rotation);
        void Rotate(const quat& rotation);

        const vec3& GetScale() const;
        void SetScale(const vec3& scale);
        void Scale(const vec3& scale);

        void SetPositionRotation(const vec3& position, const quat& rotation);
        void SetPositionRotation(const vec3& position, const vec3& rotation);

        void SetPositionRotationScale(const vec3& position, const quat& rotation, const vec3& scale);
        void SetPositionRotationScale(const vec3& position, const vec3& rotation, const vec3& scale);

        void SetPositionScale(const vec3& position, const vec3& scale);

        void SetRotationScale(const quat& rotation, const vec3& scale);
        void SetRotationScale(const vec3& rotation, const vec3& scale);

        void TranslateRotate(const vec3& translation, const quat& rotation);

        mat4 GetLocal() const;
        mat4 GetWorld() const;
        f32 GetDeterminant() const { return m_determinant; }

    private:
        Transform* m_parent = nullptr;

        vec3 m_position = vec3(0);
        vec3 m_scale    = vec3(1.0f);
        quat m_rotation = quat(1.0f, 0, 0, 0);

        mutable f32 m_determinant  = 0.0f;
        mutable mat4 m_local       = mat4(1.0f);
        mutable bool m_needsUpdate = true;
    };
}  // namespace C3D