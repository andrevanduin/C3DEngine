
#include "shader_system.h"

#include "core/engine.h"
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_utils.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "SHADER_SYSTEM";

    bool ShaderSystem::OnInit(const ShaderSystemConfig& config)
    {
        INFO_LOG("Initializing.");

        if (config.maxShaderCount == 0)
        {
            ERROR_LOG("config.maxShaderCount must be greater than 0.");
            return false;
        }

        m_config          = config;
        m_currentShaderId = INVALID_ID;

        m_shaders.Create(config.maxShaderCount);
        return true;
    }

    void ShaderSystem::OnShutdown()
    {
        INFO_LOG("Destroying all currently loaded shaders.");
        for (auto& shader : m_shaders)
        {
            ShaderDestroy(shader);
        }
        m_shaders.Destroy();
    }

    bool ShaderSystem::Create(void* pass, const ShaderConfig& config)
    {
        if (m_shaders.Has(config.name))
        {
            INFO_LOG("A shader with the name: '{}' already exists.", config.name);
            return true;
        }

        Shader shader;
        shader.state = ShaderState::NotCreated;
        shader.name  = config.name;

        // Setup our dynamic arrays
        shader.globalTextureMaps.Reserve(m_config.maxGlobalTextures + 1);
        shader.attributes.Reserve(4);

        // Setup HashMap for uniform lookups
        // NOTE: Way more than we will ever need but it prevents collisions in our hashtable
        shader.uniforms.Create(967);

        // Ensure that our push-constants are always 128 bytes (this is the minimum guaranteed size by Vulkan)
        shader.localUboStride = 128;

        // Copy over the flags specified in the config
        shader.flags = config.flags;

        // Mark Shader as created (but not yet initialized)
        shader.state = ShaderState::Uninitialized;

        // Add attributes
        for (const auto& attribute : config.attributes)
        {
            if (!AddAttribute(shader, attribute))
            {
                ERROR_LOG("Failed to add Attribute: {} to Shader: {}", attribute.name, config.name);
                return false;
            }
        }

        // Add Samplers and other uniforms
        for (const auto& uniform : config.uniforms)
        {
            if (UniformTypeIsASampler(uniform.type))
            {
                if (!AddSampler(shader, uniform))
                {
                    ERROR_LOG("Failed to add Sampler: {} to Shader: {}", uniform.name, config.name);
                    return false;
                }
            }
            else
            {
                if (!AddUniform(shader, uniform, INVALID_ID))
                {
                    ERROR_LOG("Failed to add Uniform: {} to Shader: {}", uniform.name, config.name);
                    return false;
                }
            }
        }

        // Create the shader in the renderer backend
        if (!Renderer.CreateShader(shader, config, pass))
        {
            ERROR_LOG("Failed to create shader: '{}'.", config.name);
            return false;
        }

        // Initialize the Shader
        if (!Renderer.InitializeShader(shader))
        {
            ERROR_LOG("Initialization failed for shader: '{}'.", config.name);
            return false;
        }

        // The id of the shader will be equal to the index in our HashMap
        shader.id = static_cast<u32>(m_shaders.GetIndex(config.name));
        // Store the shader in our hashtable
        m_shaders.Set(config.name, shader);

        INFO_LOG("Successfully created shader: '{}'.", config.name);
        return true;
    }

    u32 ShaderSystem::GetId(const String& name) const
    {
        if (!m_shaders.Has(name))
        {
            ERROR_LOG("There is no shader registered with name: '{}'.", name);
            return INVALID_ID;
        }
        return static_cast<u32>(m_shaders.GetIndex(name));
    }

    Shader* ShaderSystem::Get(const String& name)
    {
        if (const u32 shaderId = GetId(name); shaderId != INVALID_ID)
        {
            return GetById(shaderId);
        }
        return nullptr;
    }

    Shader* ShaderSystem::GetById(const u32 shaderId) { return &m_shaders.GetByIndex(shaderId); }

    bool ShaderSystem::Use(const char* name)
    {
        const u32 shaderId = GetId(name);
        if (shaderId == INVALID_ID) return false;
        return UseById(shaderId);
    }

    bool ShaderSystem::Use(const String& name)
    {
        const u32 shaderId = GetId(name);
        if (shaderId == INVALID_ID) return false;
        return UseById(shaderId);
    }

    bool ShaderSystem::UseById(const u32 shaderId)
    {
        // Only perform the use command if the shader id is different from the current
        // if (m_currentShaderId != shaderId)
        //{
        Shader& shader    = m_shaders.GetByIndex(shaderId);
        m_currentShaderId = shaderId;
        if (!Renderer.UseShader(shader))
        {
            ERROR_LOG("Failed to use shader: '{}'.", shader.name);
            return false;
        }
        if (!Renderer.BindShaderGlobals(shader))
        {
            ERROR_LOG("Failed to bind globals for shader: '{}'.", shader.name);
            return false;
        }
        //}
        return true;
    }

    u16 ShaderSystem::GetUniformIndex(Shader* shader, const char* name) const
    {
        if (!shader || shader->id == INVALID_ID)
        {
            ERROR_LOG("Called with invalid shader.");
            return INVALID_ID_U16;
        }

        if (!shader->uniforms.Has(name))
        {
            ERROR_LOG("Shader: '{}' does not a have a registered uniform named '{}'.", shader->name, name);
            return INVALID_ID_U16;
        }

        return static_cast<u16>(shader->uniforms.GetIndex(name));
    }

    bool ShaderSystem::SetUniform(const char* name, const void* value) { return SetArrayUniform(name, 0, value); }

    bool ShaderSystem::SetUniformByIndex(const u16 index, const void* value) { return SetArrayUniformByIndex(index, 0, value); }

    bool ShaderSystem::SetArrayUniform(const char* name, u32 arrayIndex, const void* value)
    {
        if (m_currentShaderId == INVALID_ID)
        {
            ERROR_LOG("No shader currently in use.");
            return false;
        }

        auto& shader = m_shaders.GetByIndex(m_currentShaderId);
        u16 index    = shader.GetUniformIndex(name);
        return SetArrayUniformByIndex(index, arrayIndex, value);
    }

    bool ShaderSystem::SetArrayUniformByIndex(u16 index, u32 arrayIndex, const void* value)
    {
        auto& shader  = m_shaders.GetByIndex(m_currentShaderId);
        auto& uniform = shader.uniforms.GetByIndex(index);
        if (shader.boundScope != uniform.scope)
        {
            if (uniform.scope == ShaderScope::Global)
            {
                Renderer.BindShaderGlobals(shader);
            }
            else if (uniform.scope == ShaderScope::Instance)
            {
                Renderer.BindShaderInstance(shader, shader.boundInstanceId);
            }
            else
            {
                Renderer.BindShaderLocal(shader);
            }
            shader.boundScope = uniform.scope;
        }
        return Renderer.SetUniform(shader, uniform, arrayIndex, value);
    }

    bool ShaderSystem::SetSampler(const char* name, const Texture* t) { return SetArraySampler(name, 0, t); }

    bool ShaderSystem::SetSamplerByIndex(const u16 index, const Texture* t) { return SetArraySamlerByIndex(index, 0, t); }

    bool ShaderSystem::SetArraySampler(const char* name, u32 arrayIndex, const Texture* t) { return SetArrayUniform(name, arrayIndex, t); }

    bool ShaderSystem::SetArraySamlerByIndex(u16 index, u32 arrayIndex, const Texture* t)
    {
        return SetArrayUniformByIndex(index, arrayIndex, t);
    }

    bool ShaderSystem::ApplyGlobal(const FrameData& frameData, const bool needsUpdate)
    {
        const auto& shader = m_shaders.GetByIndex(m_currentShaderId);
        return Renderer.ShaderApplyGlobals(frameData, shader, needsUpdate);
    }

    bool ShaderSystem::ApplyInstance(const FrameData& frameData, const bool needsUpdate)
    {
        const auto& shader = m_shaders.GetByIndex(m_currentShaderId);
        return Renderer.ShaderApplyInstance(frameData, shader, needsUpdate);
    }

    bool ShaderSystem::ApplyLocal(const FrameData& frameData)
    {
        const auto& shader = m_shaders.GetByIndex(m_currentShaderId);
        return Renderer.ShaderApplyLocal(frameData, shader);
    }

    bool ShaderSystem::BindInstance(const u32 instanceId)
    {
        Shader& shader         = m_shaders.GetByIndex(m_currentShaderId);
        shader.boundInstanceId = instanceId;
        return Renderer.BindShaderInstance(shader, instanceId);
    }

    bool ShaderSystem::BindLocal()
    {
        Shader& shader = m_shaders.GetByIndex(m_currentShaderId);
        return Renderer.BindShaderLocal(shader);
    }

    bool ShaderSystem::AddAttribute(Shader& shader, const ShaderAttributeConfig& config) const
    {
        u32 size;
        switch (config.type)
        {
            case Attribute_Int8:
            case Attribute_UInt8:
                size = 1;
                break;
            case Attribute_Int16:
            case Attribute_UInt16:
                size = 2;
                break;
            case Attribute_Float32:
            case Attribute_Int32:
            case Attribute_UInt32:
                size = 4;
                break;
            case Attribute_Float32_2:
                size = 8;
                break;
            case Attribute_Float32_3:
                size = 12;
                break;
            case Attribute_Float32_4:
                size = 16;
                break;
            default:
                ERROR_LOG("Unrecognized type, default to size of 4. This is probably not what you want!");
                size = 4;
                break;
        }

        shader.attributeStride += size;

        // Create and push the attribute
        ShaderAttribute attribute;
        attribute.name = config.name;
        attribute.size = size;
        attribute.type = config.type;
        shader.attributes.PushBack(attribute);

        return true;
    }

    bool ShaderSystem::AddSampler(Shader& shader, const ShaderUniformConfig& config)
    {
        // We cannot use PushConstants for samplers
        if (config.scope == ShaderScope::Local)
        {
            ERROR_LOG("Cannot add a sampler at local scope.");
            return false;
        }

        // Verify the name is valid and unique
        if (!UniformNameIsValid(shader, config.name) || !UniformAddStateIsValid(shader))
        {
            return false;
        }

        u16 location = 0;
        if (config.scope == ShaderScope::Global)
        {
            // If Global, push into the global list
            const u8 globalTextureCount = static_cast<u8>(shader.globalTextureMaps.Size());
            if (globalTextureCount + 1 > m_config.maxGlobalTextures)
            {
                ERROR_LOG("Global texture count: {} exceeds the max of: {}.", globalTextureCount, m_config.maxGlobalTextures);
                return false;
            }
            location = globalTextureCount;

            // NOTE: Creating a default texture map to be used here. Can always be updated later.
            TextureMap defaultMap;

            // Allocate a pointer, assign the texture and push into global texture maps.
            // NOTE: This allocation is only done for global texture maps.
            auto* map    = Memory.Allocate<TextureMap>(MemoryType::RenderSystem);
            *map         = defaultMap;
            map->texture = Textures.GetDefault();

            if (!Renderer.AcquireTextureMapResources(*map))
            {
                ERROR_LOG("Failed to acquire global texture map resources.");
                return false;
            }

            shader.globalTextureMaps.PushBack(map);
        }
        else
        {
            // Otherwise we are on instance level, so we keep the count of how many need to be added during resource
            // acquisition
            if (shader.instanceTextureCount + 1 > m_config.maxInstanceTextures)
            {
                ERROR_LOG("Instance texture count: {} exceeds the max of: {}.", shader.instanceTextureCount, m_config.maxInstanceTextures);
                return false;
            }

            location = shader.instanceTextureCount;
            shader.instanceTextureCount++;
        }

        // Then treat the sampler like any other uniform.
        if (!AddUniform(shader, config, location))
        {
            ERROR_LOG("Unable to add sampler uniform.");
            return false;
        }

        return true;
    }

    bool ShaderSystem::AddUniform(Shader& shader, const ShaderUniformConfig& config, u32 location)
    {
        if (!UniformAddStateIsValid(shader) || !UniformNameIsValid(shader, config.name))
        {
            return false;
        }

        const u16 uniformCount = static_cast<u16>(shader.uniforms.Count());
        if (uniformCount + 1 > m_config.maxUniformCount)
        {
            ERROR_LOG("A shader can only accept a combined maximum of: {} uniforms and samplers at global, instance and local scopes.",
                      m_config.maxUniformCount);
            return false;
        }

        bool isSampler = UniformTypeIsASampler(config.type);

        ShaderUniform entry{};
        entry.index       = uniformCount;
        entry.scope       = config.scope;
        entry.type        = config.type;
        entry.arrayLength = config.arrayLength;
        if (isSampler)
        {
            entry.location = location;
        }
        else
        {
            entry.location = entry.index;
        }

        if (config.scope == ShaderScope::Local)
        {
            entry.setIndex = 2;
            entry.offset   = shader.localUboSize;
            entry.size     = config.size;
        }
        else
        {
            entry.setIndex = static_cast<u8>(config.scope);
            entry.offset   = isSampler ? 0 : (config.scope == ShaderScope::Global ? shader.globalUboSize : shader.uboSize);
            entry.size     = isSampler ? 0 : config.size;
        }

        // Save the uniform in the HashMap with the name as it's key
        shader.uniforms.Set(config.name, entry);

        if (!isSampler)
        {
            u32 size = entry.size * entry.arrayLength;
            switch (entry.scope)
            {
                case ShaderScope::Global:
                    shader.globalUboSize += size;
                    break;
                case ShaderScope::Instance:
                    shader.uboSize += size;
                    break;
                case ShaderScope::Local:
                    shader.localUboSize += size;
                    break;
            }
        }

        return true;
    }

    void ShaderSystem::ShaderDestroy(Shader& shader) const
    {
        Renderer.DestroyShader(shader);

        // Set it to be unusable
        shader.state = ShaderState::NotCreated;

        // Free the global texture maps
        for (const auto textureMap : shader.globalTextureMaps)
        {
            Memory.Free(textureMap);
        }
        // Destroy the global texture maps
        shader.globalTextureMaps.Destroy();
        // Free the name
        shader.name.Destroy();
        // Set the id to invalid so we don't accidentally use this shader after this
        shader.id = INVALID_ID;

        // Free the uniforms and attributes
        shader.uniforms.Destroy();
        shader.attributes.Destroy();
    }

    bool ShaderSystem::UniformAddStateIsValid(const Shader& shader) const
    {
        if (shader.state != ShaderState::Uninitialized)
        {
            ERROR_LOG("Uniforms may only be added to shaders before initialization.");
            return false;
        }
        return true;
    }

    bool ShaderSystem::UniformNameIsValid(const Shader& shader, const String& name) const
    {
        if (!name)
        {
            ERROR_LOG("Uniform name does not exist or is empty.");
            return false;
        }
        if (shader.uniforms.Has(name))
        {
            ERROR_LOG("Shader: '{}' already contains a uniform named '{}'.", shader.name, name);
            return false;
        }
        return true;
    }
}  // namespace C3D
