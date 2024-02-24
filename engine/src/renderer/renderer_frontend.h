
#pragma once
#include "core/defines.h"
#include "render_buffer.h"
#include "renderer_plugin.h"
#include "renderer_types.h"
#include "systems/system.h"

namespace C3D
{
    class SystemManager;
    class Shader;

    struct Texture;
    struct Geometry;
    struct ShaderUniform;
    struct FrameData;

    class Camera;
    class Viewport;

    struct RenderSystemConfig
    {
        const char* applicationName;

        RendererPlugin* rendererPlugin;
        RendererConfigFlags flags;
    };

    class C3D_API RenderSystem final : public SystemWithConfig<RenderSystemConfig>
    {
    public:
        explicit RenderSystem(const SystemManager* pSystemsManager);

        bool OnInit(const RenderSystemConfig& config) override;
        void OnShutdown() override;

        void OnResize(u32 width, u32 height);

        /**
         * @brief Performs all the setup routines which are required at the start of a frame.
         * @note If this method returns false it does not necessarily indicate a failure
         * it can also mean that the backend is simply currently not in a state capable of drawing a frame.
         * If this method returns false you should not call EndFrame!
         *
         * @param frameData The frame data associated with this frame.
         * @return True if successful, false otherwise.
         */
        bool PrepareFrame(FrameData& frameData);

        /**
         * @brief Begins the actual render. There must be atleast one of these calls per frame.
         * @note There should be a matching end call for every begin call.
         *
         * @param frameData The frame data associated with this frame.
         * @return True if successful, false otherwise.
         */
        bool Begin(const FrameData& frameData) const;

        /**
         * @brief Ends the actual render. There must be atleast one of these calls per frame.
         *
         * @param frameData The frame data associated with this frame.
         * @return True if successful, false otherwise.
         */
        bool End(FrameData& frameData) const;

        /**
         * @brief Performs routines required to draw the frame, such as presentation.
         * @note This method should only be called if BeginFrame returned true
         *
         * @param frameData
         * @return true
         * @return false
         */
        bool Present(const FrameData& frameData) const;

        void SetViewport(const vec4& rect) const;
        void ResetViewport() const;

        void SetScissor(const vec4& rect) const;
        void ResetScissor() const;

        void SetWinding(RendererWinding winding) const;

        void CreateTexture(const u8* pixels, Texture* texture) const;
        void CreateWritableTexture(Texture* texture) const;

        void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) const;
        void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels) const;

        void ReadDataFromTexture(Texture* texture, u32 offset, u32 size, void** outMemory) const;
        void ReadPixelFromTexture(Texture* texture, u32 x, u32 y, u8** outRgba) const;

        void DestroyTexture(Texture* texture) const;

        bool CreateGeometry(Geometry& geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount,
                            const void* indices) const;
        bool UploadGeometry(Geometry& geometry) const;
        void UpdateGeometryVertices(const Geometry& geometry, u32 offset, u32 vertexCount, const void* vertices) const;
        void DestroyGeometry(Geometry& geometry) const;

        void DrawGeometry(const GeometryRenderData& data) const;

        bool BeginRenderPass(RenderPass* pass, const C3D::FrameData& frameData) const;
        bool EndRenderPass(RenderPass* pass) const;

        bool CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const;
        void DestroyShader(Shader& shader) const;

        bool InitializeShader(Shader& shader) const;
        bool UseShader(const Shader& shader) const;

        bool BindShaderGlobals(Shader& shader) const;
        bool BindShaderInstance(Shader& shader, u32 instanceId) const;

        bool ShaderApplyGlobals(const Shader& shader, bool needsUpdate) const;
        bool ShaderApplyInstance(const Shader& shader, bool needsUpdate) const;

        bool AcquireShaderInstanceResources(const Shader& shader, u32 textureMapCount, TextureMap** maps, u32* outInstanceId) const;
        bool ReleaseShaderInstanceResources(const Shader& shader, u32 instanceId) const;

        bool AcquireTextureMapResources(TextureMap& map) const;
        void ReleaseTextureMapResources(TextureMap& map) const;

        bool SetUniform(Shader& shader, const ShaderUniform& uniform, const void* value) const;

        void CreateRenderTarget(RenderPass* pass, RenderTarget& target, u32 width, u32 height) const;
        void DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory) const;

        [[nodiscard]] RenderPass* CreateRenderPass(const RenderPassConfig& config) const;
        bool DestroyRenderPass(RenderPass* pass) const;

        [[nodiscard]] Texture* GetWindowAttachment(u8 index) const;
        [[nodiscard]] Texture* GetDepthAttachment(u8 index) const;

        [[nodiscard]] u8 GetWindowAttachmentIndex() const;
        [[nodiscard]] u8 GetWindowAttachmentCount() const;

        [[nodiscard]] RenderBuffer* CreateRenderBuffer(const String& name, RenderBufferType type, u64 totalSize, bool useFreelist) const;
        bool DestroyRenderBuffer(RenderBuffer* buffer) const;

        const Viewport* GetActiveViewport() const;
        void SetActiveViewport(const Viewport* viewport);

        [[nodiscard]] bool IsMultiThreaded() const;

        void SetFlagEnabled(RendererConfigFlagBits flag, bool enabled) const;
        [[nodiscard]] bool IsFlagEnabled(RendererConfigFlagBits flag) const;

    private:
        u8 m_windowRenderTargetCount = 0;
        u32 m_frameBufferWidth = 1280, m_frameBufferHeight = 720;

        RendererPlugin* m_backendPlugin  = nullptr;
        const Viewport* m_activeViewport = nullptr;
    };
}  // namespace C3D
