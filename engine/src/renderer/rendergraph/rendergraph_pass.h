
#pragma once
#include "core/frame_data.h"
#include "rendergraph_types.h"

namespace C3D
{
    class RenderPass;
    class Viewport;

    class RendergraphPass
    {
    public:
        virtual bool Initialize()                        = 0;
        virtual bool Execute(const FrameData& frameData) = 0;
        virtual void Destroy()                           = 0;

    private:
        String m_name;

        Viewport* m_viewport;
        mat4 m_viewMatrix;
        mat4 m_projectionMatrix;
        // TODO: We might not really need this
        vec3 m_viewPosition;

        DynamicArray<RendergraphSource> m_sources;
        DynamicArray<RendergraphSink> m_sinks;

        RenderPass* m_pass;
    };
}  // namespace C3D