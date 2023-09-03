
#pragma once
#include "resource_loader.h"
#include "resources/scene/simple_scene_config.h"

namespace C3D
{
    enum class ParserTagType
    {
        Invalid,
        Closing,
        Scene,
        Mesh,
        Skybox,
        DirectionalLight,
        PointLight
    };

    template <>
    class C3D_API ResourceLoader<SimpleSceneConfig> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, SimpleSceneConfig& resource) const;

        void Unload(SimpleSceneConfig& resource) const;

    private:
        bool ParseTagContent(const String& line, const String& fileName, u32 lineNumber, u32& version,
                             ParserTagType type, SimpleSceneConfig& cfg) const;

        std::pair<bool, String> ParseScene(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        std::pair<bool, String> ParseSkybox(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        std::pair<bool, String> ParseDirectionalLight(const String& name, const String& value,
                                                      SimpleSceneConfig& cfg) const;
        std::pair<bool, String> ParsePointLight(const String& name, const String& value, SimpleSceneConfig& cfg) const;
        std::pair<bool, String> ParseMesh(const String& name, const String& value, SimpleSceneConfig& cfg) const;

        ParserTagType ParseTag(const String& line, const String& fileName, u32 lineNumber,
                               SimpleSceneConfig& cfg) const;
    };
}  // namespace C3D