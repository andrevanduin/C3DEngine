
#pragma once
#include "defines.h"
#include "logger/logger.h"
#include "render_buffer.h"
#include "renderer_types.h"
#include "rendergraph/rendergraph_types.h"

namespace C3D
{
    class Engine;
    class Shader;
    class Viewport;

    struct RenderTarget;
    struct RenderTargetAttachment;
    struct Texture;
    struct ShaderUniform;
    struct ShaderConfig;
    struct ShaderInstanceResourceConfig;
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

        virtual void BeginRenderpass(void* pass, const Viewport* viewport, const RenderTarget& target) = 0;
        virtual void EndRenderpass(void* pass)                                                         = 0;

        virtual void CreateTexture(Texture& texture, const u8* pixels) = 0;
        virtual void CreateWritableTexture(Texture& texture)           = 0;

        virtual void WriteDataToTexture(Texture& texture, u32 offset, u32 size, const u8* pixels, bool includeInFrameWorkload) = 0;
        virtual void ReadDataFromTexture(Texture& texture, u32 offset, u32 size, void** outMemory)                             = 0;
        virtual void ReadPixelFromTexture(Texture& texture, u32 x, u32 y, u8** outRgba)                                        = 0;

        virtual void ResizeTexture(Texture& texture, u32 newWidth, u32 newHeight) = 0;
        virtual void DestroyTexture(Texture& texture)                             = 0;

        virtual bool CreateShader(Shader& shader, const ShaderConfig& config, void* pass) const = 0;
        virtual bool ReloadShader(Shader& shader)                                               = 0;
        virtual void DestroyShader(Shader& shader)                                              = 0;

        virtual bool InitializeShader(Shader& shader) = 0;
        virtual bool UseShader(const Shader& shader)  = 0;

        virtual bool BindShaderGlobals(Shader& shader)                  = 0;
        virtual bool BindShaderInstance(Shader& shader, u32 instanceId) = 0;
        virtual bool BindShaderLocal(Shader& shader)                    = 0;

        virtual bool ShaderApplyGlobals(const FrameData& frameData, const Shader& shader, bool needsUpdate)  = 0;
        virtual bool ShaderApplyInstance(const FrameData& frameData, const Shader& shader, bool needsUpdate) = 0;
        virtual bool ShaderApplyLocal(const FrameData& frameData, const Shader& shader)                      = 0;

        /**
         * @brief Queries if the provided shader supports rendering in wireframe mode.
         *
         * @param shader The shader you want to query
         * @return True if the shader supports wireframe; false otherwise
         */
        virtual bool ShaderSupportsWireframe(const Shader& shader) = 0;

        virtual bool AcquireShaderInstanceResources(const Shader& shader, const ShaderInstanceResourceConfig& config,
                                                    u32& outInstanceId)            = 0;
        virtual bool ReleaseShaderInstanceResources(const Shader&, u32 instanceId) = 0;

        virtual bool AcquireTextureMapResources(TextureMap& map) = 0;
        virtual void ReleaseTextureMapResources(TextureMap& map) = 0;
        virtual bool RefreshTextureMapResources(TextureMap& map) = 0;

        virtual bool SetUniform(Shader& shader, const ShaderUniform& uniform, u32 arrayIndex, const void* value) = 0;

        virtual void CreateRenderTarget(void* pass, RenderTarget& target, u16 layerIndex, u32 width, u32 height) = 0;
        virtual void DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory)                          = 0;

        virtual bool CreateRenderpassInternals(const RenderpassConfig& config, void** internalData) = 0;
        virtual void DestroyRenderpassInternals(void* internalData)                                 = 0;

        virtual RenderBuffer* CreateRenderBuffer(const String& name, RenderBufferType type, u64 totalSize,
                                                 RenderBufferTrackType trackType) = 0;
        virtual bool DestroyRenderBuffer(RenderBuffer* buffer)                    = 0;

        virtual void WaitForIdle() = 0;

        /** @brief Begins a debug label with the provided text and color. */
        virtual void BeginDebugLabel(const String& text, const vec3& color) = 0;
        /** @brief Ends the previous debug label. */
        virtual void EndDebugLabel() = 0;

        virtual TextureHandle GetWindowAttachment(u8 index) = 0;
        virtual TextureHandle GetDepthAttachment(u8 index)  = 0;

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
