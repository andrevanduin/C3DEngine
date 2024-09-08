
#include "shader_system.h"

#include "parsers/cson_types.h"
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_utils.h"
#include "resources/textures/texture_map.h"
#include "systems/events/event_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    bool ShaderSystem::OnInit(const CSONObject& config)
    {
        INFO_LOG("Initializing.");

        // Parse the user provided config
        for (const auto& prop : config.properties)
        {
            if (prop.name.IEquals("maxShaders"))
            {
                m_config.maxShaders = prop.GetI64();
            }
            else if (prop.name.IEquals("maxUniforms"))
            {
                m_config.maxUniforms = prop.GetI64();
            }
            else if (prop.name.IEquals("maxAttributes"))
            {
                m_config.maxAttributes = prop.GetI64();
            }
            else if (prop.name.IEquals("maxGlobalTextures"))
            {
                m_config.maxGlobalTextures = prop.GetI64();
            }
            else if (prop.name.IEquals("maxInstanceTextures"))
            {
                m_config.maxInstanceTextures = prop.GetI64();
            }
        }

        if (m_config.maxShaders == 0)
        {
            ERROR_LOG("config.maxShaderCount must be greater than 0.");
            return false;
        }

        m_currentShaderId = INVALID_ID;

        // Initially we reserve room for the user provided amount of shaders
        m_shaders.Reserve(m_config.maxShaders);
        // We also create our name to index map so we can find shaders by name
        m_shaderNameToIndexMap.Create();

#ifdef _DEBUG
        m_fileWatchCallback = Event.Register(
            EventCodeWatchedFileChanged,
            [this](const u16 code, void* sender, const C3D::EventContext& context) { return OnFileWatchEvent(code, sender, context); });
#endif

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

#ifdef _DEBUG
        Event.Unregister(m_fileWatchCallback);
#endif
    }

    bool ShaderSystem::Create(void* pass, const ShaderConfig& config)
    {
        if (m_shaderNameToIndexMap.Has(config.name))
        {
            INFO_LOG("A shader with the name: '{}' already exists.", config.name);
            return true;
        }

        Shader shader;
        shader.state = ShaderState::NotCreated;
        shader.name  = config.name;

        // Setup our dynamic arrays. Reserve enough space as specified in the config
        shader.globalTextureMaps.Reserve(m_config.maxGlobalTextures);
        shader.uniforms.Reserve(m_config.maxUniforms);
        shader.attributes.Reserve(m_config.maxAttributes);

        // Setup HashMap for mapping Uniform names to the index into our Uniform array
        shader.uniformNameToIndexMap.Create();

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

        // Store our shader in our internal shaders array by finding the first empty slot
        u32 shaderId = INVALID_ID;
        for (u32 i = 0; i < m_shaders.Size(); ++i)
        {
            if (m_shaders[i].id == INVALID_ID)
            {
                // We have found an empty slot
                shader.id    = i;
                shaderId     = i;
                m_shaders[i] = shader;
                break;
            }
        }

        if (shaderId == INVALID_ID)
        {
            // There was no more room in our Shaders array so we must append our Shader to the end
            shaderId  = m_shaders.Size();
            shader.id = shaderId;
            m_shaders.PushBack(shader);
        }

        // Store the name and it's index (id) in our Name to Index map
        m_shaderNameToIndexMap.Set(config.name, shaderId);

        INFO_LOG("Successfully created shader: '{}'.", config.name);
        return true;
    }

    bool ShaderSystem::Reload(Shader& shader) { return Renderer.ReloadShader(shader); }

    u32 ShaderSystem::GetId(const String& name) const
    {
        if (!m_shaderNameToIndexMap.Has(name))
        {
            ERROR_LOG("There is no shader registered with name: '{}'.", name);
            return INVALID_ID;
        }
        return static_cast<u32>(m_shaderNameToIndexMap.Get(name));
    }

    Shader* ShaderSystem::Get(const String& name)
    {
        u32 id = GetId(name);
        if (id != INVALID_ID)
        {
            return GetById(id);
        }
        return nullptr;
    }

    Shader* ShaderSystem::GetById(const u32 shaderId)
    {
        if (shaderId >= m_shaders.Size()) return nullptr;
        return &m_shaders[shaderId];
    }

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

    bool ShaderSystem::SetWireframe(Shader& shader, bool enabled) const
    {
        if (!enabled)
        {
            // Disabling is always supported since we don't actually need to do anything in the backend
            shader.wireframeEnabled = false;
            return true;
        }
        return Renderer.ShaderSetWireframe(shader, enabled);
    }

    bool ShaderSystem::UseById(const u32 shaderId)
    {
        // Only perform the use command if the shader id is different from the current
        // if (m_currentShaderId != shaderId)
        //{
        Shader& shader    = m_shaders[shaderId];
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

        if (!shader->uniformNameToIndexMap.Has(name))
        {
            ERROR_LOG("Shader: '{}' does not a have a registered uniform named '{}'.", shader->name, name);
            return INVALID_ID_U16;
        }

        return static_cast<u16>(shader->uniformNameToIndexMap.Get(name));
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

        auto& shader = m_shaders[m_currentShaderId];
        u16 index    = shader.GetUniformIndex(name);
        return SetArrayUniformByIndex(index, arrayIndex, value);
    }

    bool ShaderSystem::SetArrayUniformByIndex(u16 index, u32 arrayIndex, const void* value)
    {
        auto& shader  = m_shaders[m_currentShaderId];
        auto& uniform = shader.uniforms[index];
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
#ifdef _DEBUG
        if (m_currentShaderId == INVALID_ID) return false;
#endif

        const auto& shader = m_shaders[m_currentShaderId];
        return Renderer.ShaderApplyGlobals(frameData, shader, needsUpdate);
    }

    bool ShaderSystem::ApplyInstance(const FrameData& frameData, const bool needsUpdate)
    {
#ifdef _DEBUG
        if (m_currentShaderId == INVALID_ID) return false;
#endif

        const auto& shader = m_shaders[m_currentShaderId];
        return Renderer.ShaderApplyInstance(frameData, shader, needsUpdate);
    }

    bool ShaderSystem::ApplyLocal(const FrameData& frameData)
    {
#ifdef _DEBUG
        if (m_currentShaderId == INVALID_ID) return false;
#endif

        const auto& shader = m_shaders[m_currentShaderId];
        return Renderer.ShaderApplyLocal(frameData, shader);
    }

    bool ShaderSystem::BindInstance(const u32 instanceId)
    {
#ifdef _DEBUG
        if (m_currentShaderId == INVALID_ID) return false;
#endif

        Shader& shader         = m_shaders[m_currentShaderId];
        shader.boundInstanceId = instanceId;
        return Renderer.BindShaderInstance(shader, instanceId);
    }

    bool ShaderSystem::BindLocal()
    {
#ifdef _DEBUG
        if (m_currentShaderId == INVALID_ID) return false;
#endif

        Shader& shader = m_shaders[m_currentShaderId];
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

        const u16 uniformCount = static_cast<u16>(shader.uniforms.Size());
        if (uniformCount >= m_config.maxUniforms)
        {
            ERROR_LOG("A shader can only accept a combined maximum of: {} uniforms and samplers at global, instance and local scopes.",
                      m_config.maxUniforms);
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

        // Find a empty slot in the uniforms array
        u16 index = INVALID_ID_U16;
        for (u16 i = 0; i < shader.uniforms.Size(); i++)
        {
            if (shader.uniforms[i].index == INVALID_ID_U16)
            {
                // We have found an empty slot keep track of the index
                index = i;
                // And copy our entry into our uniforms array
                shader.uniforms[i] = entry;
                break;
            }
        }

        if (index == INVALID_ID_U16)
        {
            // We have reached the end of our array without finding a spot so let's append
            index = shader.uniforms.Size();
            shader.uniforms.PushBack(entry);
        }

        // Save the mapping from name to index in our HashMap
        shader.uniformNameToIndexMap.Set(config.name, index);

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
        if (name.Empty())
        {
            ERROR_LOG("Uniform name does is empty.");
            return false;
        }
        if (shader.uniformNameToIndexMap.Has(name))
        {
            ERROR_LOG("Shader: '{}' already contains a uniform named '{}'.", shader.name, name);
            return false;
        }
        return true;
    }

    bool ShaderSystem::OnFileWatchEvent(const u16 code, void* sender, const C3D::EventContext& context)
    {
        FileWatchId watchId = context.data.u32[0];
        for (auto& shader : m_shaders)
        {
            for (auto shaderWatchId : shader.moduleWatchIds)
            {
                if (watchId == shaderWatchId)
                {
                    if (!Reload(shader))
                    {
                        ERROR_LOG("Failed to reload shader: '{}'", shader.name);
                    }

                    // We have handled the event but other systems might want to also handle so we return false
                    return false;
                }
            }
        }

        // No shader was updated, so return false in order to let other systems also pickup this event
        return false;
    }
}  // namespace C3D
