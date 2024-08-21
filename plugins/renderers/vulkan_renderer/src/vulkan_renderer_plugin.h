
#pragma once
#include <renderer/renderer_plugin.h>

#include "vulkan_buffer.h"
#include "vulkan_shader.h"
#include "vulkan_types.h"

namespace C3D
{
    class Viewport;

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

        void SetStencilTestingEnabled(bool enabled) override;
        void SetStencilReference(u32 reference) override;
        void SetStencilCompareMask(u32 compareMask) override;
        void SetStencilWriteMask(u32 writeMask) override;
        void SetStencilOperation(StencilOperation failOp, StencilOperation passOp, StencilOperation depthFailOp,
                                 CompareOperation compareOp) override;
        void SetDepthTestingEnabled(bool enabled) override;

        void BeginRenderpass(void* pass, const Viewport* viewport, const RenderTarget& target) override;
        void EndRenderpass(void* pass) override;

        void CreateTexture(Texture& texture, const u8* pixels) override;
        void CreateWritableTexture(Texture& texture) override;

        void WriteDataToTexture(Texture& texture, u32 offset, u32 size, const u8* pixels, bool includeInFrameWorkload) override;
        void ResizeTexture(Texture& texture, u32 newWidth, u32 newHeight) override;

        void ReadDataFromTexture(Texture& texture, u32 offset, u32 size, void** outMemory) override;
        void ReadPixelFromTexture(Texture& texture, u32 x, u32 y, u8** outRgba) override;

        void DestroyTexture(Texture& texture) override;

        bool CreateShader(Shader& shader, const ShaderConfig& config, void* pass) const override;
        bool ReloadShader(Shader& shader) override;
        void DestroyShader(Shader& shader) override;

        bool InitializeShader(Shader& shader) override;
        bool UseShader(const Shader& shader) override;

        bool BindShaderGlobals(Shader& shader) override;
        bool BindShaderInstance(Shader& shader, u32 instanceId) override;
        bool BindShaderLocal(Shader& shader) override;

        bool ShaderApplyGlobals(const FrameData& frameData, const Shader& shader, bool needsUpdate) override;
        bool ShaderApplyInstance(const FrameData& frameData, const Shader& shader, bool needsUpdate) override;
        bool ShaderApplyLocal(const FrameData& frameData, const Shader& shader) override;
        bool ShaderSupportsWireframe(const Shader& shader) override;

        bool AcquireShaderInstanceResources(const Shader& shader, const ShaderInstanceResourceConfig& config, u32& outInstanceId) override;
        bool ReleaseShaderInstanceResources(const Shader& shader, u32 instanceId) override;

        bool AcquireTextureMapResources(TextureMap& map) override;
        void ReleaseTextureMapResources(TextureMap& map) override;
        bool RefreshTextureMapResources(TextureMap& map) override;

        bool SetUniform(Shader& shader, const ShaderUniform& uniform, u32 arrayIndex, const void* value) override;

        void CreateRenderTarget(void* pass, RenderTarget& target, u16 layerIndex, u32 width, u32 height) override;
        void DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory) override;

        bool CreateRenderpassInternals(const RenderpassConfig& config, void** internalData) override;
        void DestroyRenderpassInternals(void* internalData) override;

        RenderBuffer* CreateRenderBuffer(const String& name, RenderBufferType bufferType, u64 totalSize,
                                         RenderBufferTrackType trackType) override;
        bool DestroyRenderBuffer(RenderBuffer* buffer) override;

        void WaitForIdle() override;

        void BeginDebugLabel(const String& text, const vec3& color) override;
        void EndDebugLabel() override;

        TextureHandle GetWindowAttachment(u8 index) override;
        TextureHandle GetDepthAttachment(u8 index) override;

        u8 GetWindowAttachmentIndex() override;
        u8 GetWindowAttachmentCount() override;

        [[nodiscard]] bool IsMultiThreaded() const override;

        void SetFlagEnabled(RendererConfigFlagBits flag, bool enabled) override;
        [[nodiscard]] bool IsFlagEnabled(RendererConfigFlagBits flag) const override;

    private:
        void CreateCommandBuffers();

        bool RecreateSwapChain();

        bool CreateShaderModulesAndPipelines(Shader& shader);
        bool CreateShaderModule(const ShaderStageConfig& config, VulkanShaderStage& outStage) const;

        VkSampler CreateSampler(TextureMap& map);

        VkSamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat) const;
        VkFilter ConvertFilterType(const char* op, TextureFilter filter) const;

        VulkanContext m_context;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
#endif
    };
}  // namespace C3D