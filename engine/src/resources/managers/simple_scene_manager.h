
#pragma once
#include "resource_manager.h"
#include "resources/scenes/simple_scene_config.h"

namespace C3D
{
    template <>
    class C3D_API ResourceManager<SimpleSceneConfig> final : public IResourceManager
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

        bool Read(const String& name, SimpleSceneConfig& resource) const;
        bool Write(const SimpleSceneConfig& resource) const;
        void Cleanup(SimpleSceneConfig& resource) const;

    private:
        bool ParseTagContent(const String& line, const String& fileName, u32 lineNumber, u32& version, ParserTagType type,
                             SimpleSceneConfig& cfg) const;

        void ParseScene(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseSkybox(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseDirectionalLight(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParsePointLight(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseMesh(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        void ParseTerrain(const String& name, const String& value, SimpleSceneConfig& cfg) const;

        Transform ParseTransform(const String& value) const;

        ParserTagType ParseTag(const String& line, const String& fileName, u32 lineNumber, SimpleSceneConfig& cfg) const;
    };
}  // namespace C3D