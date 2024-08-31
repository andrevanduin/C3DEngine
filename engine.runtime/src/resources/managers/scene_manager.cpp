
#include "scene_manager.h"

#include "exceptions.h"
#include "platform/file_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"
#include "systems/transforms/transform_system.h"

namespace C3D
{
    constexpr const char* FILE_EXTENSION = "scenecfg";

    ResourceManager<SceneConfig>::ResourceManager() : IResourceManager(MemoryType::Scene, ResourceType::Scene, nullptr, "scenes") {}

    bool ResourceManager<SceneConfig>::Read(const String& name, SceneConfig& resource) const
    {
        if (name.Empty())
        {
            ERROR_LOG("Provided name was empty.");
            return false;
        }

        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, FILE_EXTENSION);
        auto fileName = String::FromFormat("{}.{}", name, FILE_EXTENSION);

        File file;
        if (!file.Open(fullPath, FileModeRead))
        {
            ERROR_LOG("Failed to open simple scene config file for reading: '{}'.", fullPath);
            return false;
        }

        // TODO: Make this not hardcoded!
        resource.version     = 1;
        resource.fullPath    = fullPath;
        resource.name        = name;
        resource.description = "";

        String line;
        u32 lineNumber     = 1;
        u32 version        = 0;
        ParserTagType type = ParserTagType::Invalid;

        while (file.ReadLine(line))
        {
            // Trim the line
            line.Trim();

            // Skip Blank lines and comments
            if (line.Empty() || line.First() == '#')
            {
                lineNumber++;
                continue;
            }

            if (version == 0 && !line.StartsWith("!version"))
            {
                ERROR_LOG("Failed to load file: '{}'. Simple scene config should start with !version = <parser version>.", fullPath);
                return false;
            }

            if (line.StartsWith("["))
            {
                type = ParseTag(line, fileName, lineNumber, resource);
                if (type == ParserTagType::Invalid)
                {
                    ERROR_LOG("Failed to load file: '{}'. Unknown tag: '{}' found on line: {}.", fileName, line, lineNumber);
                    return false;
                }
            }
            else
            {
                if (!ParseTagContent(line, fileName, lineNumber, version, type, resource)) return false;
            }

            lineNumber++;
        }

        file.Close();
        return true;
    }

    bool ResourceManager<SceneConfig>::Write(const SceneConfig& resource) const
    {
        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, resource.name, FILE_EXTENSION);
        // auto fileName = String::FromFormat("{}.{}", resource.name, FILE_EXTENSION);

        File file;
        if (!file.Open(fullPath, FileModeWrite))
        {
            ERROR_LOG("Failed to open simple scene config file for reading: '{}'.", fullPath);
            return false;
        }

        String buffer;
        buffer.Reserve(256);

        // TODO: Replace this non-hardcoded version
        file.WriteLine("!version = {}", resource.version);

        // Scene block
        file.WriteLine("[Scene]");
        file.WriteLine("name = {}", resource.name);
        file.WriteLine("description = {}", resource.description);
        file.WriteLine("[/Scene]");

        // Skybox block
        if (!resource.skyboxConfig.name.Empty() && !resource.skyboxConfig.cubemapName.Empty())
        {
            file.WriteLine("[Skybox]");
            file.WriteLine("name = {}", resource.skyboxConfig.name);
            file.WriteLine("cubemapName = {}", resource.skyboxConfig.cubemapName);
            file.WriteLine("[/Skybox]");
        }

        // Directional light block
        if (!resource.directionalLightConfig.name.Empty())
        {
            file.WriteLine("[DirectionalLight]");
            file.WriteLine("name = {}", resource.directionalLightConfig.name);
            file.WriteLine("color = {}", resource.directionalLightConfig.color);
            file.WriteLine("direction = {}", resource.directionalLightConfig.direction);
            file.WriteLine("shadowDistance = {}", resource.directionalLightConfig.shadowDistance);
            file.WriteLine("shadowFadeDistance = {}", resource.directionalLightConfig.shadowFadeDistance);
            file.WriteLine("shadowSplitMultiplier = {}", resource.directionalLightConfig.shadowSplitMultiplier);
            file.WriteLine("[DirectionalLight]");
        }

        // Meshes
        for (const auto& mesh : resource.meshes)
        {
            file.WriteLine("[Mesh]");
            file.WriteLine("name = {}", mesh.name);
            file.WriteLine("resourceName = {}", mesh.resourceName);
            file.WriteLine("transform = {}", mesh.transform);
            if (!mesh.parentName.Empty())
            {
                file.WriteLine("parent = {}", mesh.parentName);
            }
            file.WriteLine("[/Mesh]");
        }

        // Point lights
        for (const auto& light : resource.pointLights)
        {
            file.WriteLine("[PointLight]");
            file.WriteLine("name = {}", light.name);
            file.WriteLine("color = {}", light.color);
            file.WriteLine("position = {}", light.position);
            file.WriteLine("constant = {}", light.constant);
            file.WriteLine("linear = {}", light.linear);
            file.WriteLine("quadratic = {}", light.quadratic);
            file.WriteLine("[/PointLight]");
        }

        // Terrains
        for (const auto& terrain : resource.terrains)
        {
            file.WriteLine("[Terrain]");
            file.WriteLine("name = {}", terrain.name);
            file.WriteLine("resourceName = {}", terrain.resourceName);
            file.WriteLine("transform = {}", terrain.transform);
            file.WriteLine("[/Terrain]");
        }

        file.Close();
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

    bool ResourceManager<SceneConfig>::ParseTagContent(const String& line, const String& fileName, const u32 lineNumber, u32& version,
                                                       const ParserTagType type, SceneConfig& cfg) const
    {
        // Check if we have a '=' symbol
        if (!line.Contains('='))
        {
            WARN_LOG("Potential formatting issue found in file: '{}', '=' token not found. Skipping line: {}.", fileName, lineNumber);
            return false;
        }

        auto parts = line.Split('=');
        if (parts.Size() != 2)
        {
            WARN_LOG("Potential formatting issue found in file: '{}', too many '=' tokens found. Skipping line: {}.", fileName, lineNumber);
            return false;
        }

        // Get the variable name (which is all the characters up to the '=')
        auto varName = parts.Front();

        // Get the value (which is all the characters after the '=')
        auto value = parts.Back();

        if (varName.IEquals("!version"))
        {
            version = value.ToU32();
            return true;
        }

        try
        {
            switch (type)
            {
                case ParserTagType::Mesh:
                    ParseMesh(varName, value, cfg);
                    break;
                case ParserTagType::PointLight:
                    ParsePointLight(varName, value, cfg);
                    break;
                case ParserTagType::Scene:
                    ParseScene(varName, value, cfg);
                    break;
                case ParserTagType::Skybox:
                    ParseSkybox(varName, value, cfg);
                    break;
                case ParserTagType::DirectionalLight:
                    ParseDirectionalLight(varName, value, cfg);
                    break;
                case ParserTagType::Terrain:
                    ParseTerrain(varName, value, cfg);
                    break;
                default:
                    throw Exception("Unknown ParserTageType: '{}'.", ToUnderlying(type));
                    break;
            }
        }
        catch (const std::exception& exc)
        {
            ERROR_LOG("Failed to load file: '{}'. Error found on line: {} - {}.", fileName, lineNumber, exc.what());
            return false;
        }

        return true;
    }

    void ResourceManager<SceneConfig>::ParseScene(const String& name, const String& value, SceneConfig& cfg) const
    {
        if (name.IEquals("name"))
        {
            cfg.name = value;
        }
        else if (name.IEquals("description"))
        {
            cfg.description = value;
        }
        else
        {
            throw Exception("Unknown element: '{}' specified for Scene", name);
        }
    }

    void ResourceManager<SceneConfig>::ParseSkybox(const String& name, const String& value, SceneConfig& cfg) const
    {
        if (name.IEquals("name"))
        {
            cfg.skyboxConfig.name = value;
        }
        else if (name.IEquals("cubemapName"))
        {
            cfg.skyboxConfig.cubemapName = value;
        }
        else
        {
            throw Exception("Unknown element: '{}' specified for Skybox", name);
        }
    }

    void ResourceManager<SceneConfig>::ParseDirectionalLight(const String& name, const String& value, SceneConfig& cfg) const
    {
        if (name.IEquals("name"))
        {
            cfg.directionalLightConfig.name = value;
        }
        else if (name.IEquals("direction"))
        {
            cfg.directionalLightConfig.direction = value.ToVec4();
        }
        else if (name.IEquals("color"))
        {
            cfg.directionalLightConfig.color = value.ToVec4();
        }
        else if (name.IEquals("shadowDistance"))
        {
            cfg.directionalLightConfig.shadowDistance = value.ToF32();
        }
        else if (name.IEquals("shadowFadeDistance"))
        {
            cfg.directionalLightConfig.shadowFadeDistance = value.ToF32();
        }
        else if (name.IEquals("shadowSplitMultiplier"))
        {
            cfg.directionalLightConfig.shadowSplitMultiplier = value.ToF32();
        }
        else
        {
            throw Exception("Unknown element: '{}' specified for Directional Light", name);
        }
    }

    void ResourceManager<SceneConfig>::ParsePointLight(const String& name, const String& value, SceneConfig& cfg) const
    {
        auto& pointLight = cfg.pointLights.Back();

        if (name.IEquals("name"))
        {
            pointLight.name = value;
        }
        else if (name.IEquals("color"))
        {
            pointLight.color = value.ToVec4();
        }
        else if (name.IEquals("position"))
        {
            pointLight.position = value.ToVec4();
        }
        else if (name.IEquals("constant"))
        {
            pointLight.constant = value.ToF32();
        }
        else if (name.IEquals("linear"))
        {
            pointLight.linear = value.ToF32();
        }
        else if (name.IEquals("quadratic"))
        {
            pointLight.quadratic = value.ToF32();
        }
        else
        {
            throw Exception("Unknown element: '{}' specified for Point Light", name);
        }
    }

    void ResourceManager<SceneConfig>::ParseMesh(const String& name, const String& value, SceneConfig& cfg) const
    {
        auto& mesh = cfg.meshes.Back();

        if (name.IEquals("name"))
        {
            mesh.name = value;
        }
        else if (name.IEquals("resourcename"))
        {
            mesh.resourceName = value;
        }
        else if (name.IEquals("transform"))
        {
            mesh.transform = ParseTransform(value);
        }
        else if (name.IEquals("parent"))
        {
            mesh.parentName = value;
        }
        else
        {
            throw Exception("Unknown element: '{}' specified for Mesh", name);
        }
    }

    void ResourceManager<SceneConfig>::ParseTerrain(const String& name, const String& value, SceneConfig& cfg) const
    {
        auto& terrain = cfg.terrains.Back();

        if (name.IEquals("name"))
        {
            terrain.name = value;
        }
        else if (name.IEquals("transform"))
        {
            terrain.transform = ParseTransform(value);
        }
        else if (name.IEquals("resourcename"))
        {
            terrain.resourceName = value;
        }
        else
        {
            throw Exception("Unknown element: '{}' specified for Terrain", name);
        }
    }

    Handle<Transform> ResourceManager<SceneConfig>::ParseTransform(const String& value) const
    {
        const auto values = value.Split(' ');

        Handle<Transform> transform;

        if (values.Size() == 10)
        {
            vec3 pos   = { values[0].ToF32(), values[1].ToF32(), values[2].ToF32() };
            quat rot   = { values[6].ToF32(), values[3].ToF32(), values[4].ToF32(), values[5].ToF32() };
            vec3 scale = { values[7].ToF32(), values[8].ToF32(), values[9].ToF32() };
            transform  = Transforms.Acquire(pos, rot, scale);
        }
        else if (values.Size() == 9)
        {
            vec3 pos   = { values[0].ToF32(), values[1].ToF32(), values[2].ToF32() };
            vec3 rot   = { values[3].ToF32(), values[4].ToF32(), values[5].ToF32() };
            vec3 scale = { values[6].ToF32(), values[7].ToF32(), values[8].ToF32() };
            transform  = Transforms.Acquire(pos, rot, scale);
        }
        else
        {
            throw Exception(
                "Transform should have 10 values in the form px py pz qx qy qz qw sx sy sz (quaternion mode) "
                "or 9 values in the form of px py pz ex ey ez sx sy sz (euler angle mode) but it had {}",
                values.Size());
        }
        return transform;
    }

    ResourceManager<SceneConfig>::ParserTagType ResourceManager<SceneConfig>::ParseTag(const String& line, const String& fileName,
                                                                                       const u32 lineNumber, SceneConfig& cfg) const
    {
        static bool closeTag = true;
        String name;
        if (closeTag)
        {
            if (line[1] == '/')
            {
                ERROR_LOG("Failed to load file: '{}'. Expected an opening tag but found a closing tag at line: {}.", fileName, lineNumber);
                return ParserTagType::Invalid;
            }
            closeTag = false;
        }
        else
        {
            if (line[1] != '/')
            {
                ERROR_LOG("Failed to load file: '{}'. Expected a closing tag but found an opening tag at line: {}.", fileName, lineNumber);
                return ParserTagType::Invalid;
            }

            closeTag = true;
            return ParserTagType::Closing;
        }

        name = line.SubStr(1, line.Size() - 1);
        if (name.IEquals("scene"))
        {
            return ParserTagType::Scene;
        }
        else if (name.IEquals("skybox"))
        {
            return ParserTagType::Skybox;
        }
        else if (name.IEquals("directionallight"))
        {
            return ParserTagType::DirectionalLight;
        }
        else if (name.IEquals("mesh"))
        {
            cfg.meshes.EmplaceBack();  // Add a mesh which we will populate in the ParseTagContent method
            return ParserTagType::Mesh;
        }
        else if (name.IEquals("pointlight"))
        {
            cfg.pointLights.EmplaceBack();  // Add a point light which we will populate in the ParseTagContent method
            return ParserTagType::PointLight;
        }
        else if (name.IEquals("terrain"))
        {
            cfg.terrains.EmplaceBack();  // Add a terrain which we will popluate in the ParseTagContent method
            return ParserTagType::Terrain;
        }

        return ParserTagType::Invalid;
    }

}  // namespace C3D