
#include "shader_loader.h"

#include "core/engine.h"
#include "core/exceptions.h"
#include "platform/file_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    ResourceLoader<ShaderConfig>::ResourceLoader()
        : IResourceLoader(MemoryType::Shader, ResourceType::Shader, nullptr, "shaders"), BaseTextLoader<ShaderConfig>()
    {}

    bool ResourceLoader<ShaderConfig>::Load(const char* name, ShaderConfig& resource) const
    {
        m_currentTagType = ParserTagType::None;
        return LoadAndParseFile(name, "shaders", "shadercfg", resource);
    }

    void ResourceLoader<ShaderConfig>::Unload(ShaderConfig& resource)
    {
        // Cleanup stage configs
        resource.stageConfigs.Destroy();

        // Cleanup attributes
        resource.attributes.Destroy();

        // Cleanup uniforms
        resource.uniforms.Destroy();

        // Cleanup strings
        resource.name.Destroy();
        resource.fullPath.Destroy();

        // Set the version to 0 so the Config can be re-used (for loading the files it must start at version == 0)
        resource.version = 0;
    }

    void ResourceLoader<ShaderConfig>::SetDefaults(ShaderConfig& resource) const
    {
        if (resource.version != 2)
        {
            FATAL_LOG("We currently only support loading shadercfgs where version == 2");
        }

        resource.cullMode      = FaceCullMode::Back;
        resource.topologyTypes = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST;
    }

    void ResourceLoader<ShaderConfig>::ParseNameValuePair(const String& name, const String& value, ShaderConfig& resource) const
    {
        switch (m_currentTagType)
        {
            case ParserTagType::General:
                ParseGeneral(name, value, resource);
                break;
            case ParserTagType::Stages:
                ParseStages(name, value, resource);
                break;
            case ParserTagType::Attributes:
                ParseAttribute(name, value, resource);
                break;
            case ParserTagType::Uniforms:
                ParseUniform(name, value, resource);
                break;
            default:
                throw Exception("Invalid ParserTagType found: '{}'", ToUnderlying(m_currentTagType));
        }
    }

    void ResourceLoader<ShaderConfig>::ParseTag(const String& name, bool isOpeningTag, ShaderConfig& resource) const
    {
        if (isOpeningTag)
        {
            // Opening Tag
            if (name.IEquals("general"))
            {
                m_currentTagType = ParserTagType::General;
            }
            else if (name.IEquals("stages"))
            {
                m_currentTagType = ParserTagType::Stages;
            }
            else if (name.IEquals("attributes"))
            {
                m_currentTagType = ParserTagType::Attributes;
            }
            else if (name.IEquals("uniforms"))
            {
                m_currentTagType = ParserTagType::Uniforms;
            }
            else if (name.IEquals("global"))
            {
                if (m_currentTagType != ParserTagType::Uniforms)
                {
                    throw Exception("Tag name: {} may only appear inside of a uniforms tag.", name);
                }
                m_currentUniformScope = ParserUniformScope::Global;
            }
            else if (name.IEquals("instance"))
            {
                if (m_currentTagType != ParserTagType::Uniforms)
                {
                    throw Exception("Tag name: {} may only appear inside of a uniforms tag.", name);
                }
                m_currentUniformScope = ParserUniformScope::Instance;
            }
            else if (name.IEquals("local"))
            {
                if (m_currentTagType != ParserTagType::Uniforms)
                {
                    throw Exception("Tag name: {} may only appear inside of a uniforms tag.", name);
                }
                m_currentUniformScope = ParserUniformScope::Local;
            }
            else
            {
                throw Exception("Invalid Tag name: '{}'", name);
            }
        }
        else
        {
            // Closing Tag
            if (name.IEquals("general"))
            {
                if (m_currentTagType != ParserTagType::General)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type general", name);
                }
                m_currentTagType = ParserTagType::None;
            }
            else if (name.IEquals("stages"))
            {
                if (m_currentTagType != ParserTagType::Stages)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type stages", name);
                }
                m_currentTagType = ParserTagType::None;
            }
            else if (name.IEquals("attributes"))
            {
                if (m_currentTagType != ParserTagType::Attributes)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type attributes", name);
                }
                m_currentTagType = ParserTagType::None;
            }
            else if (name.IEquals("uniforms"))
            {
                if (m_currentTagType != ParserTagType::Uniforms)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type uniforms", name);
                }
                m_currentTagType = ParserTagType::None;
            }
            else if (name.IEquals("global"))
            {
                if (m_currentUniformScope != ParserUniformScope::Global)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type global", name);
                }
                m_currentUniformScope = ParserUniformScope::None;
            }
            else if (name.IEquals("instance"))
            {
                if (m_currentUniformScope != ParserUniformScope::Instance)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type instance", name);
                }
                m_currentUniformScope = ParserUniformScope::None;
            }
            else if (name.IEquals("local"))
            {
                if (m_currentUniformScope != ParserUniformScope::Local)
                {
                    throw Exception("Invalid closing Tag name: '{}' expected type local", name);
                }
                m_currentUniformScope = ParserUniformScope::None;
            }
            else
            {
                throw Exception("Invalid Tag name: '{}'", name);
            }
        }
    }

    void ResourceLoader<ShaderConfig>::ParseGeneral(const String& name, const String& value, ShaderConfig& resource) const
    {
        if (name.IEquals("name"))
        {
            resource.name = value;
        }
        else if (name.IEquals("renderpass"))
        {
            WARN_LOG("ShaderCfg contains deprecated keyword: 'renderPass' which is ignored.");
        }
        else if (name.IEquals("maxInstances"))
        {
            resource.maxInstances = value.ToU32();
        }
        else if (name.IEquals("depthTest"))
        {
            if (value.ToBool()) resource.flags |= ShaderFlagDepthTest;
        }
        else if (name.IEquals("stencilTest"))
        {
            if (value.ToBool()) resource.flags |= ShaderFlagStencilTest;
        }
        else if (name.IEquals("depthWrite"))
        {
            if (value.ToBool()) resource.flags |= ShaderFlagDepthWrite;
        }
        else if (name.IEquals("stencilWrite"))
        {
            if (value.ToBool()) resource.flags |= ShaderFlagStencilWrite;
        }
        else if (name.IEquals("supportsWireframe"))
        {
            if (value.ToBool()) resource.flags |= ShaderFlagWireframe;
        }
        else if (name.IEquals("topology"))
        {
            // Reset our topology types
            resource.topologyTypes = PRIMITIVE_TOPOLOGY_TYPE_NONE;
            // Parse them as a comma-seperated list
            const auto topologyTypes = value.Split(',');
            // Add them to our config one by one
            for (auto topology : topologyTypes)
            {
                ParseTopology(resource, topology);
            }
        }
        else if (name.IEquals("cullMode"))
        {
            ParseCullMode(resource, value);
        }
        else
        {
            throw Exception("Unknown specifier: '{}' found in [general] section", value);
        }
    }

    void ResourceLoader<ShaderConfig>::ParseStages(const String& name, const String& value, ShaderConfig& resource) const
    {
        ShaderStageConfig config;
        config.fileName = "shaders/" + value;
        config.name     = FileSystem::FileNameFromPath(config.fileName);

        if (name.IEquals("vert") || name.IEquals("vertex"))
        {
            config.stage = ShaderStage::Vertex;
        }
        else if (name.IEquals("frag") || name.IEquals("fragment"))
        {
            config.stage = ShaderStage::Fragment;
        }
        else
        {
            throw Exception("Unknown ShaderStage: '{}' specified", name);
        }

        resource.stageConfigs.PushBack(config);
    }

    void ResourceLoader<ShaderConfig>::ParseAttribute(const String& name, const String& value, ShaderConfig& resource) const
    {
        ShaderAttributeConfig attribute;
        attribute.name = name;

        if (value.IEquals("f32"))
        {
            attribute.type = Attribute_Float32;
            attribute.size = 4;
        }
        else if (value.IEquals("vec2"))
        {
            attribute.type = Attribute_Float32_2;
            attribute.size = 8;
        }
        else if (value.IEquals("vec3"))
        {
            attribute.type = Attribute_Float32_3;
            attribute.size = 12;
        }
        else if (value.IEquals("vec4"))
        {
            attribute.type = Attribute_Float32_4;
            attribute.size = 16;
        }
        else if (value.IEquals("u8"))
        {
            attribute.type = Attribute_UInt8;
            attribute.size = 1;
        }
        else if (value.IEquals("u16"))
        {
            attribute.type = Attribute_UInt16;
            attribute.size = 2;
        }
        else if (value.IEquals("u32"))
        {
            attribute.type = Attribute_UInt32;
            attribute.size = 4;
        }
        else if (value.IEquals("i8"))
        {
            attribute.type = Attribute_Int8;
            attribute.size = 1;
        }
        else if (value.IEquals("i16"))
        {
            attribute.type = Attribute_Int16;
            attribute.size = 2;
        }
        else if (value.IEquals("i32"))
        {
            attribute.type = Attribute_Int32;
            attribute.size = 4;
        }
        else
        {
            throw Exception("Unknown attribute type: '{}'", value);
        }

        resource.attributes.PushBack(attribute);
    }

    void ResourceLoader<ShaderConfig>::ParseUniform(const String& name, const String& value, ShaderConfig& resource) const
    {
        ShaderUniformConfig uniform;
        uniform.name = name;
        switch (m_currentUniformScope)
        {
            case ParserUniformScope::Global:
                uniform.scope = ShaderScope::Global;
                break;
            case ParserUniformScope::Instance:
                uniform.scope = ShaderScope::Instance;
                break;
            case ParserUniformScope::Local:
                uniform.scope = ShaderScope::Local;
                break;
            default:
                throw Exception("Invalid scope defined for current uniforms");
        }

        String type;

        // Check if it's an array type
        if (value.Contains('['))
        {
            // Array-type
            auto openBracket    = value.Find('[');
            auto closeBracket   = value.begin() + value.Size() - 1;
            auto lengthStr      = value.SubStr(openBracket + 1, closeBracket);
            uniform.arrayLength = lengthStr.ToU8();
            type                = value.SubStr(value.begin(), openBracket);
        }
        else
        {
            // Not an array-type
            uniform.arrayLength = 1;
            type                = value;
        }

        if (type.IEquals("f32"))
        {
            uniform.type = Uniform_Float32;
            uniform.size = 4;
        }
        else if (type.IEquals("vec2"))
        {
            uniform.type = Uniform_Float32_2;
            uniform.size = 8;
        }
        else if (type.IEquals("vec3"))
        {
            uniform.type = Uniform_Float32_3;
            uniform.size = 12;
        }
        else if (type.IEquals("vec4"))
        {
            uniform.type = Uniform_Float32_4;
            uniform.size = 16;
        }
        else if (type.IEquals("u8"))
        {
            uniform.type = Uniform_UInt8;
            uniform.size = 1;
        }
        else if (type.IEquals("u16"))
        {
            uniform.type = Uniform_UInt16;
            uniform.size = 2;
        }
        else if (type.IEquals("u32"))
        {
            uniform.type = Uniform_UInt32;
            uniform.size = 4;
        }
        else if (type.IEquals("i8"))
        {
            uniform.type = Uniform_Int8;
            uniform.size = 1;
        }
        else if (type.IEquals("i16"))
        {
            uniform.type = Uniform_Int16;
            uniform.size = 2;
        }
        else if (type.IEquals("i32"))
        {
            uniform.type = Uniform_Int32;
            uniform.size = 4;
        }
        else if (type.IEquals("mat4"))
        {
            uniform.type = Uniform_Matrix4;
            uniform.size = 64;
        }
        else if (type.StartsWithI("samp"))
        {
            // Some kind of sampler
            uniform.size = 0;  // Samplers don't have a size.

            if (type.IEquals("sampler1d"))
            {
                uniform.type = Uniform_Sampler1D;
            }
            else if (type.IEquals("sampler2d") || type.IEquals("samp") || type.IEquals("sampler"))
            {
                // NOTE: Also checking here for samp/sampler for backwards compatibility
                uniform.type = Uniform_Sampler2D;
            }
            else if (type.IEquals("sampler3d"))
            {
                uniform.type = Uniform_Sampler3D;
            }
            else if (type.IEquals("samplercube"))
            {
                uniform.type = Uniform_SamplerCube;
            }
            else if (type.IEquals("sampler1darray"))
            {
                uniform.type = Uniform_Sampler1DArray;
            }
            else if (type.IEquals("sampler2darray"))
            {
                uniform.type = Uniform_Sampler2DArray;
            }
            else if (type.IEquals("samplercubearray"))
            {
                uniform.type = Uniform_SamplerCubeArray;
            }
            else
            {
                throw Exception("Unknown sampler type: '{}'.", type);
            }
        }
        else if (type.StartsWithI("struct"))
        {
            // Type is struct
            if (type.Size() <= 6)
            {
                throw Exception("Invalid struct type: '{}'.", type);
            }

            String structSize = type.SubStr(6, 0);
            uniform.type      = Uniform_Custom;
            uniform.size      = structSize.ToU16();
        }
        else
        {
            throw Exception("Unknown uniform type: '{}'", type);
        }

        resource.uniforms.PushBack(uniform);
    }

    void ResourceLoader<ShaderConfig>::ParseTopology(ShaderConfig& data, const String& value)
    {
        if (value.IEquals("triangleList"))
        {
            data.topologyTypes |= PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST;
        }
        else if (value.IEquals("triangleStrip"))
        {
            data.topologyTypes |= PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP;
        }
        else if (value.IEquals("triangleFan"))
        {
            data.topologyTypes |= PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN;
        }
        else if (value.IEquals("lineList"))
        {
            data.topologyTypes |= PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST;
        }
        else if (value.IEquals("lineStrip"))
        {
            data.topologyTypes |= PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP;
        }
        else if (value.IEquals("pointList"))
        {
            data.topologyTypes |= PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST;
        }
        else
        {
            throw Exception("Invalid topology type: '{}'", value);
        }
    }

    void ResourceLoader<ShaderConfig>::ParseCullMode(ShaderConfig& data, const String& value)
    {
        if (value.IEquals("front"))
        {
            data.cullMode = FaceCullMode::Front;
        }
        else if (value.IEquals("front_and_back"))
        {
            data.cullMode = FaceCullMode::FrontAndBack;
        }
        else if (value.IEquals("none"))
        {
            data.cullMode = FaceCullMode::None;
        }
        // Default is BACK so we don't have to do anything
    }
}  // namespace C3D
