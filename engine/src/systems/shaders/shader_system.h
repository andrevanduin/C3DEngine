
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

        bool OnInit(const ShaderSystemConfig& config) override;
        void OnShutdown() override;

        bool Create(void* pass, const ShaderConfig& config);

        u32 GetId(const String& name) const;

        Shader* Get(const String& name);
        Shader* GetById(u32 shaderId);

        bool Use(const char* name);
        bool Use(const String& name);

        bool UseById(u32 shaderId);

        u16 GetUniformIndex(Shader* shader, const char* name) const;

        bool SetUniform(const char* name, const void* value);
        bool SetUniformByIndex(u16 index, const void* value);

        bool SetArrayUniform(const char* name, u32 arrayIndex, const void* value);
        bool SetArrayUniformByIndex(u16 index, u32 arrayIndex, const void* value);

        bool SetSampler(const char* name, const Texture* t);
        bool SetSamplerByIndex(u16 index, const Texture* t);

        bool SetArraySampler(const char* name, u32 arrayIndex, const Texture* t);
        bool SetArraySamlerByIndex(u16 index, u32 arrayIndex, const Texture* t);

        bool ApplyGlobal(const FrameData& frameData, bool needsUpdate);
        bool ApplyInstance(const FrameData& frameData, bool needsUpdate);
        bool ApplyLocal(const FrameData& frameData);

        bool BindInstance(u32 instanceId);
        bool BindLocal();

    private:
        bool AddAttribute(Shader& shader, const ShaderAttributeConfig& config) const;
        bool AddSampler(Shader& shader, const ShaderUniformConfig& config);

        bool AddUniform(Shader& shader, const ShaderUniformConfig& config, u32 location);

        void ShaderDestroy(Shader& shader) const;

        bool UniformAddStateIsValid(const Shader& shader) const;
        bool UniformNameIsValid(const Shader& shader, const String& name) const;

        u32 m_currentShaderId = 0;
        HashMap<String, Shader> m_shaders;
    };
}  // namespace C3D
