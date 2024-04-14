
#pragma once
#include "core/frame_data.h"
#include "rendergraph_types.h"

namespace C3D
{
    class RenderPass;
    class Viewport;
    class Camera;
    class SystemManager;

    class C3D_API RendergraphPass
    {
    public:
        RendergraphPass();
        RendergraphPass(const String& name, const SystemManager* pSystemsManager);

        virtual bool Initialize(const LinearAllocator* frameAllocator) = 0;
        virtual bool Execute(const FrameData& frameData)               = 0;
        virtual void Destroy();

        bool RegenerateRenderTargets(u32 width, u32 height) const;

        bool SourcesContains(const String& name) const;
        bool SinksContains(const String& name) const;

        RendergraphSource* GetSourceByName(const String& name) const;
        RendergraphSink* GetSinkByName(const String& name) const;

        const String& GetName() const { return m_name; }

        void SetViewport(Viewport* viewport) { m_viewport = viewport; }
        void SetCamera(Camera* camera) { m_camera = camera; }

        RenderPass* GetRenderPass() const { return m_pass; }

    protected:
        void AddSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin);
        void AddSink(const String& name);

        String m_name;

        Viewport* m_viewport = nullptr;
        Camera* m_camera     = nullptr;

        bool m_presentsAfter = false;
        bool m_prepared      = false;

        DynamicArray<RendergraphSource> m_sources;
        DynamicArray<RendergraphSink> m_sinks;

        RenderPass* m_pass = nullptr;

        const SystemManager* m_pSystemsManager = nullptr;

        friend class Rendergraph;
    };
}  // namespace C3D