
#include "scene_manager.h"

#include "exceptions.h"
#include "platform/file_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"
#include "systems/transforms/transform_system.h"

namespace C3D
{
    constexpr const char* FILE_EXTENSION = "cson";

    ResourceManager<SceneConfig>::ResourceManager() : IResourceManager(MemoryType::Scene, ResourceType::Scene, nullptr, "scenes") {}

    bool ResourceManager<SceneConfig>::Read(const String& name, SceneConfig& resource)
    {
        if (name.Empty())
        {
            ERROR_LOG("Provided name was empty.");
            return false;
        }

        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, FILE_EXTENSION);
        auto fileName = String::FromFormat("{}.{}", name, FILE_EXTENSION);

        auto object = m_reader.ReadFromFile(fullPath);

        // TODO: Make this not hardcoded!
        resource.version     = 1;
        resource.fullPath    = fullPath;
        resource.name        = name;
        resource.description = "";

        for (const auto& prop : object.properties)
        {
            if (prop.name.IEquals("name"))
            {
                resource.name = prop.GetString();
            }
            else if (prop.name.IEquals("description"))
            {
                resource.description = prop.GetString();
            }
            else if (prop.name.IEquals("skyboxes"))
            {
                const auto& skyboxes = prop.GetArray();
                ParseSkyboxes(resource, skyboxes);
            }
            else if (prop.name.IEquals("directionalLights"))
            {
                const auto& directionalLights = prop.GetArray();
                ParseDirectionalLights(resource, directionalLights);
            }
            else if (prop.name.IEquals("pointLights"))
            {
                const auto& pointLights = prop.GetArray();
                ParsePointLights(resource, pointLights);
            }
            else if (prop.name.IEquals("meshes"))
            {
                const auto& meshes = prop.GetArray();
                if (!ParseMeshes(resource, meshes))
                {
                    ERROR_LOG("Failed to parse meshes.");
                    return false;
                }
            }
            else if (prop.name.IEquals("terrains"))
            {
                const auto& terrains = prop.GetArray();
                ParseTerrains(resource, terrains);
            }
        }

        return true;
    }

    bool ResourceManager<SceneConfig>::Write(const SceneConfig& resource)
    {
        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, resource.name, FILE_EXTENSION);

        CSONObject object(CSONObjectType::Object);
        object.properties.EmplaceBack("name", resource.name);
        object.properties.EmplaceBack("description", resource.description);

        CSONObject skybox(CSONObjectType::Object);
        skybox.properties.EmplaceBack("name", resource.skyboxConfig.name);
        skybox.properties.EmplaceBack("cubemapName", resource.skyboxConfig.cubemapName);

        CSONArray skyboxes(CSONObjectType::Array);
        skyboxes.properties.EmplaceBack(skybox);

        object.properties.EmplaceBack("skyboxes", skyboxes);

        CSONObject directionalLight(CSONObjectType::Object);
        directionalLight.properties.EmplaceBack("name", resource.directionalLightConfig.name);
        directionalLight.properties.EmplaceBack("color", resource.directionalLightConfig.color);
        directionalLight.properties.EmplaceBack("direction", resource.directionalLightConfig.direction);
        directionalLight.properties.EmplaceBack("shadowDistance", resource.directionalLightConfig.shadowDistance);
        directionalLight.properties.EmplaceBack("shadowFadeDistance", resource.directionalLightConfig.shadowFadeDistance);
        directionalLight.properties.EmplaceBack("shadowSplitMultiplier", resource.directionalLightConfig.shadowSplitMultiplier);

        CSONObject directionalLights(CSONObjectType::Array);
        directionalLights.properties.EmplaceBack(directionalLight);

        CSONProperty directionalLightsProperty("directionalLights", directionalLights);

        CSONArray pointLights(CSONObjectType::Array);
        for (const auto& p : resource.pointLights)
        {
            CSONObject pointLight(CSONObjectType::Object);
            pointLight.properties.EmplaceBack("name", p.name);
            pointLight.properties.EmplaceBack("color", p.color);
            pointLight.properties.EmplaceBack("position", p.position);
            pointLight.properties.EmplaceBack("constant", p.constant);
            pointLight.properties.EmplaceBack("linear", p.linear);
            pointLight.properties.EmplaceBack("quadratic", p.quadratic);

            pointLights.properties.EmplaceBack(pointLight);
        }
        object.properties.EmplaceBack("pointLights", pointLights);

        CSONArray meshes(CSONObjectType::Array);
        for (const auto& m : resource.meshes)
        {
            CSONObject mesh(CSONObjectType::Object);
            mesh.properties.EmplaceBack("name", m.name);
            mesh.properties.EmplaceBack("resourceName", m.resourceName);
            mesh.properties.EmplaceBack("transform", m.transform);
        }
        object.properties.EmplaceBack("meshes", meshes);

        CSONArray terrains(CSONObjectType::Array);
        for (const auto& t : resource.terrains)
        {
            CSONObject terrain(CSONObjectType::Object);
            terrain.properties.EmplaceBack("name", t.name);
            terrain.properties.EmplaceBack("resourceName", t.resourceName);
            terrain.properties.EmplaceBack("transform", t.transform);
        }
        object.properties.EmplaceBack("terrains", terrains);

        if (!m_writer.WriteToFile(object, fullPath))
        {
            ERROR_LOG("Failed to write: '{}' scene to a file.", resource.name);
            return false;
        }

        return true;
    }

    void ResourceManager<SceneConfig>::Cleanup(SceneConfig& resource) const
    {
        resource.name.Destroy();
        resource.description.Destroy();
        resource.fullPath.Destroy();
        resource.pointLights.Destroy();
        resource.meshes.Destroy();
    }

    void ResourceManager<SceneConfig>::ParseSkyboxes(SceneConfig& resource, const CSONArray& skyboxes)
    {
        for (const auto& skyboxProp : skyboxes.properties)
        {
            const auto& skyboxObj = skyboxProp.GetObject();
            for (const auto& prop : skyboxObj.properties)
            {
                if (prop.name.IEquals("name"))
                {
                    resource.skyboxConfig.name = prop.GetString();
                }
                else if (prop.name.IEquals("cubemapname"))
                {
                    resource.skyboxConfig.cubemapName = prop.GetString();
                }
            }
        }
    }

    void ResourceManager<SceneConfig>::ParseDirectionalLights(SceneConfig& resource, const CSONArray& lights)
    {
        for (const auto& lightProp : lights.properties)
        {
            const auto& lightObj = lightProp.GetObject();
            for (const auto& prop : lightObj.properties)
            {
                if (prop.name.IEquals("name"))
                {
                    resource.directionalLightConfig.name = prop.GetString();
                }
                else if (prop.name.IEquals("color"))
                {
                    resource.directionalLightConfig.color = prop.GetVec4();
                }
                else if (prop.name.IEquals("direction"))
                {
                    resource.directionalLightConfig.direction = prop.GetVec4();
                }
                else if (prop.name.IEquals("shadowDistance"))
                {
                    resource.directionalLightConfig.shadowDistance = prop.GetF64();
                }
                else if (prop.name.IEquals("shadowFadeDistance"))
                {
                    resource.directionalLightConfig.shadowFadeDistance = prop.GetF64();
                }
                else if (prop.name.IEquals("shadowSplitMultiplier"))
                {
                    resource.directionalLightConfig.shadowSplitMultiplier = prop.GetF64();
                }
            }
        }
    }

    void ResourceManager<SceneConfig>::ParsePointLights(SceneConfig& resource, const CSONArray& lights)
    {
        for (const auto& lightProp : lights.properties)
        {
            const auto& lightObj = lightProp.GetObject();

            auto pointLight = ScenePointLightConfig();

            for (const auto& prop : lightObj.properties)
            {
                if (prop.name.IEquals("name"))
                {
                    pointLight.name = prop.GetString();
                }
                else if (prop.name.IEquals("color"))
                {
                    pointLight.color = prop.GetVec4();
                }
                else if (prop.name.IEquals("position"))
                {
                    pointLight.position = prop.GetVec4();
                }
                else if (prop.name.IEquals("constant"))
                {
                    pointLight.constant = prop.GetF64();
                }
                else if (prop.name.IEquals("linear"))
                {
                    pointLight.linear = prop.GetF64();
                }
                else if (prop.name.IEquals("quadratic"))
                {
                    pointLight.quadratic = prop.GetF64();
                }
            }

            resource.pointLights.PushBack(pointLight);
        }
    }

    bool ResourceManager<SceneConfig>::ParseMeshes(SceneConfig& resource, const CSONArray& meshes)
    {
        for (const auto& meshProp : meshes.properties)
        {
            const auto& meshObj = meshProp.GetObject();

            auto mesh = SceneMeshConfig();

            for (const auto& prop : meshObj.properties)
            {
                if (prop.name.IEquals("name"))
                {
                    mesh.name = prop.GetString();
                }
                else if (prop.name.IEquals("resourceName"))
                {
                    mesh.resourceName = prop.GetString();
                }
                else if (prop.name.IEquals("parent"))
                {
                    mesh.parentName = prop.GetString();
                }
                else if (prop.name.IEquals("transform"))
                {
                    const auto& transform = prop.GetArray();
                    const auto& props     = transform.properties;
                    if (props.Size() != 10)
                    {
                        ERROR_LOG("Transform for: '{}' does not contain 10 floats.", mesh.name);
                        return false;
                    }

                    vec3 pos   = { props[0].GetF32(), props[1].GetF32(), props[2].GetF32() };
                    quat rot   = { props[6].GetF32(), props[3].GetF32(), props[4].GetF32(), props[5].GetF32() };
                    vec3 scale = { props[7].GetF32(), props[8].GetF32(), props[9].GetF32() };

                    mesh.transform = Transforms.Acquire(pos, rot, scale);
                }
            }

            resource.meshes.PushBack(mesh);
        }

        return true;
    }

    bool ResourceManager<SceneConfig>::ParseTerrains(SceneConfig& resource, const CSONArray& terrains)
    {
        for (const auto& terrainProp : terrains.properties)
        {
            const auto& terrainObj = terrainProp.GetObject();

            auto terrain = SceneTerrainConfig();

            for (const auto& prop : terrainObj.properties)
            {
                if (prop.name.IEquals("name"))
                {
                    terrain.name = prop.GetString();
                }
                else if (prop.name.IEquals("resourceName"))
                {
                    terrain.resourceName = prop.GetString();
                }
                else if (prop.name.IEquals("transform"))
                {
                    const auto& transform = prop.GetArray();
                    const auto& props     = transform.properties;
                    if (props.Size() != 10)
                    {
                        ERROR_LOG("Transform for: '{}' does not contain 10 floats.", terrain.name);
                        return false;
                    }

                    vec3 pos   = { props[0].GetF32(), props[1].GetF32(), props[2].GetF32() };
                    quat rot   = { props[6].GetF32(), props[3].GetF32(), props[4].GetF32(), props[5].GetF32() };
                    vec3 scale = { props[7].GetF32(), props[8].GetF32(), props[9].GetF32() };

                    terrain.transform = Transforms.Acquire(pos, rot, scale);
                }
            }

            resource.terrains.PushBack(terrain);
        }

        return true;
    }
}  // namespace C3D
