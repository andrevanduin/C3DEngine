
#pragma once
#include "core/defines.h"
#include "math/math_types.h"
#include "renderer_types.h"

namespace C3D
{
    class C3D_API Viewport
    {
    public:
        Viewport() = default;

        bool Create(const Rect2D& rect, f32 fov, f32 nearClip, f32 farClip, RendererProjectionMatrixType projectionMatrixType);

        void Destroy();

        void Resize(const Rect2D& rect);

        bool PointIsInside(const vec2& point) const;

        void SetProjectionMatrix(const mat4& matrix);

        const Rect2D& GetRect2D() const { return m_rect; }
        const mat4& GetProjection() const { return m_projection; }

        f32 GetAspectRatio() const { return m_rect.width / m_rect.height; }
        f32 GetFov() const { return m_fov; }
        f32 GetNearClip() const { return m_nearClip; }
        f32 GetFarClip() const { return m_farClip; }

    private:
        void RegenerateProjectionMatrix();

        /** @brief The dimensions of the viewport. X position, Y position, width and then height. */
        Rect2D m_rect;
        /** @brief The FOV (field of view) used by the viewport. */
        f32 m_fov = 0.0f;
        /** @brief The near and far clip used by the viewport. */
        f32 m_nearClip = 0.0f, m_farClip = 0.0f;
        /** @brief The type of projection matrix that we should use for this viewport. */
        RendererProjectionMatrixType m_projectionMatrixType;
        /** @brief The projection matrix for this viewport. */
        mat4 m_projection;
    };
}  // namespace C3D