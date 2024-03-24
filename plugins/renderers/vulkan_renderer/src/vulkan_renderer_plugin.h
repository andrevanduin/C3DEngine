
#pragma once
#include <renderer/renderer_plugin.h>

#include "vulkan_buffer.h"
#include "vulkan_shader.h"
#include "vulkan_types.h"

namespace C3D
{
    struct FrameData;

    extern "C" {
    C3D_API RendererPlugin* CreatePlugin();
    C3D_API void DeletePlugin(RendererPlugin* plugin);
    }

    class VulkanRendererPlugin final : public RendererPlugin
    {
    public:
        VulkanRendererPlugin();

        bool Init(const RendererPluginConfig& config, u8* outWindowRenderTargetCount) override;
        void Shutdown() override;

        void OnResize(u32 width, u32 height) override;

        bool PrepareFrame(const FrameData& frameData) override;
        bool Begin(const FrameData& frameData) override;
        bool End(const FrameData& frameData) override;
        bool Present(const FrameData& frameData) override;

        void SetViewport(const vec4& rect) override;
        void ResetViewport() override;
        void SetScissor(const ivec4& rect) override;
        void ResetScissor() override;
        void SetWinding(RendererWinding winding) override;

        bool BeginRenderPass(RenderPass* pass, const C3D::FrameData& frameData) override;
        bool EndRenderPass(RenderPass* pass) override;

        void CreateTexture(const u8* pixels, Texture* texture) override;
        void CreateWritableTexture(Texture* texture) override;

        void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels) override;
        void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) override;

        void ReadDataFromTexture(Texture* texture, u32 offset, u32 size, void** outMemory) override;
        void ReadPixelFromTexture(Texture* texture, u32 x, u32 y, u8** outRgba) override;

        void DestroyTexture(Texture* texture) override;

        bool CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const override;
        void DestroyShader(Shader& shader) override;

        bool InitializeShader(Shader& shader) override;
        bool UseShader(const Shader& shader) override;

        bool BindShaderGlobals(Shader& shader) override;
        bool BindShaderInstance(Shader& shader, u32 instanceId) override;

        bool ShaderApplyGlobals(const Shader& shader, bool needsUpdate) override;
        bool ShaderApplyInstance(const Shader& shader, bool needsUpdate) override;

        bool AcquireShaderInstanceResources(const Shader& shader, u32 textureMapCount, TextureMap** maps, u32* outInstanceId) override;
        bool ReleaseShaderInstanceResources(const Shader& shader, u32 instanceId) override;

        bool AcquireTextureMapResources(TextureMap& map) override;
        void ReleaseTextureMapResources(TextureMap& map) override;
        bool RefreshTextureMapResources(TextureMap& map) override;

        bool SetUniform(Shader& shader, const ShaderUniform& uniform, const void* value) override;

        void CreateRenderTarget(RenderPass* pass, RenderTarget& target, u32 width, u32 height) override;
        void DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory) override;

        RenderPass* CreateRenderPass(const RenderPassConfig& config) override;
        bool DestroyRenderPass(RenderPass* pass) override;

        RenderBuffer* CreateRenderBuffer(const String& name, RenderBufferType bufferType, u64 totalSize,
                                         RenderBufferTrackType trackType) override;
        bool DestroyRenderBuffer(RenderBuffer* buffer) override;

        Texture* GetWindowAttachment(u8 index) override;
        Texture* GetDepthAttachment(u8 index) override;

        u8 GetWindowAttachmentIndex() override;
        u8 GetWindowAttachmentCount() override;

        [[nodiscard]] bool IsMultiThreaded() const override;

        void SetFlagEnabled(RendererConfigFlagBits flag, bool enabled) override;
        [[nodiscard]] bool IsFlagEnabled(RendererConfigFlagBits flag) const override;

    private:
        void CreateCommandBuffers();

        bool RecreateSwapChain();

        bool CreateShaderModule(const VulkanShaderStageConfig& config, VulkanShaderStage* shaderStage) const;

        VkSampler CreateSampler(TextureMap& map);

        VkSamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat) const;
        VkFilter ConvertFilterType(const char* op, TextureFilter filter) const;

        VulkanContext m_context;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
#endif

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D