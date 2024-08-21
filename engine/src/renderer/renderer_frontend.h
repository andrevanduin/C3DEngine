
#pragma once
#include "core/defines.h"
#include "core/dynamic_library/dynamic_library.h"
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
        const char* rendererPlugin;
        RendererConfigFlags flags;
    };

    class C3D_API RenderSystem final : public SystemWithConfig<RenderSystemConfig>
    {
    public:
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

        /**
         * @brief Sets Stencil testing to enabled or disabled.
         *
         * @param enabled Bool indicating if you want Stencil testing enabled or disabled
         */
        void SetStencilTestingEnabled(bool enabled) const;

        /**
         * @brief Sets the Stencil Reference for testing.
         *
         * @param reference The reference to use when stencil testing/writing
         */
        void SetStencilReference(u32 reference) const;

        /**
         * @brief Sets the Stencil Compare Mask.
         *
         * @param compareMask The value to use for the Stencil Compare Mask
         */
        void SetStencilCompareMask(u8 compareMask) const;

        /**
         * @brief Sets the Stencil Write Mask.
         *
         * @param writeMask The value to use for the Stencil Write Mask
         */
        void SetStencilWriteMask(u8 writeMask) const;

        /**
         * @brief Sets the Stencil operation.
         *
         * @param failOp The action that should be performed on samples that fail the stencil test
         * @param passOp The action that should be performed on samples that pass both depth and stencil tests
         * @param depthFailOp The action that should be performed on samples that pass the stencil test but fail the depth test
         * @param compareOp The comparison operaion used in the stencil test
         */
        void SetStencilOperation(StencilOperation failOp, StencilOperation passOp, StencilOperation depthFailOp,
                                 CompareOperation compareOp) const;

        /**
         * @brief Sets Depth testing to enabled or disabled.
         *
         * @param enabled Bool indicating if you want Depth testing enabled or disabled
         */
        void SetDepthTestingEnabled(bool enabled) const;

        void CreateTexture(Texture& texture, const u8* pixels) const;
        void CreateWritableTexture(Texture& texture) const;

        void ResizeTexture(Texture& texture, u32 newWidth, u32 newHeight) const;
        void WriteDataToTexture(Texture& texture, u32 offset, u32 size, const u8* pixels) const;

        void ReadDataFromTexture(Texture& texture, u32 offset, u32 size, void** outMemory) const;
        void ReadPixelFromTexture(Texture& texture, u32 x, u32 y, u8** outRgba) const;

        void DestroyTexture(Texture& texture) const;

        bool CreateGeometry(Geometry& geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount,
                            const void* indices) const;
        bool UploadGeometry(Geometry& geometry);
        void UpdateGeometryVertices(const Geometry& geometry, u32 offset, u32 vertexCount, const void* vertices,
                                    bool includeInFrameWorkload) const;
        void DestroyGeometry(Geometry& geometry) const;

        void DrawGeometry(const GeometryRenderData& data) const;

        void BeginRenderpass(void* pass, const RenderTarget& target) const;
        void EndRenderpass(void* pass) const;

        bool CreateShader(Shader& shader, const ShaderConfig& config, void* pass) const;
        bool ReloadShader(Shader& shader) const;
        void DestroyShader(Shader& shader) const;

        bool InitializeShader(Shader& shader) const;
        bool UseShader(const Shader& shader) const;

        bool BindShaderGlobals(Shader& shader) const;
        bool BindShaderInstance(Shader& shader, u32 instanceId) const;
        bool BindShaderLocal(Shader& shader) const;

        bool ShaderApplyGlobals(const FrameData& frameData, const Shader& shader, bool needsUpdate) const;
        bool ShaderApplyInstance(const FrameData& frameData, const Shader& shader, bool needsUpdate) const;
        bool ShaderApplyLocal(const FrameData& frameData, const Shader& shader) const;

        /**
         * @brief Enables/disables wireframe rendering for the provided shader.
         *
         * @param shader The shader for which you want to enable/disable wireframe
         * @param enabled A boolean indicating if wireframe should be enabled or disabled
         * @return True if successfully switched; false if unsupported or failed
         */
        bool ShaderSetWireframe(Shader& shader, bool enabled) const;

        bool AcquireShaderInstanceResources(const Shader& shader, const ShaderInstanceResourceConfig& config, u32& outInstanceId) const;
        bool ReleaseShaderInstanceResources(const Shader& shader, u32 instanceId) const;

        bool AcquireTextureMapResources(TextureMap& map) const;
        void ReleaseTextureMapResources(TextureMap& map) const;

        bool SetUniform(Shader& shader, const ShaderUniform& uniform, u32 arrayIndex, const void* value) const;

        void CreateRenderTarget(void* pass, RenderTarget& target, u16 layerIndex, u32 width, u32 height) const;
        void DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory) const;

        void CreateRenderpassInternals(const RenderpassConfig& config, void** internalData) const;
        void DestroyRenderpassInternals(void* internalData) const;

        [[nodiscard]] RenderBuffer* CreateRenderBuffer(const String& name, RenderBufferType type, u64 totalSize,
                                                       RenderBufferTrackType trackType) const;

        bool AllocateInRenderBuffer(RenderBufferType type, u64 size, u64& outOffset);
        bool FreeInRenderBuffer(RenderBufferType type, u64 size, u64 offset);
        bool LoadRangeInRenderBuffer(RenderBufferType type, u64 offset, u64 size, const void* data, bool includeInFrameWorkload);

        bool DestroyRenderBuffer(RenderBuffer* buffer) const;

        /**
         * @brief Begins the marking of a section of render commands under the provided name and color.
         * Each renderer backend will have a different implementation of this (or possibly an empty implementation)
         * NOTE: For non-debug builds this method becomes a no-op
         *
         * @param text The debug text/name you want to use for your label
         * @param color The color of your label
         */
        void BeginDebugLabel(const String& text, const vec3& color) const;

        /**
         * @brief Ends the last debug section of commands.
         * Each renderer backend will have a different implementation of this (or possibly an empty implementation)
         * NOTE: For non-debug builds this method becomes a no-op
         */
        void EndDebugLabel() const;

        /**
         * @brief Wait for the renderer backend to be completly idle.
         * NOTE: This incurs a lot of overhead so only use this method in non-time critcal code
         */
        void WaitForIdle() const;

        const Viewport* GetActiveViewport() const;
        void SetActiveViewport(const Viewport* viewport);

        [[nodiscard]] bool IsMultiThreaded() const;

        void SetFlagEnabled(RendererConfigFlagBits flag, bool enabled) const;
        [[nodiscard]] bool IsFlagEnabled(RendererConfigFlagBits flag) const;

        [[nodiscard]] TextureHandle GetWindowAttachment(u8 index) const;
        [[nodiscard]] TextureHandle GetDepthAttachment(u8 index) const;

        [[nodiscard]] u8 GetWindowAttachmentIndex() const;
        [[nodiscard]] u8 GetWindowAttachmentCount() const;

    private:
        u8 m_windowRenderTargetCount = 0;
        u32 m_frameBufferWidth = 1280, m_frameBufferHeight = 720;

        RenderBuffer* m_geometryVertexBuffer;
        RenderBuffer* m_geometryIndexBuffer;

        DynamicLibrary m_backendDynamicLibrary;
        RendererPlugin* m_backendPlugin = nullptr;

        const Viewport* m_activeViewport = nullptr;
    };
}  // namespace C3D
