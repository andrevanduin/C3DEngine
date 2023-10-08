
#pragma once
#include <resources/loaders/resource_loader.h>

#include "resources/scenes/simple_scene_config.h"

template <>
class C3D::ResourceLoader<SimpleSceneConfig> final : public C3D::IResourceLoader
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
    explicit ResourceLoader(const C3D::SystemManager* pSystemsManager);

    bool Load(const char* name, SimpleSceneConfig& resource) const;

    void Unload(SimpleSceneConfig& resource) const;

private:
    bool ParseTagContent(const C3D::String& line, const C3D::String& fileName, u32 lineNumber, u32& version,
                         ParserTagType type, SimpleSceneConfig& cfg) const;

    void ParseScene(const C3D::String& name, const C3D::String& value, SimpleSceneConfig& cfg) const;
    void ParseSkybox(const C3D::String& name, const C3D::String& value, SimpleSceneConfig& cfg) const;
    void ParseDirectionalLight(const C3D::String& name, const String& value, SimpleSceneConfig& cfg) const;
    void ParsePointLight(const C3D::String& name, const C3D::String& value, SimpleSceneConfig& cfg) const;
    void ParseMesh(const C3D::String& name, const C3D::String& value, SimpleSceneConfig& cfg) const;
    void ParseTerrain(const C3D::String& name, const C3D::String& value, SimpleSceneConfig& cfg) const;

    Transform ParseTransform(const C3D::String& value) const;

    ParserTagType ParseTag(const C3D::String& line, const C3D::String& fileName, u32 lineNumber,
                           SimpleSceneConfig& cfg) const;
};