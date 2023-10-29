
#include "viewport.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "VIEWPORT";

    bool Viewport::Create(const Rect2D& rect, const f32 fov, const f32 nearClip, const f32 farClip,
                          const RendererProjectionMatrixType projectionMatrixType)
    {
        m_rect                 = rect;
        m_fov                  = fov;
        m_nearClip             = nearClip;
        m_farClip              = farClip;
        m_projectionMatrixType = projectionMatrixType;

        RegenerateProjectionMatrix();

        return true;
    }

    void Viewport::Destroy() { std::memset(&m_rect, 0, sizeof(vec4)); }

    void Viewport::Resize(const Rect2D& rect)
    {
        m_rect = rect;
        RegenerateProjectionMatrix();
    }

    bool Viewport::PointIsInside(const vec2& point) const { return m_rect.PointIsInside(point); }

    void Viewport::RegenerateProjectionMatrix()
    {
        if (m_projectionMatrixType == RendererProjectionMatrixType::Perspective)
        {
            m_projection = glm::perspective(m_fov, m_rect.width / m_rect.height, m_nearClip, m_farClip);
        }
        else if (m_projectionMatrixType == RendererProjectionMatrixType::Orthographic)
        {
            m_projection = glm::ortho(m_rect.x, m_rect.width, m_rect.height, m_rect.y, m_nearClip, m_farClip);
        }
        else
        {
            ERROR_LOG("Unknown Projection Matrix type: '{}'.", ToUnderlying(m_projectionMatrixType));
        }
    }
}  // namespace C3D