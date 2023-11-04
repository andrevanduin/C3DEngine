
#pragma once
#include "rendergraph_pass.h"
#include "rendergraph_types.h"

namespace C3D
{
    class Application;

    C3D_API class Rendergraph
    {
    public:
        Rendergraph() = default;

        bool Create(const String& name, Application* application);
        void Destroy();

        bool AddGlobalSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin);

        bool AddPass(const String& name, RendergraphPass* pass);
        bool AddSource(const String& passName, const String& sourceName, RendergraphSourceType type, RendergraphSourceOrigin origin);
        bool AddSink(const String& passName, const String& sinkName);

        bool Link(const String& sourcePassName, const String& sourceName, const String& sinkPassName, const String& sinkName);

        bool Finalize();

        bool ExecuteFrame(const FrameData& frameData);

    private:
        String m_name;
        Application* m_application;

        /** @brief The globally accessible sources that can be used by everyone. */
        DynamicArray<RendergraphSource> m_globalSources;
        /** @brief The passes that the Rendergraph is working with. */
        DynamicArray<RendergraphPass*> m_passes;
        /** @brief The global final sink which everything feeds into. */
        RendergraphSink m_globalSink;

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D