
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "math/math_types.h"
#include "render_target.h"

namespace C3D
{
    struct RenderTarget;

    struct RenderPassConfig
    {
        String name;
        f32 depth;
        u32 stencil;

        vec4 renderArea;
        vec4 clearColor;

        u8 clearFlags;

        u8 renderTargetCount;
        RenderTargetConfig target;
    };

    enum RenderPassClearFlags : u8
    {
        ClearNone          = 0x0,
        ClearColorBuffer   = 0x1,
        ClearDepthBuffer   = 0x2,
        ClearStencilBuffer = 0x4
    };

    class C3D_API RenderPass
    {
    public:
        RenderPass();
        explicit RenderPass(const RenderPassConfig& config);

        RenderPass(const RenderPass&) = delete;
        RenderPass(RenderPass&&)      = delete;

        RenderPass& operator=(const RenderPass&)  = delete;
        RenderPass& operator=(RenderPass&& other) = delete;

        virtual ~RenderPass() = default;

        virtual bool Create(const RenderPassConfig& config) = 0;
        virtual void Destroy();

        u16 id;

        ivec4 renderArea;
        u8 renderTargetCount;
        RenderTarget* targets;

    protected:
        String m_name;		
		u8 m_clearFlags;
        vec4 m_clearColor;
    };
}  // namespace C3D