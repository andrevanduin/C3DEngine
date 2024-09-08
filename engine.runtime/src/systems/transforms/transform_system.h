
#pragma once
#include "defines.h"
#include "identifiers/handle.h"
#include "math/math_types.h"
#include "systems/system.h"

namespace C3D
{
    struct TransformSystemConfig
    {
        u32 initialTransforms = 64;
    };

    // Keep a struct simply to indentify handles of the Transform type
    struct Transform
    {
    };

    class C3D_API TransformSystem final : public SystemWithConfig<TransformSystemConfig>
    {
    public:
        bool OnInit(const CSONObject& config) override;
        void OnShutdown() override;

        Handle<Transform> Acquire();
        Handle<Transform> Acquire(const vec3& position);
        Handle<Transform> Acquire(const quat& rotation);
        Handle<Transform> Acquire(const vec3& position, const quat& rotation);
        Handle<Transform> Acquire(const vec3& position, const quat& rotation, const vec3& scale);

        bool Translate(Handle<Transform> handle, const vec3& translation);
        bool Scale(Handle<Transform> handle, const vec3& scale);
        bool Rotate(Handle<Transform> handle, const quat& rotation);

        const mat4& GetLocal(Handle<Transform> handle) const;

        const mat4& GetWorld(Handle<Transform> handle) const;
        void SetWorld(Handle<Transform> handle, const mat4& world);

        f32 GetDeterminant(Handle<Transform> handle) const;

        const vec3& GetPosition(Handle<Transform> handle) const;
        bool SetPosition(Handle<Transform> handle, const vec3& position);

        bool SetX(Handle<Transform> handle, f32 x);
        bool SetY(Handle<Transform> handle, f32 y);
        bool SetZ(Handle<Transform> handle, f32 z);

        const quat& GetRotation(Handle<Transform> handle) const;
        bool SetRotation(Handle<Transform> handle, const quat& rotation);

        bool IsDirty(Handle<Transform> handle) const;

        bool UpdateLocal(Handle<Transform> handle);

        bool Release(Handle<Transform>& handle);

    private:
        bool Allocate(u64 newNumberOfTransforms);

        Handle<Transform> CreateHandle();

        vec3* m_positions = nullptr;
        vec3* m_scales    = nullptr;
        quat* m_rotations = nullptr;

        f32* m_determinants = nullptr;

        mat4* m_localMatrices = nullptr;
        mat4* m_worldMatrices = nullptr;

        UUID* m_uuids        = nullptr;
        bool* m_isDirtyFlags = nullptr;

        u64 m_numberOfTransforms = 0;
    };
}  // namespace C3D