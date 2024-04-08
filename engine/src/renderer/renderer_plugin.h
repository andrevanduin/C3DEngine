
#pragma once
#include "core/defines.h"
#include "core/logger.h"
#include "render_buffer.h"
#include "renderer_types.h"

namespace C3D
{
    class Engine;
    class Shader;

    class RenderPass;
    struct RenderTarget;
    struct RenderTargetAttachment;
    struct Texture;
    struct ShaderUniform;
    struct ShaderConfig;
    struct FrameData;
    struct TextureMap;
    struct RenderPassConfig;

    class RendererPlugin
    {
    public:
        virtual bool Init(const RendererPluginConfig& config, u8* outWindowRenderTargetCount) = 0;
        virtual void Shutdown()                                                               = 0;

        virtual void OnResize(u32 width, u32 height) = 0;

        virtual bool PrepareFrame(const FrameData& frameData) = 0;
        virtual bool Begin(const FrameData& frameData)        = 0;
        virtual bool End(const FrameData& frameData)          = 0;

        virtual bool Present(const FrameData& frameData) = 0;

        virtual void SetViewport(const vec4& rect) = 0;
        virtual void ResetViewport()               = 0;
        /** @brief Sets the Renderer's scissor to the provided rectangle. */
        virtual void SetScissor(const ivec4& rect) = 0;
        /** @brief Resets the Renderer's scissor to the default. */
        virtual void ResetScissor() = 0;
        /** @brief Sets the Renderer's vertex winding direction. */
        virtual void SetWinding(RendererWinding winding) = 0;

        /**
         * @brief Sets Stencil testing to enabled or disabled.
         *
         * @param enabled Bool indicating if you want Stencil testing enabled or disabled
         */
        virtual void SetStencilTestingEnabled(bool enabled) = 0;

        /**
         * @brief Sets the Stencil Reference for testing.
         *
         * @param reference The reference to use when stencil testing/writing
         */
        virtual void SetStencilReference(u32 reference) = 0;

        /**
         * @brief Sets the Stencil Compare Mask.
         *
         * @param compareMask The value to use for the Stencil Compare Mask
         */
        virtual void SetStencilCompareMask(u32 compareMask) = 0;

        /**
         * @brief Sets the Stencil Write Mask.
         *
         * @param writeMask The value to use for the Stencil Write Mask
         */
        virtual void SetStencilWriteMask(u32 writeMask) = 0;

        /**
         * @brief Sets the Stencil operation.
         *
         * @param failOp The action that should be performed on samples that fail the stencil test
         * @param passOp The action that should be performed on samples that pass both depth and stencil tests
         * @param depthFailOp The action that should be performed on samples that pass the stencil test but fail the depth test
         * @param compareOp The comparison operaion used in the stencil test
         */
        virtual void SetStencilOperation(StencilOperation failOp, StencilOperation passOp, StencilOperation depthFailOp,
                                         CompareOperation compareOp) = 0;

        /**
         * @brief Sets Depth testing to enabled or disabled.
         *
         * @param enabled Bool indicating if you want Depth testing enabled or disabled
         */
        virtual void SetDepthTestingEnabled(bool enabled) = 0;

        virtual bool BeginRenderPass(RenderPass* pass, const C3D::FrameData& frameData) = 0;
        virtual bool EndRenderPass(RenderPass* pass)                                    = 0;

        virtual void CreateTexture(const u8* pixels, Texture* texture) = 0;
        virtual void CreateWritableTexture(Texture* texture)           = 0;

        virtual void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels)  = 0;
        virtual void ReadDataFromTexture(Texture* texture, u32 offset, u32 size, void** outMemory) = 0;
        virtual void ReadPixelFromTexture(Texture* texture, u32 x, u32 y, u8** outRgba)            = 0;

        virtual void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) = 0;
        virtual void DestroyTexture(Texture* texture)                             = 0;

        virtual bool CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const = 0;
        virtual void DestroyShader(Shader& shader)                                                    = 0;

        virtual bool InitializeShader(Shader& shader) = 0;
        virtual bool UseShader(const Shader& shader)  = 0;

        virtual bool BindShaderGlobals(Shader& shader)                  = 0;
        virtual bool BindShaderInstance(Shader& shader, u32 instanceId) = 0;

        virtual bool ShaderApplyGlobals(const Shader& shader, bool needsUpdate)  = 0;
        virtual bool ShaderApplyInstance(const Shader& shader, bool needsUpdate) = 0;

        virtual bool AcquireShaderInstanceResources(const Shader&, u32 textureMapCount, TextureMap** maps, u32* outInstanceId) = 0;
        virtual bool ReleaseShaderInstanceResources(const Shader&, u32 instanceId)                                             = 0;

        virtual bool AcquireTextureMapResources(TextureMap& map) = 0;
        virtual void ReleaseTextureMapResources(TextureMap& map) = 0;
        virtual bool RefreshTextureMapResources(TextureMap& map) = 0;

        virtual bool SetUniform(Shader& shader, const ShaderUniform& uniform, const void* value) = 0;

        virtual void CreateRenderTarget(RenderPass* pass, RenderTarget& target, u32 width, u32 height) = 0;
        virtual void DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory)                = 0;

        virtual RenderPass* CreateRenderPass(const RenderPassConfig& config) = 0;
        virtual bool DestroyRenderPass(RenderPass* pass)                     = 0;

        virtual RenderBuffer* CreateRenderBuffer(const String& name, RenderBufferType type, u64 totalSize,
                                                 RenderBufferTrackType trackType) = 0;
        virtual bool DestroyRenderBuffer(RenderBuffer* buffer)                    = 0;

        virtual Texture* GetWindowAttachment(u8 index) = 0;
        virtual Texture* GetDepthAttachment(u8 index)  = 0;

        virtual u8 GetWindowAttachmentIndex() = 0;
        virtual u8 GetWindowAttachmentCount() = 0;

        [[nodiscard]] virtual bool IsMultiThreaded() const = 0;

        virtual void SetFlagEnabled(RendererConfigFlagBits flag, bool enabled)      = 0;
        [[nodiscard]] virtual bool IsFlagEnabled(RendererConfigFlagBits flag) const = 0;

        RendererPluginType type = RendererPluginType::Unknown;
        u64 frameNumber         = INVALID_ID_U64;
        u8 drawIndex            = INVALID_ID_U8;

    protected:
        RendererPluginConfig m_config;
    };
}  // namespace C3D
