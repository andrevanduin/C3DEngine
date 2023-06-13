
#include "shader_system.h"

#include "core/engine.h"
#include "systems/system_manager.h"
#include "renderer/renderer_frontend.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
	ShaderSystem::ShaderSystem(const Engine* engine)
		: SystemWithConfig(engine, "SHADER_SYSTEM"), m_currentShaderId(0)
	{}

	bool ShaderSystem::Init(const ShaderSystemConfig& config)
	{
		if (config.maxShaderCount == 0)
		{
			m_logger.Error("Init() - config.maxShaderCount must be greater than 0");
			return false;
		}

		m_config = config;
		m_currentShaderId = INVALID_ID;

		m_shaders.Create(config.maxShaderCount);
		return true;
	}

	void ShaderSystem::Shutdown()
	{
		for (auto& shader : m_shaders)
		{
			ShaderDestroy(shader);
		}
		m_shaders.Destroy();
	}

	bool ShaderSystem::Create(RenderPass* pass, const ShaderConfig& config)
	{
		if (m_shaders.Has(config.name))
		{
			m_logger.Error("Create() - A shader with the name '{}' already exists", config.name);
			return false;
		}

		Shader shader;
		shader.state = ShaderState::NotCreated;
		shader.name = config.name;

		// Setup our dynamic arrays
		shader.globalTextureMaps.Reserve(m_config.maxGlobalTextures + 1);
		shader.attributes.Reserve(4);

		// Setup HashMap for uniform lookups
		shader.uniforms.Create(1024); // NOTE: Way more than we will ever need but it prevents collisions in our hashtable

		// Process flags
		if (config.depthTest)
		{
			shader.flags |= ShaderFlagDepthTest;
		}
		if (config.depthWrite)
		{
			shader.flags |= ShaderFlagDepthWrite;
		}

		if (!Renderer.CreateShader(&shader, config, pass))
		{
			m_logger.Error("Create() - Failed to create shader: '{}'", config.name);
			return false;
		}

		// Mark shader as created (but not yet initialized)
		shader.state = ShaderState::Uninitialized;

		// Add attributes
		for (const auto& attribute : config.attributes)
		{
			AddAttribute(&shader, &attribute);
		}

		// Add Samplers and other uniforms
		for (const auto& uniform : config.uniforms)
		{
			if (uniform.type == Uniform_Sampler)
			{
				AddSampler(&shader, &uniform);
			}
			else
			{
				AddUniform(&shader, &uniform);
			}
		}
		 
		// Initialize the Shader
		if (!Renderer.InitializeShader(&shader))
		{
			m_logger.Error("Create() - Initialization failed for shader: '{}'", config.name);
			return false;
		}

		// The id of the shader will be equal to the index in our HashMap
		shader.id = static_cast<u32>(m_shaders.GetIndex(config.name));
		// Store the shader in our hashtable
		m_shaders.Set(config.name, shader);
		return true;
	}

	u32 ShaderSystem::GetId(const char* name) const
	{
		if (!m_shaders.Has(name))
		{
			m_logger.Error("GetId() - There is no shader registered with name: '{}'", name);
			return INVALID_ID;
		}
		return static_cast<u32>(m_shaders.GetIndex(name));
	}

	Shader* ShaderSystem::Get(const char* name)
	{
		if (const u32 shaderId = GetId(name); shaderId != INVALID_ID)
		{
			return GetById(shaderId);
		}
		return nullptr;
	}

	Shader* ShaderSystem::GetById(const u32 shaderId)
	{
		return &m_shaders.GetByIndex(shaderId);
	}

	bool ShaderSystem::Use(const char* name)
	{
		const u32 shaderId = GetId(name);
		if (shaderId == INVALID_ID) return false;
		return UseById(shaderId);
	}

	bool ShaderSystem::UseById(const u32 shaderId)
	{
		// Only perform the use command if the shader id is different from the current
		if (m_currentShaderId != shaderId)
		{
			Shader* shader = GetById(shaderId);
			m_currentShaderId = shaderId;
			if (!Renderer.UseShader(shader))
			{
				m_logger.Error("UseById() - Failed to use shader '{}'", shader->name);
				return false;
			}
			if (!Renderer.ShaderBindGlobals(shader))
			{
				m_logger.Error("UseById() - Failed to bind globals for shader '{}'", shader->name);
				return false;
			}
		}
		return true;
	}

	u16 ShaderSystem::GetUniformIndex(Shader* shader, const char* name) const
	{
		if (!shader || shader->id == INVALID_ID)
		{
			m_logger.Error("GetUniformIndex() - Called with invalid shader");
			return INVALID_ID_U16;
		}

		if (!shader->uniforms.Has(name))
		{
			m_logger.Error("GetUniformIndex() - Shader '{}' does not a have a registered uniform named '{}'", shader->name, name);
			return INVALID_ID_U16;
		}

		return static_cast<u16>(shader->uniforms.GetIndex(name));
	}

	bool ShaderSystem::SetUniform(const char* name, const void* value)
	{
		if (m_currentShaderId == INVALID_ID)
		{
			m_logger.Error("SetUniform() - Called with no Shader in use.");
			return false;
		}
		Shader* shader = &m_shaders.GetByIndex(m_currentShaderId);
		const u16 index = GetUniformIndex(shader, name);
		if (index == INVALID_ID_U16) 
		{
			m_logger.Error("SetUniform() - Called with invalid Uniform Name: '{}'", name);
			return false;
		}
		return SetUniformByIndex(index, value);
	}

	bool ShaderSystem::SetUniformByIndex(const u16 index, const void* value)
	{
		Shader* shader = &m_shaders.GetByIndex(m_currentShaderId);
		const ShaderUniform* uniform = &shader->uniforms.GetByIndex(index);
		if (shader->boundScope != uniform->scope)
		{
			if (uniform->scope == ShaderScope::Global)
			{
				Renderer.ShaderBindGlobals(shader);
			}
			else if (uniform->scope == ShaderScope::Instance)
			{
				Renderer.ShaderBindInstance(shader, shader->boundInstanceId);
			}
			shader->boundScope = uniform->scope;
		}
		return Renderer.SetUniform(shader, uniform, value);
	}

	bool ShaderSystem::SetSampler(const char* name, const Texture* t)
	{
		return SetUniform(name, t);
	}

	bool ShaderSystem::SetSamplerByIndex(const u16 index, const Texture* t)
	{
		return SetUniformByIndex(index, t);
	}

	bool ShaderSystem::ApplyGlobal()
	{
		return Renderer.ShaderApplyGlobals(&m_shaders.GetByIndex(m_currentShaderId));
	}

	bool ShaderSystem::ApplyInstance(const bool needsUpdate)
	{
		return Renderer.ShaderApplyInstance(&m_shaders.GetByIndex(m_currentShaderId), needsUpdate);
	}

	bool ShaderSystem::BindInstance(const u32 instanceId)
	{
		Shader* shader = &m_shaders.GetByIndex(m_currentShaderId);
		shader->boundInstanceId = instanceId;
		return Renderer.ShaderBindInstance(shader, instanceId);
	}

	bool ShaderSystem::AddAttribute(Shader* shader, const ShaderAttributeConfig* config) const
	{
		u32 size;
		switch (config->type)
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
				m_logger.Error("AddAttribute() - Unrecognized type {}, default to size of 4. This is probably not what you want!");
				size = 4;
				break;
		}

		shader->attributeStride += size;

		// Create and push the attribute
		ShaderAttribute attribute;
		attribute.name = config->name;
		attribute.size = size;
		attribute.type = config->type;
		shader->attributes.PushBack(attribute);

		return true;
	}

	bool ShaderSystem::AddSampler(Shader* shader, const ShaderUniformConfig* config)
	{
		// We cannot use PushConstants for samplers
		if (config->scope == ShaderScope::Local)
		{
			m_logger.Error("AddSampler() - Cannot add a sampler at local scope.");
			return false;
		}

		// Verify the name is valid and unique
		if (!UniformNameIsValid(shader, config->name.Data()) || !UniformAddStateIsValid(shader))
		{
			return false;
		}

		u16 location;
		if (config->scope == ShaderScope::Global)
		{
			// If Global, push into the global list
			const u8 globalTextureCount = static_cast<u8>(shader->globalTextureMaps.Size());
			if (globalTextureCount + 1 > m_config.maxGlobalTextures)
			{
				m_logger.Error("AddSampler() - Global texture count {} exceeds the max of {}.", globalTextureCount, m_config.maxGlobalTextures);
				return false;
			}
			location = globalTextureCount;

			// NOTE: Creating a default texture map to be used here. Can always be updated later.
			TextureMap defaultMap(TextureUse::Unknown, TextureFilter::ModeLinear, TextureFilter::ModeLinear, TextureRepeat::Repeat);

			if (!Renderer.AcquireTextureMapResources(&defaultMap))
			{
				m_logger.Error("AddSampler() - Failed to acquire global texture map resources.");
				return false;
			}

			// Allocate a pointer assign the texture and push into global texture maps.
			// NOTE: This allocation is only done for global texture maps.
			auto* map = Memory.Allocate<TextureMap>(MemoryType::RenderSystem);
			*map = defaultMap;
			map->texture = Textures.GetDefault();

			shader->globalTextureMaps.PushBack(map);
		}
		else
		{
			// Otherwise we are on instance level, so we keep the count of how many need to be added during resource acquisition
			if (shader->instanceTextureCount + 1 > m_config.maxInstanceTextures)
			{
				m_logger.Error("AddSampler() - Instance texture count {} exceeds the max of {}.", shader->instanceTextureCount, m_config.maxInstanceTextures);
				return false;
			}

			location = static_cast<u16>(shader->instanceTextureCount);
			shader->instanceTextureCount++;
		}

		if (!AddUniform(shader, config->name.Data(), 0, config->type, config->scope, location, true))
		{
			m_logger.Error("AddSampler() - Unable to add sampler uniform.");
			return false;
		}

		return true;
	}

	bool ShaderSystem::AddUniform(Shader* shader, const ShaderUniformConfig* config)
	{
		if (!UniformAddStateIsValid(shader) || !UniformNameIsValid(shader, config->name.Data()))
		{
			return false;
		}
		return AddUniform(shader, config->name, config->size, config->type, config->scope, 0, false);
	}

	bool ShaderSystem::AddUniform(Shader* shader, const String& name, const u16 size, const ShaderUniformType type, ShaderScope scope, const u16 setLocation, const bool isSampler)
	{
		const u16 uniformCount = static_cast<u16>(shader->uniforms.Count());
		if (uniformCount + 1 > m_config.maxUniformCount)
		{
			m_logger.Error("AddUniform() - A shader can only accept a combined maximum of {} uniforms and sampler at global, instance and local scopes.", m_config.maxUniformCount);
			return false;
		}

		ShaderUniform entry{};
		entry.index = uniformCount;
		entry.scope = scope;
		entry.type = type;
		// If we are adding a sampler just use the provided location otherwise the location is equal to the index
		entry.location = isSampler ? setLocation : entry.index;

		const bool isGlobal = scope == ShaderScope::Global;
		if (scope != ShaderScope::Local)
		{
			entry.setIndex = static_cast<u8>(scope);
			entry.offset = isSampler ? 0 : isGlobal ? shader->globalUboSize : shader->uboSize;
			entry.size = isSampler ? 0 : size;
		}
		else
		{
			entry.setIndex = INVALID_ID_U8;
			const Range r = GetAlignedRange(shader->pushConstantSize, size, 4);
			entry.offset = r.offset;
			entry.size = static_cast<u16>(r.size);

			// Keep track in the shader for use in initialization
			shader->pushConstantRanges[shader->pushConstantRangeCount] = r;
			shader->pushConstantRangeCount++;

			shader->pushConstantSize += r.size;
		}

		// Save the uniform in the HashMap with the name as it's key
		shader->uniforms.Set(name, entry);

		if (!isSampler)
		{
			if (entry.scope == ShaderScope::Global)
			{
				shader->globalUboSize += entry.size;
			}
			else if (entry.scope == ShaderScope::Instance)
			{
				shader->uboSize += entry.size;
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
			Memory.Free(MemoryType::RenderSystem, textureMap);
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

	bool ShaderSystem::UniformAddStateIsValid(const Shader* shader) const
	{
		if (shader->state != ShaderState::Uninitialized)
		{
			m_logger.Error("Uniforms may only be added to shaders before initialization");
			return false;
		}
		return true;
	}

	bool ShaderSystem::UniformNameIsValid(Shader* shader, const String& name) const
	{
		if (!name)
		{
			m_logger.Error("Uniform name does not exist or is empty");
			return false;
		}
		if (shader->uniforms.Has(name))
		{
			m_logger.Error("Shader '{}' already contains a uniform named '{}'", shader->name, name);
			return false;
		}
		return true;
	}
}
