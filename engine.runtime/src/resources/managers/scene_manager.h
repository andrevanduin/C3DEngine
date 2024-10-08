
#pragma once
#include "cson/cson_reader.h"
#include "cson/cson_writer.h"
#include "resource_manager.h"
#include "resources/scenes/scene_config.h"

namespace C3D
{
    template <>
    class C3D_API ResourceManager<SceneConfig> final : public IResourceManager
    {
        enum class ParserTagType
        {
            Invalid,
            Closing,
            Scene,
            Mesh,
            Skybox,
            DirectionalLight,
            PointLight,
            Terrain
        };

    public:
        ResourceManager();

        bool Read(const String& name, SceneConfig& resource);
        bool Write(const SceneConfig& resource);
        void Cleanup(SceneConfig& resource) const;

    private:
        void ParseSkyboxes(SceneConfig& resource, const CSONArray& skyboxes);
        void ParseDirectionalLights(SceneConfig& resource, const CSONArray& lights);
        void ParsePointLights(SceneConfig& resource, const CSONArray& lights);
        bool ParseMeshes(SceneConfig& resource, const CSONArray& meshes);
        bool ParseTerrains(SceneConfig& resource, const CSONArray& terrains);

        CSONReader m_reader;
        CSONWriter m_writer;
    };
}  // namespace C3D