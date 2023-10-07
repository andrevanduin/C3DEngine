
#pragma once
#include "core/defines.h"
#include "render_buffer.h"
#include "renderer_plugin.h"
#include "renderer_types.h"
#include "systems/system.h"

namespace C3D
{
    class SystemManager;

    struct Texture;
    struct Geometry;
    struct Shader;
    struct ShaderUniform;
    struct FrameData;

    class Camera;

    struct RenderSystemConfig
    {
        const char* applicationName;

        RendererPlugin* rendererPlugin;
        RendererConfigFlags flags;

        SDL_Window* pWindow;
    };

    class C3D_API RenderSystem final : public SystemWithConfig<RenderSystemConfig>
    {
    public:
        explicit RenderSystem(const SystemManager* pSystemsManager);

        bool Init(const RenderSystemConfig& config) override;
        void Shutdown() override;

        void OnResize(u32 width, u32 height);

        bool DrawFrame(RenderPacket* packet, const FrameData& frameData);

        void SetViewport(const vec4& rect) const;
        void ResetViewport() const;

        void SetScissor(const vec4& rect) const;
        void ResetScissor() const;

        void SetLineWidth(float lineWidth) const;

        void CreateTexture(const u8* pixels, Texture* texture) const;
        void CreateWritableTexture(Texture* texture) const;

        void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) const;
        void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels) const;

        void ReadDataFromTexture(Texture* texture, u32 offset, u32 size, void** outMemory) const;
        void ReadPixelFromTexture(Texture* texture, u32 x, u32 y, u8** outRgba) const;

        void DestroyTexture(Texture* texture) const;

        bool CreateGeometry(Geometry* geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize,
                            u64 indexCount, const void* indices) const;
        void UpdateGeometry(Geometry* geometry, u32 offset, u32 vertexCount, const void* vertices) const;
        void DestroyGeometry(Geometry* geometry) const;

        void DrawGeometry(const GeometryRenderData& data) const;

        bool BeginRenderPass(RenderPass* pass, RenderTarget* target) const;
        bool EndRenderPass(RenderPass* pass) const;

        bool CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const;
        void DestroyShader(Shader& shader) const;

        bool InitializeShader(Shader* shader) const;

        bool UseShader(Shader* shader) const;

        bool ShaderBindGlobals(Shader* shader) const;
        bool ShaderBindInstance(Shader* shader, u32 instanceId) const;

        bool ShaderApplyGlobals(Shader* shader) const;
        bool ShaderApplyInstance(Shader* shader, bool needsUpdate) const;

        bool AcquireShaderInstanceResources(Shader* shader, u32 textureMapCount, TextureMap** maps,
                                            u32* outInstanceId) const;
        bool ReleaseShaderInstanceResources(Shader* shader, u32 instanceId) const;

        bool AcquireTextureMapResources(TextureMap& map) const;
        void ReleaseTextureMapResources(TextureMap& map) const;

        bool SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value) const;

        void CreateRenderTarget(u8 attachmentCount, RenderTargetAttachment* attachments, RenderPass* pass, u32 width,
                                u32 height, RenderTarget* outTarget) const;
        void DestroyRenderTarget(RenderTarget* target, bool freeInternalMemory) const;

        [[nodiscard]] RenderPass* CreateRenderPass(const RenderPassConfig& config) const;
        bool DestroyRenderPass(RenderPass* pass) const;

        [[nodiscard]] Texture* GetWindowAttachment(u8 index) const;
        [[nodiscard]] Texture* GetDepthAttachment(u8 index) const;

        [[nodiscard]] u8 GetWindowAttachmentIndex() const;
        [[nodiscard]] u8 GetWindowAttachmentCount() const;

        [[nodiscard]] RenderBuffer* CreateRenderBuffer(const String& name, RenderBufferType type, u64 totalSize,
                                                       bool useFreelist) const;
        bool DestroyRenderBuffer(RenderBuffer* buffer) const;

        [[nodiscard]] bool IsMultiThreaded() const;

        void SetFlagEnabled(RendererConfigFlagBits flag, bool enabled) const;
        [[nodiscard]] bool IsFlagEnabled(RendererConfigFlagBits flag) const;

    private:
        u8 m_windowRenderTargetCount = 0;
        u32 m_frameBufferWidth = 1280, m_frameBufferHeight = 720;

        bool m_resizing        = false;
        u8 m_framesSinceResize = 0;

        RendererPlugin* m_backendPlugin = nullptr;
    };
}  // namespace C3D
