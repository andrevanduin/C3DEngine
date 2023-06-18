
#include "shader_loader.h"

#include "core/engine.h"
#include "platform/filesystem.h"
#include "resources/shader.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    ResourceLoader<ShaderResource>::ResourceLoader(const Engine* engine)
        : IResourceLoader(engine, "SHADER_LOADER", MemoryType::Shader, ResourceType::Shader, nullptr, "shaders")
    {
    }

    bool ResourceLoader<ShaderResource>::Load(const char* name, ShaderResource& resource) const
    {
        if (std::strlen(name) == 0)
        {
            m_logger.Error("Load() - Provided name was empty or no outResource pointer provided");
            return false;
        }

        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, "shadercfg");

        File file;
        if (!file.Open(fullPath, FileModeRead))
        {
            m_logger.Error("Load() - Failed to open shader config file for reading: '{}'", fullPath);
            return false;
        }

        resource.fullPath = fullPath;
        resource.config.cullMode = FaceCullMode::Back;

        resource.config.attributes.Reserve(4);
        resource.config.uniforms.Reserve(8);

        String line;
        u32 lineNumber = 1;
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

            // Check if we have a '=' symbol
            if (!line.Contains('='))
            {
                m_logger.Warn("Potential formatting issue found in file '{}': '=' token not found. Skipping line {}.",
                              fullPath, lineNumber);
                lineNumber++;
                continue;
            }

            auto parts = line.Split('=');
            if (parts.Size() != 2)
            {
                m_logger.Warn(
                    "Potential formatting issue found in file '{}': Too many '=' tokens found. Skipping line {}.",
                    fullPath, lineNumber);
                lineNumber++;
                continue;
            }

            // Get the variable name (which is all the characters up to the '=' and trim
            auto varName = parts[0];
            varName.Trim();

            // Get the value (which is all the characters after the '=' and trim
            auto value = parts[1];
            value.Trim();

            if (varName.IEquals("version"))
            {
                // TODO: version
            }
            else if (varName.IEquals("name"))
            {
                resource.config.name = value;
            }
            else if (varName.IEquals("renderPass"))
            {
                // NOTE: Deprecated so we ignore it from now on
                // outResource->config.renderPassName = StringDuplicate(value.data());
            }
            else if (varName.IEquals("depthTest"))
            {
                resource.config.depthTest = value.ToBool();
            }
            else if (varName.IEquals("depthWrite"))
            {
                resource.config.depthWrite = value.ToBool();
            }
            else if (varName.IEquals("stages"))
            {
                ParseStages(resource.config, value);
            }
            else if (varName.IEquals("stageFiles"))
            {
                ParseStageFiles(resource.config, value);
            }
            else if (varName.IEquals("cullMode"))
            {
                ParseCullMode(resource.config, value);
            }
            else if (varName.IEquals("attribute"))
            {
                ParseAttribute(resource.config, value);
            }
            else if (varName.IEquals("uniform"))
            {
                ParseUniform(resource.config, value);
            }
            else if (varName.IEquals("topology"))
            {
                ParseTopology(resource.config, value);
            }

            lineNumber++;
        }

        file.Close();
        return true;
    }

    void ResourceLoader<ShaderResource>::Unload(ShaderResource& resource) const
    {
        auto& data = resource.config;

        data.stageFileNames.Destroy();
        data.stageNames.Destroy();
        data.stages.Destroy();

        // Cleanup attributes
        data.attributes.Destroy();

        // Cleanup uniforms
        data.uniforms.Destroy();

        // Memory.Free(data->renderPassName, StringLength(data->renderPassName) + 1, MemoryType::String);
        // data->renderPassName = nullptr;

        data.name.Destroy();

        resource.name.Destroy();
        resource.fullPath.Destroy();
    }

    void ResourceLoader<ShaderResource>::ParseStages(ShaderConfig& data, const String& value) const
    {
        data.stageNames = value.Split(',');
        if (!data.stageFileNames.Empty() && data.stageNames.Size() != data.stageFileNames.Size())
        {
            // We already found the stage file names and the lengths don't match
            m_logger.Error("ParseStages() - Mismatch between the amount of StageNames ({}) and StageFileNames ({})",
                           data.stageNames.Size(), data.stageFileNames.Size());
        }

        for (auto& stageName : data.stageNames)
        {
            if (stageName.IEquals("frag") || stageName.IEquals("fragment"))
            {
                data.stages.PushBack(ShaderStage::Fragment);
            }
            else if (stageName.IEquals("vert") || stageName.IEquals("vertex"))
            {
                data.stages.PushBack(ShaderStage::Vertex);
            }
            else if (stageName.IEquals("geom") || stageName.IEquals("geometry"))
            {
                data.stages.PushBack(ShaderStage::Geometry);
            }
            else if (stageName.IEquals("comp") || stageName.IEquals("compute"))
            {
                data.stages.PushBack(ShaderStage::Compute);
            }
            else
            {
                m_logger.Error("ParseStages() - Unrecognized stage '{}'", stageName);
            }
        }
    }

    void ResourceLoader<ShaderResource>::ParseStageFiles(ShaderConfig& data, const String& value) const
    {
        data.stageFileNames = value.Split(',');
        if (!data.stageNames.Empty() && data.stageNames.Size() != data.stageFileNames.Size())
        {
            // We already found the stage names and the lengths don't match
            m_logger.Error("ParseStages() - Mismatch between the amount of StageNames ({}) and StageFileNames ({})",
                           data.stageNames.Size(), data.stageFileNames.Size());
        }
    }

    void ResourceLoader<ShaderResource>::ParseAttribute(ShaderConfig& data, const String& value) const
    {
        const auto fields = value.Split(',');
        if (fields.Size() != 2)
        {
            m_logger.Error(
                "ParseAttribute() - Invalid layout. Attribute field must be 'type,name'. Skipping this line.");
            return;
        }

        ShaderAttributeConfig attribute{};
        if (fields[0].IEquals("f32"))
        {
            attribute.type = Attribute_Float32;
            attribute.size = 4;
        }
        else if (fields[0].IEquals("vec2"))
        {
            attribute.type = Attribute_Float32_2;
            attribute.size = 8;
        }
        else if (fields[0].IEquals("vec3"))
        {
            attribute.type = Attribute_Float32_3;
            attribute.size = 12;
        }
        else if (fields[0].IEquals("vec4"))
        {
            attribute.type = Attribute_Float32_4;
            attribute.size = 16;
        }
        else if (fields[0].IEquals("u8"))
        {
            attribute.type = Attribute_UInt8;
            attribute.size = 1;
        }
        else if (fields[0].IEquals("u16"))
        {
            attribute.type = Attribute_UInt16;
            attribute.size = 2;
        }
        else if (fields[0].IEquals("u32"))
        {
            attribute.type = Attribute_UInt32;
            attribute.size = 4;
        }
        else if (fields[0].IEquals("i8"))
        {
            attribute.type = Attribute_Int8;
            attribute.size = 1;
        }
        else if (fields[0].IEquals("i16"))
        {
            attribute.type = Attribute_Int16;
            attribute.size = 2;
        }
        else if (fields[0].IEquals("i32"))
        {
            attribute.type = Attribute_Int32;
            attribute.size = 4;
        }
        else
        {
            m_logger.Error(
                "ParseAttribute() - Invalid file layout. Attribute type must be be f32, vec2, vec3, vec4, i8, i16, "
                "i32, u8, u16, or u32.");
            m_logger.Warn("Defaulting to f32.");
            attribute.type = Attribute_Float32;
            attribute.size = 4;
        }
        attribute.name = fields[1];

        data.attributes.PushBack(attribute);
    }

    bool ResourceLoader<ShaderResource>::ParseUniform(ShaderConfig& data, const String& value) const
    {
        const auto fields = value.Split(',');
        if (fields.Size() != 3)
        {
            m_logger.Error("ParseAttribute() - Invalid layout. Uniform field must be 'type,scope,name'.");
            return false;
        }

        ShaderUniformConfig uniform{};
        if (fields[0].IEquals("f32"))
        {
            uniform.type = Uniform_Float32;
            uniform.size = 4;
        }
        else if (fields[0].IEquals("vec2"))
        {
            uniform.type = Uniform_Float32_2;
            uniform.size = 8;
        }
        else if (fields[0].IEquals("vec3"))
        {
            uniform.type = Uniform_Float32_3;
            uniform.size = 12;
        }
        else if (fields[0].IEquals("vec4"))
        {
            uniform.type = Uniform_Float32_4;
            uniform.size = 16;
        }
        else if (fields[0].IEquals("u8"))
        {
            uniform.type = Uniform_UInt8;
            uniform.size = 1;
        }
        else if (fields[0].IEquals("u16"))
        {
            uniform.type = Uniform_UInt16;
            uniform.size = 2;
        }
        else if (fields[0].IEquals("u32"))
        {
            uniform.type = Uniform_UInt32;
            uniform.size = 4;
        }
        else if (fields[0].IEquals("i8"))
        {
            uniform.type = Uniform_Int8;
            uniform.size = 1;
        }
        else if (fields[0].IEquals("i16"))
        {
            uniform.type = Uniform_Int16;
            uniform.size = 2;
        }
        else if (fields[0].IEquals("i32"))
        {
            uniform.type = Uniform_Int32;
            uniform.size = 4;
        }
        else if (fields[0].IEquals("mat4"))
        {
            uniform.type = Uniform_Matrix4;
            uniform.size = 64;
        }
        else if (fields[0].IEquals("samp") || fields[0].IEquals("sampler"))
        {
            uniform.type = Uniform_Sampler;
            uniform.size = 0;  // Samplers don't have a size.
        }
        else if (fields[0].NIEquals("struct", 6))
        {
            // Type is struct
            if (fields[0].Size() <= 6)
            {
                m_logger.Error("ParseUniform() - Invalid struct type: {}", fields[0]);
                return false;
            }
            String structSize = fields[0].SubStr(6, 0);
            uniform.type = Uniform_Custom;
            uniform.size = structSize.ToU16();
        }
        else
        {
            m_logger.Error(
                "ParseUniform(). Invalid file layout. Uniform type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, "
                "u16, u32, mat4, samp/sampler or struct.");
            m_logger.Warn("Defaulting to f32.");
            uniform.type = Uniform_Float32;
            uniform.size = 4;
        }

        if (fields[1].IEquals("global"))
        {
            uniform.scope = ShaderScope::Global;
        }
        else if (fields[1].IEquals("instance"))
        {
            uniform.scope = ShaderScope::Instance;
        }
        else if (fields[1].IEquals("local"))
        {
            uniform.scope = ShaderScope::Local;
        }
        else
        {
            m_logger.Error("ParseUniform(). Invalid file layout. Uniform scope must be global, instance or local.");
            m_logger.Warn("Defaulting to global.");
            uniform.scope = ShaderScope::Global;
        }

        uniform.name = fields[2];

        data.uniforms.PushBack(uniform);

        return true;
    }

    void ResourceLoader<ShaderResource>::ParseTopology(ShaderConfig& data, const String& value)
    {
        if (value.IEquals("points"))
        {
            data.topology = ShaderTopology::Points;
        }
        else if (value.IEquals("lines"))
        {
            data.topology = ShaderTopology::Lines;
        }
        else
        {
            data.topology = ShaderTopology::Triangles;
        }
    }

    void ResourceLoader<ShaderResource>::ParseCullMode(ShaderConfig& data, const String& value)
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
