
#include "material_loader.h"

#include "core/engine.h"
#include "core/logger.h"
#include "platform/file_system.h"
#include "resources/materials/material.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    ResourceLoader<MaterialConfig>::ResourceLoader()
        : IResourceLoader(MemoryType::MaterialInstance, ResourceType::Material, nullptr, "materials"), BaseTextLoader<MaterialConfig>()
    {}

    bool ResourceLoader<MaterialConfig>::Load(const char* name, MaterialConfig& resource) const
    {
        m_currentTagType = ParserTagType::Global;
        return LoadAndParseFile(name, "materials", "mt", resource);
    }

    void ResourceLoader<MaterialConfig>::Unload(MaterialConfig& resource)
    {
        resource.shaderName.Destroy();
        resource.fullPath.Destroy();
        resource.name.Destroy();
    }

    void ResourceLoader<MaterialConfig>::SetDefaults(MaterialConfig& resource) const
    {
        if (resource.version == 1)
        {
            resource.type = MaterialType::Phong;
        }
    }

    void ResourceLoader<MaterialConfig>::ParseNameValuePair(const String& name, const String& value, MaterialConfig& resource) const
    {
        if (resource.version == 1)
        {
            ParseGlobal(name, value, resource);
        }
        else
        {
            switch (m_currentTagType)
            {
                case ParserTagType::Global:
                    ParseGlobalV2(name, value, resource);
                    break;
                case ParserTagType::Map:
                    ParseMap(name, value, resource);
                    break;
                case ParserTagType::Prop:
                    ParseProp(name, value, resource);
                    break;
                default:
                    throw Exception("Invalid ParserTagType found: '{}'", ToUnderlying(m_currentTagType));
            }
        }
    }

    void ResourceLoader<MaterialConfig>::ParseGlobal(const String& name, const String& value, MaterialConfig& resource) const
    {
        if (name.IEquals("type"))
        {
            if (value.IEquals("phong"))
            {
                resource.type = MaterialType::Phong;
            }
            else if (value.IEquals("pbr"))
            {
                resource.type = MaterialType::PBR;
            }
            else if (value.IEquals("ui"))
            {
                resource.type = MaterialType::UI;
            }
            else if (value.IEquals("terrain"))
            {
                resource.type = MaterialType::Terrain;
            }
            else if (value.IEquals("custom"))
            {
                resource.type = MaterialType::Custom;
            }
            else
            {
                throw Exception("Unknown Material type: '{}'", value);
            }
        }
        else if (name.IEquals("name"))
        {
            resource.name = value;
        }
        else if (name.IEquals("shader"))
        {
            resource.shaderName = value;
        }
        else if (name.IEquals("diffuseColor"))
        {
            resource.props.EmplaceBack(MaterialConfigProp("diffuseColor", Uniform_Float32_4, value.ToVec4()));
        }
        else if (name.IEquals("shininess"))
        {
            resource.props.EmplaceBack(MaterialConfigProp("shininess", Uniform_Float32, value.ToF32()));
        }
        else if (name.IEquals("diffuseMapName"))
        {
            resource.maps.EmplaceBack(MaterialConfigMap("diffuse", value));
        }
        else if (name.IEquals("specularMapName"))
        {
            resource.maps.EmplaceBack(MaterialConfigMap("specular", value));
        }
        else if (name.IEquals("normalMapName"))
        {
            resource.maps.EmplaceBack(MaterialConfigMap("normal", value));
        }
        else
        {
            throw Exception("Unknown name found: '{}'", name);
        }
    }

    void ResourceLoader<MaterialConfig>::ParseGlobalV2(const String& name, const String& value, MaterialConfig& resource) const
    {
        if (name.IEquals("type"))
        {
            if (value.IEquals("phong"))
            {
                resource.type = MaterialType::Phong;
            }
            else if (value.IEquals("pbr"))
            {
                resource.type       = MaterialType::PBR;
                resource.shaderName = "Shader.PBR";
            }
            else if (value.IEquals("custom"))
            {
                resource.type = MaterialType::Custom;
            }
            else if (value.IEquals("ui"))
            {
                resource.type = MaterialType::UI;
            }
            else
            {
                throw Exception("Unknown Material type: '{}'", value);
            }
        }
        else if (name.IEquals("name"))
        {
            resource.name = value;
        }
        else if (name.IEquals("shader"))
        {
            resource.shaderName = value;
        }
        else
        {
            throw Exception("Unknown name found: '{}'", name);
        }
    }

    void ResourceLoader<MaterialConfig>::ParseTag(const String& name, bool isOpeningTag, MaterialConfig& resource) const
    {
        if (isOpeningTag)
        {
            // Opening Tag
            if (name.IEquals("map"))
            {
                m_currentTagType = ParserTagType::Map;
                resource.maps.EmplaceBack();
            }
            else if (name.IEquals("prop"))
            {
                m_currentTagType = ParserTagType::Prop;
                resource.props.EmplaceBack();
            }
            else
            {
                throw Exception("Invalid Tag name: '{}'", name);
            }
        }
        else
        {
            // Closing Tag
            if (name.IEquals("map"))
            {
                if (m_currentTagType != ParserTagType::Map)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type Map", name);
                }
            }
            else if (name.IEquals("prop"))
            {
                if (m_currentTagType != ParserTagType::Prop)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type Prop", name);
                }
            }
            else
            {
                throw Exception("Invalid Tag name: '{}'", name);
            }

            m_currentTagType = ParserTagType::Global;
        }
    }

    void ParseTextureFilter(const String& value, TextureFilter& filter)
    {
        if (value.IEquals("linear"))
        {
            filter = TextureFilter::ModeLinear;
        }
        else if (value.IEquals("nearest"))
        {
            filter = TextureFilter::ModeNearest;
        }
        else
        {
            throw Exception("Unknown TextureFilter type: '{}'", value);
        }
    }

    void ParseTextureRepeat(const String& value, TextureRepeat& repeat)
    {
        if (value.IEquals("Repeat"))
        {
            repeat = TextureRepeat::Repeat;
        }
        else if (value.IEquals("MirroredRepeat"))
        {
            repeat = TextureRepeat::MirroredRepeat;
        }
        else if (value.IEquals("ClampToEdge"))
        {
            repeat = TextureRepeat::ClampToEdge;
        }
        else if (value.IEquals("ClampToBorder"))
        {
            repeat = TextureRepeat::ClampToBorder;
        }
        else
        {
            throw Exception("Unknown TextureRepeat type: '{}'", value);
        }
    }

    void ResourceLoader<MaterialConfig>::ParseMap(const String& name, const String& value, MaterialConfig& resource) const
    {
        auto& map = resource.maps.Back();

        if (name.IEquals("name"))
        {
            map.name = value;
        }
        else if (name.IEquals("filterMin"))
        {
            ParseTextureFilter(value, map.minifyFilter);
        }
        else if (name.IEquals("filterMag"))
        {
            ParseTextureFilter(value, map.magnifyFilter);
        }
        else if (name.StartsWith("repeat"))
        {
            auto dir = name.Last();
            if (dir == 'U')
            {
                ParseTextureRepeat(value, map.repeatU);
            }
            else if (dir == 'V')
            {
                ParseTextureRepeat(value, map.repeatV);
            }
            else if (dir == 'W')
            {
                ParseTextureRepeat(value, map.repeatW);
            }
            else
            {
                throw Exception("Unknown repeat direction for texture: '{}'", dir);
            }
        }
        else if (name.IEquals("textureName"))
        {
            map.textureName = value;
        }
        else
        {
            throw Exception("Invalid property found in Map: '{}'", name);
        }
    }

    void ResourceLoader<MaterialConfig>::ParseProp(const String& name, const String& value, MaterialConfig& resource) const
    {
        auto& prop = resource.props.Back();

        if (name.IEquals("name"))
        {
            prop.name = value;
        }
        else if (name.IEquals("type"))
        {
            if (value.IEquals("f32"))
            {
                prop.type = ShaderUniformType::Uniform_Float32;
                prop.size = sizeof(f32);
            }
            else if (value.IEquals("vec2"))
            {
                prop.type = ShaderUniformType::Uniform_Float32_2;
                prop.size = sizeof(vec2);
            }
            else if (value.IEquals("vec3"))
            {
                prop.type = ShaderUniformType::Uniform_Float32_3;
                prop.size = sizeof(vec3);
            }
            else if (value.IEquals("vec4"))
            {
                prop.type = ShaderUniformType::Uniform_Float32_4;
                prop.size = sizeof(vec4);
            }

            else
            {
                throw Exception("Invalid type for Prop: '{}'", value);
            }
        }
        else if (name.IEquals("value"))
        {
            if (prop.type == ShaderUniformType::Uniform_Float32)
            {
                prop.value = value.ToF32();
            }
            else if (prop.type == ShaderUniformType::Uniform_Float32_2)
            {
                prop.value = value.ToVec2();
            }
            else if (prop.type == ShaderUniformType::Uniform_Float32_3)
            {
                prop.value = value.ToVec3();
            }
            else if (prop.type == ShaderUniformType::Uniform_Float32_4)
            {
                prop.value = value.ToVec4();
            }
            else
            {
                throw Exception("Unknown type: '{}' for Prop while trying to parse Value", ToUnderlying(prop.type));
            }
        }
        else
        {
            throw Exception("Invalid property found in Prop: '{}'", name);
        }
    }
}  // namespace C3D
