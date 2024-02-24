
#pragma once
#include "rendergraph_pass.h"
#include "rendergraph_types.h"

namespace C3D
{
    class Application;
    class SystemManager;

    class C3D_API Rendergraph
    {
    public:
        Rendergraph();

        bool Create(const String& name, const Application* application);

        bool OnResize(u32 width, u32 height);

        void Destroy();

        bool AddGlobalSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin);

        bool AddPass(const String& name, RendergraphPass* pass);
        bool AddSource(const String& passName, const String& sourceName, RendergraphSourceType type, RendergraphSourceOrigin origin);
        bool AddSink(const String& passName, const String& sinkName);

        /** @brief Link a Pass Source to a Pass Sink. */
        bool Link(const String& sourcePassName, const String& sourceName, const String& sinkPassName, const String& sinkName);

        /** @brief Link a Global Source to a Pass Sink. */
        bool Link(const String& sourceName, const String& sinkPassName, const String& sinkName);

        bool Finalize(const C3D::LinearAllocator* frameAllocator);

        bool ExecuteFrame(const FrameData& frameData);

    private:
        RendergraphPass* GetPassByName(const String& name) const;

        bool SourceHasLinkedSink(RendergraphSource* source) const;

        String m_name;
        const Application* m_application = nullptr;

        /** @brief The globally accessible sources that can be used by everyone. */
        DynamicArray<RendergraphSource> m_globalSources;
        /** @brief The passes that the Rendergraph is working with. */
        DynamicArray<RendergraphPass*> m_passes;
        /** @brief The global final sink which everything feeds into. */
        RendergraphSink m_globalSink;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D