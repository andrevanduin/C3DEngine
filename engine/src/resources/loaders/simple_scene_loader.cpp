
#include "simple_scene_loader.h"

#include "core/engine.h"
#include "platform/filesystem.h"
#include "resources/shader.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr auto FILE_EXTENSION = "csimplescenecfg";

    ResourceLoader<SimpleSceneConfig>::ResourceLoader(const SystemManager* pSystemsManager)
        : IResourceLoader(pSystemsManager, "SIMPLE_SCENE_LOADER", MemoryType::Scene, ResourceType::SimpleScene, nullptr,
                          "scenes")
    {}

    bool ResourceLoader<SimpleSceneConfig>::Load(const char* name, SimpleSceneConfig& resource) const
    {
        if (std::strlen(name) == 0)
        {
            m_logger.Error("Load() - Provided name was empty");
            return false;
        }

        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, FILE_EXTENSION);
        auto fileName = String::FromFormat("{}.{}", name, FILE_EXTENSION);

        File file;
        if (!file.Open(fullPath, FileModeRead))
        {
            m_logger.Error("Load() - Failed to open simple scene config file for reading: '{}'", fullPath);
            return false;
        }

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
                m_logger.Error(
                    "Load() - Failed to load file: '{}'. Simple scene config should start with !version = <parser "
                    "version>: ",
                    fullPath);
                return false;
            }

            if (line.StartsWith("["))
            {
                type = ParseTag(line, fileName, lineNumber, resource);
                if (type == ParserTagType::Invalid)
                {
                    m_logger.Error("Load() - Failed to load file: '{}'. Unknown tag: '{}' found on line: {}", fileName,
                                   line, lineNumber);
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

    void ResourceLoader<SimpleSceneConfig>::Unload(SimpleSceneConfig& resource) const
    {
        resource.name.Clear();
        resource.description.Clear();
        resource.fullPath.Clear();
        resource.pointLights.Clear();
        resource.meshes.Clear();
    }

    bool ResourceLoader<SimpleSceneConfig>::ParseTagContent(const String& line, const String& fileName,
                                                            const u32 lineNumber, u32& version,
                                                            const ParserTagType type, SimpleSceneConfig& cfg) const
    {
        // Check if we have a '=' symbol
        if (!line.Contains('='))
        {
            m_logger.Warn("Potential formatting issue found in file '{}': '=' token not found. Skipping line {}.",
                          fileName, lineNumber);
            return false;
        }

        auto parts = line.Split('=');
        if (parts.Size() != 2)
        {
            m_logger.Warn("Potential formatting issue found in file '{}': Too many '=' tokens found. Skipping line {}.",
                          fileName, lineNumber);
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
                    throw std::exception(
                        String::FromFormat("Unknown ParserTageType: '{}'.", ToUnderlying(type)).Data());
                    break;
            }
        }
        catch (const std::exception& exc)
        {
            m_logger.Error("Load() - Failed to load file: '{}'. Error found on line {}: {}", fileName, lineNumber,
                           exc.what());
            return false;
        }

        return true;
    }

    void ResourceLoader<SimpleSceneConfig>::ParseScene(const String& name, const String& value,
                                                       SimpleSceneConfig& cfg) const
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
            throw std::exception(String::FromFormat("Unknown element: '{}' specified for Scene", name).Data());
        }
    }

    void ResourceLoader<SimpleSceneConfig>::ParseSkybox(const String& name, const String& value,
                                                        SimpleSceneConfig& cfg) const
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
            throw std::exception(String::FromFormat("Unknown element: '{}' specified for Skybox", name).Data());
        }
    }

    void ResourceLoader<SimpleSceneConfig>::ParseDirectionalLight(const String& name, const String& value,
                                                                  SimpleSceneConfig& cfg) const
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
        else
        {
            throw std::exception(
                String::FromFormat("Unknown element: '{}' specified for Directional Light", name).Data());
        }
    }

    void ResourceLoader<SimpleSceneConfig>::ParsePointLight(const String& name, const String& value,
                                                            SimpleSceneConfig& cfg) const
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
            throw std::exception(String::FromFormat("Unknown element: '{}' specified for Point Light", name).Data());
        }
    }

    void ResourceLoader<SimpleSceneConfig>::ParseMesh(const String& name, const String& value,
                                                      SimpleSceneConfig& cfg) const
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
            throw std::exception(String::FromFormat("Unknown element: '{}' specified for Mesh", name).Data());
        }
    }

    void ResourceLoader<SimpleSceneConfig>::ParseTerrain(const String& name, const String& value,
                                                         SimpleSceneConfig& cfg) const
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
            throw std::exception(String::FromFormat("Unknown element: '{}' specified for Terrain", name).Data());
        }
    }

    Transform ResourceLoader<SimpleSceneConfig>::ParseTransform(const String& value) const
    {
        const auto values = value.Split(' ');
        Transform transform;

        if (values.Size() == 10)
        {
            vec3 pos   = { values[0].ToF32(), values[1].ToF32(), values[2].ToF32() };
            quat rot   = { values[6].ToF32(), values[3].ToF32(), values[4].ToF32(), values[5].ToF32() };
            vec3 scale = { values[7].ToF32(), values[8].ToF32(), values[9].ToF32() };
            transform.SetPositionRotationScale(pos, rot, scale);
        }
        else if (values.Size() == 9)
        {
            vec3 pos   = { values[0].ToF32(), values[1].ToF32(), values[2].ToF32() };
            vec3 rot   = { values[3].ToF32(), values[4].ToF32(), values[5].ToF32() };
            vec3 scale = { values[6].ToF32(), values[7].ToF32(), values[8].ToF32() };
            transform.SetPositionRotationScale(pos, rot, scale);
        }
        else
        {
            throw std::exception(
                String::FromFormat(
                    "Transform should have 10 values in the form px py pz qx qy qz qw sx sy sz (quaternion mode) "
                    "or 9 values in the form of px py pz ex ey ez sx sy sz (euler angle mode) but it had {}",
                    values.Size())
                    .Data());
        }
        return transform;
    }

    ParserTagType ResourceLoader<SimpleSceneConfig>::ParseTag(const String& line, const String& fileName,
                                                              const u32 lineNumber, SimpleSceneConfig& cfg) const
    {
        static bool closeTag = true;
        String name;
        if (closeTag)
        {
            if (line[1] == '/')
            {
                m_logger.Error(
                    "Load() - Failed to load file: '{}'. Expected an opening tag but found a closing tag at line: {}",
                    fileName, lineNumber);
                return ParserTagType::Invalid;
            }
            closeTag = false;
        }
        else
        {
            if (line[1] != '/')
            {
                m_logger.Error(
                    "Load() - Failed to load file: '{}'. Expected a closing tag but found an opening tag at line: {}",
                    fileName, lineNumber);
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