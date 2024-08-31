
#pragma once
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

        bool Read(const String& name, SceneConfig& resource) const;
        bool Write(const SceneConfig& resource) const;
        void Cleanup(SceneConfig& resource) const;

    private:
        bool ParseTagContent(const String& line, const String& fileName, u32 lineNumber, u32& version, ParserTagType type,
                             SceneConfig& cfg) const;

        void ParseScene(const String& name, const String& value, SceneConfig& cfg) const;
        void ParseSkybox(const String& name, const String& value, SceneConfig& cfg) const;
        void ParseDirectionalLight(const String& name, const String& value, SceneConfig& cfg) const;
        void ParsePointLight(const String& name, const String& value, SceneConfig& cfg) const;
        void ParseMesh(const String& name, const String& value, SceneConfig& cfg) const;
        void ParseTerrain(const String& name, const String& value, SceneConfig& cfg) const;

        Handle<Transform> ParseTransform(const String& value) const;

        ParserTagType ParseTag(const String& line, const String& fileName, u32 lineNumber, SceneConfig& cfg) const;
    };
}  // namespace C3D