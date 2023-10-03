
#pragma once
#include "resource_loader.h"
#include "resources/scene/simple_scene_config.h"

namespace C3D
{
    template <>
    class C3D_API ResourceLoader<SimpleSceneConfig> final : public IResourceLoader
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
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, SimpleSceneConfig& resource) const;

        void Unload(SimpleSceneConfig& resource) const;

    private:
        bool ParseTagContent(const String& line, const String& fileName, u32 lineNumber, u32& version,
                             ParserTagType type, SimpleSceneConfig& cfg) const;

        void ParseScene(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseSkybox(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseDirectionalLight(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParsePointLight(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseMesh(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseTerrain(const String& name, const String& value, SimpleSceneConfig& cfg) const;

        Transform ParseTransform(const String& value) const;

        ParserTagType ParseTag(const String& line, const String& fileName, u32 lineNumber,
                               SimpleSceneConfig& cfg) const;
    };
}  // namespace C3D