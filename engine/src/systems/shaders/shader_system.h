
#pragma once
#include "containers/hash_map.h"
#include "resources/shaders/shader.h"
#include "systems/system.h"

namespace C3D
{
    class RenderPass;

    struct ShaderSystemConfig
    {
        u16 maxShaderCount;
        u8 maxUniformCount;
        u8 maxGlobalTextures;
        u8 maxInstanceTextures;
    };

    class C3D_API ShaderSystem final : public SystemWithConfig<ShaderSystemConfig>
    {
    public:
        explicit ShaderSystem(const SystemManager* pSystemsManager);

        bool Init(const ShaderSystemConfig& config) override;
        void Shutdown() override;

        bool Create(RenderPass* pass, const ShaderConfig& config);

        u32 GetId(const String& name) const;

        Shader* Get(const String& name);
        Shader* GetById(u32 shaderId);

        bool Use(const char* name);
        bool UseById(u32 shaderId);

        u16 GetUniformIndex(Shader* shader, const char* name) const;

        bool SetUniform(const char* name, const void* value);
        bool SetUniformByIndex(u16 index, const void* value);

        bool SetSampler(const char* name, const Texture* t);
        bool SetSamplerByIndex(u16 index, const Texture* t);

        bool ApplyGlobal();
        bool ApplyInstance(bool needsUpdate);

        bool BindInstance(u32 instanceId);

    private:
        bool AddAttribute(Shader* shader, const ShaderAttributeConfig* config) const;
        bool AddSampler(Shader* shader, const ShaderUniformConfig* config);

        bool AddUniform(Shader* shader, const ShaderUniformConfig* config);
        bool AddUniform(Shader* shader, const String& name, u16 size, ShaderUniformType type, ShaderScope scope,
                        u16 setLocation, bool isSampler);

        void ShaderDestroy(Shader& shader) const;

        bool UniformAddStateIsValid(const Shader* shader) const;
        bool UniformNameIsValid(Shader* shader, const String& name) const;

        u32 m_currentShaderId;
        HashMap<String, Shader> m_shaders;
    };
}  // namespace C3D
