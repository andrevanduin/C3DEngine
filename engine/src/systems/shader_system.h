
#pragma once
#include "system.h"
#include "containers/hash_map.h"
#include "resources/shader.h"

namespace C3D
{
	struct ShaderSystemConfig
	{
		u16 maxShaderCount;
		u8 maxUniformCount;
		u8 maxGlobalTextures;
		u8 maxInstanceTextures;
	};

	class C3D_API ShaderSystem : public System<ShaderSystemConfig>
	{
	public:
		ShaderSystem();

		bool Init(const ShaderSystemConfig& config) override;
		void Shutdown() override;

		bool Create(RenderPass* pass, const ShaderConfig& config);

		u32 GetId(const char* name);

		Shader* Get(const char* name);
		[[nodiscard]] Shader* GetById(u32 shaderId) const;

		bool Use(const char* name);
		bool UseById(u32 shaderId);

		u16 GetUniformIndex(Shader* shader, const char* name) const;

		bool SetUniform(const char* name, const void* value) const;
		bool SetUniformByIndex(u16 index, const void* value) const;;

		bool SetSampler(const char* name, const Texture* t) const;
		bool SetSamplerByIndex(u16 index, const Texture* t) const;

		[[nodiscard]] bool ApplyGlobal() const;
		[[nodiscard]] bool ApplyInstance(bool needsUpdate) const;

		[[nodiscard]] bool BindInstance(u32 instanceId) const;

	private:
		[[nodiscard]] u32 GetNewShaderId() const;

		bool AddAttribute(Shader* shader, const ShaderAttributeConfig* config) const;
		bool AddSampler(Shader* shader, const ShaderUniformConfig* config);

		bool AddUniform(Shader* shader, const ShaderUniformConfig* config);
		bool AddUniform(Shader* shader, const String& name, u16 size, ShaderUniformType type, ShaderScope scope, u16 setLocation, bool isSampler);

		static void ShaderDestroy(Shader* shader);

		bool UniformAddStateIsValid(const Shader* shader) const;
		bool UniformNameIsValid(Shader* shader, const String& name) const;

		u32 m_currentShaderId;
		HashMap<String, Shader> m_shaders;
	};
}
