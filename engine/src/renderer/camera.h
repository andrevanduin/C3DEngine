
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

namespace C3D
{
    class C3D_API Camera
    {
    public:
        Camera() = default;
        void Reset();

        [[nodiscard]] vec3 GetPosition() const;
        void SetPosition(const vec3& position);

        [[nodiscard]] vec3 GetEulerRotation() const;
        void SetEulerRotation(const vec3& eulerRotation);

        void SetViewMatrix(const mat4& viewMatrix);
        mat4 GetViewMatrix() const;

        vec3 GetForward() const;
        vec3 GetBackward() const;

        vec3 GetLeft() const;
        vec3 GetRight() const;

        vec3 GetUp() const;
        vec3 GetDown() const;

        void MoveForward(f32 amount);
        void MoveForward(f64 amount);

        void MoveBackward(f32 amount);
        void MoveBackward(f64 amount);

        void MoveLeft(f32 amount);
        void MoveLeft(f64 amount);

        void MoveRight(f32 amount);
        void MoveRight(f64 amount);

        void MoveUp(f32 amount);
        void MoveUp(f64 amount);

        void MoveDown(f32 amount);
        void MoveDown(f64 amount);

        void AddYaw(f32 amount);
        void AddYaw(f64 amount);

        void AddPitch(f32 amount);
        void AddPitch(f64 amount);

    private:
        /** @brief flag that indicates if camera view needs an update */
        mutable bool m_needsUpdate = false;

        vec3 m_position;
        vec3 m_eulerRotation;

        mutable mat4 m_viewMatrix = mat4(1);
    };
}  // namespace C3D
