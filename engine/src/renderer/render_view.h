
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"
#include "core/events/event_context.h"
#include "math/math_types.h"
#include "memory/allocators/linear_allocator.h"
#include "render_view_types.h"
#include "renderer/renderpass.h"
#include "resources/geometry.h"
#include "resources/skybox.h"
#include "systems/events/event_system.h"

namespace C3D
{
    class Mesh;
    class UIMesh;
    class SystemManager;

    struct RenderViewPacket;

    class C3D_API RenderView
    {
    public:
        RenderView(const String& name, const String& customShaderName);

        RenderView(const RenderView&) = delete;
        RenderView(RenderView&&)      = delete;

        RenderView& operator=(const RenderView&) = delete;
        RenderView& operator=(RenderView&&)      = delete;

        virtual ~RenderView() = default;

        bool OnRegister(const SystemManager* pSystemsManager);

        virtual bool OnCreate() = 0;
        virtual void OnDestroy();

        /**
         * @brief Base method that gets called on resize and performs basic verification, sets the m_width and m_height
         * and resizes the view's passes then it calls the OnResize event. You can override this event if you need
         * custom behaviour like for example a view that does not automatically resize to the size of the screen.
         */
        virtual void OnBaseResize(u32 width, u32 height);

        virtual bool OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket) = 0;
        virtual void OnDestroyPacket(RenderViewPacket& packet);

        virtual bool OnRender(const FrameData& frameData, const RenderViewPacket* packet, u64 frameNumber,
                              u64 renderTargetIndex) = 0;

        virtual bool RegenerateAttachmentTarget(u32 passIndex, RenderTargetAttachment* attachment);

        virtual void RegenerateRenderTargets();

        const String& GetName() const { return m_name; }

    protected:
        bool OnRenderTargetRefreshRequired(u16 code, void* sender, const EventContext& context);

        /** @brief Method that gets called by OnBaseResize() where you can put your basic extra OnResize behaviour. */
        virtual void OnResize();

        /** @brief A method that gets called right after the SystemsManager is setup so you can use the already
         * initialized systems (like for example the Renderer). */
        virtual void OnSetupPasses() = 0;

        String m_name;

        u16 m_width  = 1280;
        u16 m_height = 720;

        DynamicArray<RenderPass*> m_passes;
        DynamicArray<RenderPassConfig> m_passConfigs;

        RegisteredEventCallback m_defaultRenderTargetRefreshRequiredCallback;

        String m_customShaderName;

        LoggerInstance<64> m_logger;

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D
