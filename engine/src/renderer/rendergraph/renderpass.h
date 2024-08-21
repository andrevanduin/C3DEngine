
#pragma once
#include "core/frame_data.h"
#include "renderer/render_target.h"
#include "rendergraph_types.h"

namespace C3D
{
    class Viewport;
    class Camera;
    class SystemManager;

    class C3D_API Renderpass
    {
    public:
        Renderpass();
        Renderpass(const String& name);

        bool CreateInternals(const RenderpassConfig& config);

        void Begin(const FrameData& frameData) const;
        void End() const;

        virtual bool Initialize(const LinearAllocator* frameAllocator) = 0;
        virtual bool LoadResources();
        virtual bool Execute(const FrameData& frameData) = 0;
        virtual void Destroy();

        /** @brief Optional method to regenerate attachment textures. */
        virtual bool RegenerateAttachmentTextures(u16 width, u16 height) { return true; };

        /** @brief Optional method to populate a Rendergraph Source. */
        virtual bool PopulateSource(RendergraphSource& source) { return true; }

        /** @brief Optional method to populate a Rendertarget Attachment. */
        virtual bool PopulateAttachment(RenderTargetAttachment& attachment) { return true; }

        void AddSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin);
        void AddSink(const String& name);

        bool RegenerateRenderTargets(u32 width, u32 height);

        bool SourcesContains(const String& name) const;
        bool SinksContains(const String& name) const;

        bool IsPrepared() const { return m_prepared; }

        RendergraphSource* GetSourceByName(const String& name) const;
        RendergraphSink* GetSinkByName(const String& name) const;

        const String& GetName() const { return m_name; }

        void SetViewport(Viewport* viewport) { m_viewport = viewport; }
        void SetCamera(Camera* camera) { m_camera = camera; }

        void SetPresentsAfter(bool b) { m_presentsAfter = b; }
        void SetPrepared(bool b) { m_prepared = b; }

        DynamicArray<RendergraphSource>& Sources() { return m_sources; }
        const DynamicArray<RendergraphSource>& Sources() const { return m_sources; }

        const DynamicArray<RendergraphSink>& Sinks() const { return m_sinks; }

    protected:
        String m_name;

        const Viewport* m_viewport = nullptr;
        Camera* m_camera           = nullptr;

        bool m_presentsAfter = false;
        bool m_prepared      = false;

        DynamicArray<RendergraphSource> m_sources;
        DynamicArray<RendergraphSink> m_sinks;
        DynamicArray<RenderTarget> m_targets;

        void* m_pInternalData = nullptr;
    };
}  // namespace C3D