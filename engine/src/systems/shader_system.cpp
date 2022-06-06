
#include "shader_system.h"

#include "core/c3d_string.h"
#include "core/memory.h"

#include "services/services.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"

namespace C3D
{
	ShaderSystem::ShaderSystem()
		: System("SHADER_SYSTEM"), m_currentShaderId(0), m_shaders(nullptr)
	{}

	bool ShaderSystem::Init(const ShaderSystemConfig& config)
	{
		if (config.maxShaderCount == 0)
		{
			m_logger.Error("Init() - config.maxShaderCount must be greater than 0");
			return false;
		}

		m_config = config;
		m_shaders = Memory.Allocate<Shader>(m_config.maxShaderCount, MemoryType::Shader);
		m_currentShaderId = INVALID_ID;

		// Invalidate all shaders
		for (u32 i = 0; i < m_config.maxShaderCount; i++)
		{
			m_shaders[i].id = INVALID_ID;
		}
		return true;
	}

	void ShaderSystem::Shutdown()
	{
		for (u32 i = 0; i < m_config.maxShaderCount; i++)
		{
			if (m_shaders[i].id != INVALID_ID)
			{
				ShaderDestroy(&m_shaders[i]);
			}
		}
		m_nameToIdMap.clear();
		Memory.Free(m_shaders, m_config.maxShaderCount * sizeof(Shader), MemoryType::Shader);
	}

	bool ShaderSystem::Create(const ShaderConfig* config)
	{
		// Grab a new id for this shader
		const u32 id = GetNewShaderId();
		Shader* shader = &m_shaders[id];
		shader->id = id;
		if (shader->id == INVALID_ID)
		{
			m_logger.Error("Create() - Unable to find a free slot to create a new shader.");
			return false;
		}

		shader->state = ShaderState::NotCreated;
		shader->name = StringDuplicate(config->name);
		shader->useInstances = config->useInstances;
		shader->useLocals = config->useLocals;
		shader->pushConstantRangeCount = 0;
		shader->boundInstanceId = INVALID_ID;
		shader->attributeStride = 0;

		// Setup our dynamic arrays
		shader->attributes.Create(4);
		shader->uniforms.Create(8);

		// Setup HashTable for uniform lookups
		shader->uniformLookup.Create(1024); // NOTE: Waaaay more than we will ever need but it prevents collisions
		shader->uniformLookup.Fill(INVALID_ID_U16);

		// A running total of the size of the global uniform buffer object
		shader->globalUboSize = 0;
		// A running total of the size of the instance uniform buffer object
		shader->uboSize = 0;

		// Note: this is hard-coded because the vulkan spec only guarantees a minimum 128bytes stride
		// The drive might allocate more but this is not guaranteed on all video cards
		shader->pushConstantStride = 128; 
		shader->pushConstantSize = 0;

		u8 renderPassId = INVALID_ID_U8;
		if (!Renderer.GetRenderPassId(config->renderPassName, &renderPassId))
		{
			m_logger.Error("Create() - Unable to find RenderPass '{}' for shader: '{}'", config->renderPassName, config->name);
			return false;
		}

		if (!Renderer.CreateShader(shader, renderPassId, config->stageFileNames, config->stages))
		{
			m_logger.Error("Create() - Failed to create shader: '{}'", config->name);
			return false;
		}

		// Mark shader as created (but not yet initialized)
		shader->state = ShaderState::Uninitialized;

		// Add attributes
		for (const auto& attribute : config->attributes)
		{
			AddAttribute(shader, &attribute);
		}

		// Add Samplers and other uniforms
		for (const auto& uniform : config->uniforms)
		{
			if (uniform.type == Uniform_Sampler)
			{
				AddSampler(shader, &uniform);
			}
			else
			{
				AddUniform(shader, &uniform);
			}
		}
		 
		// Initialize the Shader
		if (!Renderer.InitializeShader(shader))
		{
			m_logger.Error("Create() - Initialization failed for shader: '{}'", config->name);
			return false;
		}

		// Store the shader id in our hashtable
		m_nameToIdMap[config->name] = shader->id;
		return true;
	}

	u32 ShaderSystem::GetId(const char* name)
	{
		const auto result = m_nameToIdMap.find(name);
		if (result == m_nameToIdMap.end())
		{
			m_logger.Error("GetId() - There is no shader registered with name: '{}'", name);
			return INVALID_ID;
		}
		return result->second;
	}

	Shader* ShaderSystem::Get(const char* name)
	{
		if (const u32 shaderId = GetId(name); shaderId != INVALID_ID)
		{
			return GetById(shaderId);
		}
		return nullptr;
	}

	Shader* ShaderSystem::GetById(const u32 shaderId) const
	{
		if (shaderId > m_config.maxShaderCount || m_shaders[shaderId].id == INVALID_ID)
		{
			return nullptr;
		}
		return &m_shaders[shaderId];
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

		const u32 uniformIndex = shader->uniformLookup.Get(name);
		if (uniformIndex == INVALID_ID)
		{
			m_logger.Error("GetUniformIndex() - Shader '{}' does not a have a registered uniform named '{}'", shader->name, name);
			return INVALID_ID_U16;
		}
		return shader->uniforms[uniformIndex].index;
	}

	bool ShaderSystem::SetUniform(const char* name, const void* value) const
	{
		if (m_currentShaderId == INVALID_ID)
		{
			m_logger.Error("SetUniform() - Called with no Shader in use.");
			return false;
		}
		Shader* shader = &m_shaders[m_currentShaderId];
		const u16 index = GetUniformIndex(shader, name);
		if (index == INVALID_ID_U16) 
		{
			m_logger.Error("SetUniform() - Called with invalid Uniform Name: '{}'", name);
			return false;
		}
		return SetUniformByIndex(index, value);
	}

	bool ShaderSystem::SetUniformByIndex(const u16 index, const void* value) const
	{
		Shader* shader = &m_shaders[m_currentShaderId];
		const ShaderUniform* uniform = &shader->uniforms[index];
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

	bool ShaderSystem::SetSampler(const char* name, const Texture* t) const
	{
		return SetUniform(name, t);
	}

	bool ShaderSystem::SetSamplerByIndex(const u16 index, const Texture* t) const
	{
		return SetUniformByIndex(index, t);
	}

	bool ShaderSystem::ApplyGlobal() const
	{
		return Renderer.ShaderApplyGlobals(&m_shaders[m_currentShaderId]);
	}

	bool ShaderSystem::ApplyInstance() const
	{
		return Renderer.ShaderApplyInstance(&m_shaders[m_currentShaderId]);
	}

	bool ShaderSystem::BindInstance(const u32 instanceId) const
	{
		Shader* shader = &m_shaders[m_currentShaderId];
		shader->boundInstanceId = instanceId;
		return Renderer.ShaderBindInstance(shader, instanceId);
	}

	u32 ShaderSystem::GetNewShaderId() const
	{
		for (u16 i = 0; i < m_config.maxShaderCount; i++)
		{
			if (m_shaders[i].id == INVALID_ID) return i;
		}
		return INVALID_ID;
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
		ShaderAttribute attribute{};
		attribute.name = StringDuplicate(config->name);
		attribute.size = size;
		attribute.type = config->type;
		shader->attributes.PushBack(attribute);

		return true;
	}

	bool ShaderSystem::AddSampler(Shader* shader, const ShaderUniformConfig* config)
	{
		if (config->scope == ShaderScope::Instance && !shader->useInstances)
		{
			m_logger.Error("AddSampler() - Cannot add an instance sampler for a shader that does not use instances.");
			return false;
		}

		// We cannot use PushConstants for samplers
		if (config->scope == ShaderScope::Local)
		{
			m_logger.Error("AddSampler() - Cannot add a sampler at local scope.");
			return false;
		}

		// Verify the name is valid and unique
		if (!UniformNameIsValid(shader, config->name) || !UniformAddStateIsValid(shader))
		{
			return false;
		}

		u16 location;
		if (config->scope == ShaderScope::Global)
		{
			// If Global, push into the global list
			const u8 globalTextureCount = static_cast<u8>(shader->globalTextures.size());
			if (globalTextureCount + 1 > m_config.maxGlobalTextures)
			{
				m_logger.Error("AddSampler() - Global texture count {} exceeds the max of {}.", globalTextureCount, m_config.maxGlobalTextures);
				return false;
			}
			location = globalTextureCount;
			shader->globalTextures.push_back(Textures.GetDefault());
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

		if (!AddUniform(shader, config->name, 0, config->type, config->scope, location, true))
		{
			m_logger.Error("AddSampler() - Unable to add sampler uniform.");
			return false;
		}

		return true;
	}

	bool ShaderSystem::AddUniform(Shader* shader, const ShaderUniformConfig* config)
	{
		if (!UniformAddStateIsValid(shader) || !UniformNameIsValid(shader, config->name))
		{
			return false;
		}
		return AddUniform(shader, config->name, config->size, config->type, config->scope, 0, false);
	}

	bool ShaderSystem::AddUniform(Shader* shader, const char* name, const u16 size, const ShaderUniformType type, ShaderScope scope, const u16 setLocation, const bool isSampler)
	{
		const u16 uniformCount = static_cast<u16>(shader->uniforms.Size());
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
			if (!shader->useLocals)
			{
				m_logger.Error("AddUniform() - Cannot add a locally-scoped uniform for a shader that does not use locals.");
				return false;
			}

			entry.setIndex = INVALID_ID_U8;
			const Range r = GetAlignedRange(shader->pushConstantSize, size, 4);
			entry.offset = r.offset;
			entry.size = static_cast<u16>(r.size);

			// Keep track in the shader for use in initialization
			shader->pushConstantRanges[shader->pushConstantRangeCount] = r;
			shader->pushConstantRangeCount++;

			shader->pushConstantSize += r.size;
		}

		// Save the uniform name in our lookup table
		shader->uniformLookup.Set(name, &entry.index);
		// Add the uniform to our shader
		shader->uniforms.PushBack(entry);

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

	void ShaderSystem::ShaderDestroy(Shader* shader)
	{
		Renderer.DestroyShader(shader);

		// Set it to be unusable
		shader->state = ShaderState::NotCreated;

		// Free the name
		if (shader->name)
		{
			const u64 length = StringLength(shader->name);
			Memory.Free(shader->name, length + 1, MemoryType::String);
		}
		shader->name = nullptr;
		shader->id = INVALID_ID;

		// Free dynamic arrays for uniforms and attributes
		shader->uniforms.Destroy();
		shader->attributes.Destroy();

		// Free the uniform lookup table
		shader->uniformLookup.Destroy();
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

	bool ShaderSystem::UniformNameIsValid(Shader* shader, const char* name) const
	{
		if (!name || !StringLength(name))
		{
			m_logger.Error("Uniform name does not exist or is empty");
			return false;
		}

		if (const auto uniformIndex = shader->uniformLookup.Get(name); uniformIndex != INVALID_ID_U16)
		{
			m_logger.Error("Shader '{}' already contains a uniform named '{}'", shader->name, name);
			return false;
		}
		return true;
	}
}
