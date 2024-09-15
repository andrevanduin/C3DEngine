
#pragma once
#include "containers/hash_map.h"
#include "resources/shaders/shader.h"
#include "systems/events/event_system.h"
#include "systems/system.h"

namespace C3D
{
    class RenderPass;

    constexpr auto SHADER_SYSTEM_DEFAULT_MAX_SHADERS           = 128;
    constexpr auto SHADER_SYSTEM_DEFAULT_MAX_UNIFORMS          = 64;
    constexpr auto SHADER_SYSTEM_DEFAULT_MAX_ATTRIBUTES        = 64;
    constexpr auto SHADER_SYSTEM_DEFAULT_MAX_GLOBAL_TEXTURES   = 32;
    constexpr auto SHADER_SYSTEM_DEFAULT_MAX_INSTANCE_TEXTURES = 32;

    struct ShaderSystemConfig
    {
        u16 maxShaders         = SHADER_SYSTEM_DEFAULT_MAX_SHADERS;
        u8 maxUniforms         = SHADER_SYSTEM_DEFAULT_MAX_UNIFORMS;
        u8 maxAttributes       = SHADER_SYSTEM_DEFAULT_MAX_ATTRIBUTES;
        u8 maxGlobalTextures   = SHADER_SYSTEM_DEFAULT_MAX_GLOBAL_TEXTURES;
        u8 maxInstanceTextures = SHADER_SYSTEM_DEFAULT_MAX_INSTANCE_TEXTURES;
    };

    class C3D_API ShaderSystem final : public SystemWithConfig<ShaderSystemConfig>
    {
    public:
        bool OnInit(const CSONObject& config) override;
        void OnShutdown() override;

        bool Create(void* pass, const ShaderConfig& config);
        bool Reload(Shader& shader);

        u32 GetId(const String& name) const;

        Shader* Get(const String& name);
        Shader* GetById(u32 shaderId);

        bool Use(const char* name);
        bool Use(const String& name);

        /**
         * @brief Enables or disables wireframe mode for the provided shader.
         *
         * @param shader The shader you want to enable/disable wireframe mode for
         * @param enabled A boolean indicating if you want to enable or disable wireframe
         * @return True if successful; False if not supported or failed
         */
        bool SetWireframe(Shader& shader, bool enabled) const;

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

        bool OnFileWatchEvent(const u16 code, void* sender, const C3D::EventContext& context);

        u32 m_currentShaderId = 0;

        /** @brief An array of shaders managed by our Shader System. */
        DynamicArray<Shader> m_shaders;
        /** @brief A HashMap that maps names of Shaders to their index into our internal Shader array. */
        HashMap<String, u32> m_shaderNameToIndexMap;

        RegisteredEventCallback m_fileWatchCallback;
    };
}  // namespace C3D
