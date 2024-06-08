
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

        if (m_projectionMatrixType == RendererProjectionMatrixType::OrthographicCentered && m_fov == 0)
        {
            ERROR_LOG("Tried to create a viewport with Centered Orthographic type and Fov == 0. Fov should be non-zero for this type.");
            return false;
        }

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

    void Viewport::SetProjectionMatrix(const mat4& matrix) { m_projection = matrix; }

    void Viewport::RegenerateProjectionMatrix()
    {
        switch (m_projectionMatrixType)
        {
            case RendererProjectionMatrixType::Perspective:
                m_projection = glm::perspective(m_fov, m_rect.width / m_rect.height, m_nearClip, m_farClip);
                break;
            case RendererProjectionMatrixType::OrthographicCentered:
            {
                f32 mod = m_fov;
                m_projection =
                    glm::ortho(-m_rect.width * mod, m_rect.width * mod, -m_rect.height * mod, m_rect.height * mod, m_nearClip, m_farClip);
            }
            break;
            case RendererProjectionMatrixType::Orthographic:
                m_projection = glm::ortho(m_rect.x, m_rect.width, m_rect.height, m_rect.y, m_nearClip, m_farClip);
                break;
            default:
                ERROR_LOG("Unknown Projection Matrix type: '{}'.", ToUnderlying(m_projectionMatrixType));
                break;
        }
    }
}  // namespace C3D