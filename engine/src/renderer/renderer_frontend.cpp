
#include "renderer_frontend.h"

#include <core/function/function.h>

#include "core/engine.h"
#include "core/logger.h"
#include "geometry.h"
#include "platform/platform.h"
#include "resources/shaders/shader.h"
#include "systems/cvars/cvar_system.h"
#include "systems/system_manager.h"
#include "viewport.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "RENDERER";

    RenderSystem::RenderSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager) {}

    bool RenderSystem::OnInit(const RenderSystemConfig& config)
    {
        m_config = config;

        // Load the backend plugin
        m_backendDynamicLibrary.Load(m_config.rendererPlugin);

        m_backendPlugin = m_backendDynamicLibrary.CreatePlugin<RendererPlugin>();
        if (!m_backendPlugin)
        {
            FATAL_LOG("Failed to create valid renderer plugin.");
            return false;
        }

        RendererPluginConfig rendererPluginConfig{};
        rendererPluginConfig.applicationName = m_config.applicationName;
        rendererPluginConfig.pSystemsManager = m_pSystemsManager;
        rendererPluginConfig.flags           = m_config.flags;

        if (!m_backendPlugin->Init(rendererPluginConfig, &m_windowRenderTargetCount))
        {
            FATAL_LOG("Failed to Initialize Renderer Backend.");
            return false;
        }

        auto& vSync = CVars.Get<bool>("vsync");
        vSync.AddOnChangedCallback([this](const bool& b) { SetFlagEnabled(FlagVSyncEnabled, b); });

        INFO_LOG("Successfully initialized Rendering System.");
        return true;
    }

    void RenderSystem::OnShutdown()
    {
        INFO_LOG("Shutting down.");
        // Shutdown our plugin
        m_backendPlugin->Shutdown();
        // Delete the plugin
        m_backendDynamicLibrary.DeletePlugin(m_backendPlugin);
        // Unload the library
        if (!m_backendDynamicLibrary.Unload())
        {
            ERROR_LOG("Failed to unload backend plugin dynamic library.");
        }
    }

    void RenderSystem::OnResize(const u32 width, const u32 height)
    {
        m_frameBufferWidth  = width;
        m_frameBufferHeight = height;
        m_backendPlugin->OnResize(width, height);
    }

    bool RenderSystem::PrepareFrame(FrameData& frameData)
    {
        // Increment our frame number
        m_backendPlugin->frameNumber++;

        // Reset the draw index for this frame
        m_backendPlugin->drawIndex = 0;

        bool result = m_backendPlugin->PrepareFrame(frameData);

        // Update the frame data with the renderer info
        frameData.frameNumber       = m_backendPlugin->frameNumber;
        frameData.drawIndex         = m_backendPlugin->drawIndex;
        frameData.renderTargetIndex = m_backendPlugin->GetWindowAttachmentIndex();

        return result;
    }

    bool RenderSystem::Begin(const FrameData& frameData) const { return m_backendPlugin->Begin(frameData); }

    bool RenderSystem::End(FrameData& frameData) const
    {
        bool result = m_backendPlugin->End(frameData);
        // Increment the draw index for this frame
        m_backendPlugin->drawIndex++;
        // Sync the frame data to it
        frameData.drawIndex = m_backendPlugin->drawIndex;
        return result;
    }

    bool RenderSystem::Present(const FrameData& frameData) const
    {
        if (!m_backendPlugin->Present(frameData))
        {
            ERROR_LOG("Failed to present. Application is shutting down.");
            return false;
        }
        return true;
    }

    void RenderSystem::SetViewport(const vec4& rect) const { m_backendPlugin->SetViewport(rect); }

    void RenderSystem::ResetViewport() const { m_backendPlugin->ResetViewport(); }

    void RenderSystem::SetScissor(const vec4& rect) const { m_backendPlugin->SetScissor(rect); }

    void RenderSystem::ResetScissor() const { m_backendPlugin->ResetScissor(); }

    void RenderSystem::SetWinding(const RendererWinding winding) const { m_backendPlugin->SetWinding(winding); }

    void RenderSystem::CreateTexture(const u8* pixels, Texture* texture) const { m_backendPlugin->CreateTexture(pixels, texture); }

    void RenderSystem::CreateWritableTexture(Texture* texture) const { m_backendPlugin->CreateWritableTexture(texture); }

    void RenderSystem::ResizeTexture(Texture* texture, const u32 newWidth, const u32 newHeight) const
    {
        m_backendPlugin->ResizeTexture(texture, newWidth, newHeight);
    }

    void RenderSystem::WriteDataToTexture(Texture* texture, const u32 offset, const u32 size, const u8* pixels) const
    {
        m_backendPlugin->WriteDataToTexture(texture, offset, size, pixels);
    }

    void RenderSystem::ReadDataFromTexture(Texture* texture, const u32 offset, const u32 size, void** outMemory) const
    {
        m_backendPlugin->ReadDataFromTexture(texture, offset, size, outMemory);
    }

    void RenderSystem::ReadPixelFromTexture(Texture* texture, const u32 x, const u32 y, u8** outRgba) const
    {
        m_backendPlugin->ReadPixelFromTexture(texture, x, y, outRgba);
    }

    void RenderSystem::DestroyTexture(Texture* texture) const { m_backendPlugin->DestroyTexture(texture); }

    bool RenderSystem::CreateGeometry(Geometry& geometry, const u32 vertexSize, const u64 vertexCount, const void* vertices,
                                      const u32 indexSize, const u64 indexCount, const void* indices) const
    {
        if (!vertexCount || !vertices || !vertexSize)
        {
            ERROR_LOG("Invalid vertex data was supplied.");
            return false;
        }

        geometry.material = nullptr;

        // Invalidate IDs
        geometry.internalId = INVALID_ID;
        geometry.generation = INVALID_ID_U16;

        // Take a copy of our vertex data
        geometry.vertexCount       = vertexCount;
        geometry.vertexElementSize = vertexSize;
        geometry.vertices          = Memory.AllocateBlock(MemoryType::RenderSystem, vertexSize * vertexCount);
        std::memcpy(geometry.vertices, vertices, vertexSize * vertexCount);

        geometry.indexCount       = indexCount;
        geometry.indexElementSize = indexSize;
        geometry.indices          = nullptr;

        // If index data is supplied we take a copy of it
        if (indexSize && indexCount)
        {
            geometry.indices = Memory.AllocateBlock(MemoryType::RenderSystem, indexSize * indexCount);
            std::memcpy(geometry.indices, indices, indexSize * indexCount);
        }

        return m_backendPlugin->CreateGeometry(geometry);
    }

    bool RenderSystem::UploadGeometry(Geometry& geometry) const
    {
        return m_backendPlugin->UploadGeometry(geometry, 0, geometry.vertexElementSize * geometry.vertexCount, 0,
                                               geometry.indexElementSize * geometry.indexCount);
    }

    void RenderSystem::UpdateGeometryVertices(const Geometry& geometry, u32 offset, u32 vertexCount, const void* vertices) const
    {
        m_backendPlugin->UpdateGeometryVertices(geometry, offset, vertexCount, vertices);
    }

    void RenderSystem::DestroyGeometry(Geometry& geometry) const
    {
        // Call the backend to cleanup our internal API specific geometry data
        m_backendPlugin->DestroyGeometry(geometry);

        // Free the memory that we've allocated for the vertices in the CreateGeometry method
        Memory.Free(geometry.vertices);
        geometry.vertexCount       = 0;
        geometry.vertexElementSize = 0;
        geometry.vertices          = nullptr;

        if (geometry.indices)
        {
            // Free the memory that we've allocated for the indices in the CreateGeometry method
            Memory.Free(geometry.indices);
            geometry.indexCount       = 0;
            geometry.indexElementSize = 0;
            geometry.indices          = nullptr;
        }
    }

    void RenderSystem::DrawGeometry(const GeometryRenderData& data) const
    {
        static bool currentWindingInverted = data.windingInverted;

        if (currentWindingInverted != data.windingInverted)
        {
            currentWindingInverted = data.windingInverted;
            m_backendPlugin->SetWinding(currentWindingInverted ? RendererWinding::Clockwise : RendererWinding::CounterClockwise);
        }

        m_backendPlugin->DrawGeometry(data);
    }

    bool RenderSystem::BeginRenderPass(RenderPass* pass, const C3D::FrameData& frameData) const
    {
        return m_backendPlugin->BeginRenderPass(pass, frameData);
    }

    bool RenderSystem::EndRenderPass(RenderPass* pass) const { return m_backendPlugin->EndRenderPass(pass); }

    bool RenderSystem::CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const
    {
        return m_backendPlugin->CreateShader(shader, config, pass);
    }

    void RenderSystem::DestroyShader(Shader& shader) const { return m_backendPlugin->DestroyShader(shader); }

    bool RenderSystem::InitializeShader(Shader& shader) const { return m_backendPlugin->InitializeShader(shader); }

    bool RenderSystem::UseShader(const Shader& shader) const { return m_backendPlugin->UseShader(shader); }

    bool RenderSystem::BindShaderGlobals(Shader& shader) const { return m_backendPlugin->BindShaderGlobals(shader); }

    bool RenderSystem::BindShaderInstance(Shader& shader, u32 instanceId) const
    {
        return m_backendPlugin->BindShaderInstance(shader, instanceId);
    }

    bool RenderSystem::ShaderApplyGlobals(const Shader& shader, bool needsUpdate) const
    {
        return m_backendPlugin->ShaderApplyGlobals(shader, needsUpdate);
    }

    bool RenderSystem::ShaderApplyInstance(const Shader& shader, const bool needsUpdate) const
    {
        return m_backendPlugin->ShaderApplyInstance(shader, needsUpdate);
    }

    bool RenderSystem::AcquireShaderInstanceResources(const Shader& shader, u32 textureMapCount, TextureMap** maps,
                                                      u32* outInstanceId) const
    {
        return m_backendPlugin->AcquireShaderInstanceResources(shader, textureMapCount, maps, outInstanceId);
    }

    bool RenderSystem::ReleaseShaderInstanceResources(const Shader& shader, const u32 instanceId) const
    {
        return m_backendPlugin->ReleaseShaderInstanceResources(shader, instanceId);
    }

    bool RenderSystem::AcquireTextureMapResources(TextureMap& map) const { return m_backendPlugin->AcquireTextureMapResources(map); }

    void RenderSystem::ReleaseTextureMapResources(TextureMap& map) const { m_backendPlugin->ReleaseTextureMapResources(map); }

    bool RenderSystem::SetUniform(Shader& shader, const ShaderUniform& uniform, const void* value) const
    {
        return m_backendPlugin->SetUniform(shader, uniform, value);
    }

    void RenderSystem::CreateRenderTarget(RenderPass* pass, RenderTarget& target, u32 width, u32 height) const
    {
        m_backendPlugin->CreateRenderTarget(pass, target, width, height);
    }

    void RenderSystem::DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory) const
    {
        m_backendPlugin->DestroyRenderTarget(target, freeInternalMemory);
    }

    RenderPass* RenderSystem::CreateRenderPass(const RenderPassConfig& config) const { return m_backendPlugin->CreateRenderPass(config); }

    bool RenderSystem::DestroyRenderPass(RenderPass* pass) const { return m_backendPlugin->DestroyRenderPass(pass); }

    Texture* RenderSystem::GetWindowAttachment(const u8 index) const { return m_backendPlugin->GetWindowAttachment(index); }

    Texture* RenderSystem::GetDepthAttachment(const u8 index) const { return m_backendPlugin->GetDepthAttachment(index); }

    u8 RenderSystem::GetWindowAttachmentIndex() const { return m_backendPlugin->GetWindowAttachmentIndex(); }

    u8 RenderSystem::GetWindowAttachmentCount() const { return m_backendPlugin->GetWindowAttachmentCount(); }

    RenderBuffer* RenderSystem::CreateRenderBuffer(const String& name, const RenderBufferType type, const u64 totalSize,
                                                   const bool useFreelist) const
    {
        return m_backendPlugin->CreateRenderBuffer(name, type, totalSize, useFreelist);
    }

    bool RenderSystem::DestroyRenderBuffer(RenderBuffer* buffer) const { return m_backendPlugin->DestroyRenderBuffer(buffer); }

    const Viewport* RenderSystem::GetActiveViewport() const { return m_activeViewport; }

    void RenderSystem::SetActiveViewport(const Viewport* viewport)
    {
        m_activeViewport = viewport;

        const auto& rect2D = viewport->GetRect2D();

        const vec4 vp = { rect2D.x, rect2D.y + rect2D.height, rect2D.width, -rect2D.height };
        m_backendPlugin->SetViewport(vp);

        const vec4 sc = { rect2D.x, rect2D.y, rect2D.width, rect2D.height };
        m_backendPlugin->SetScissor(sc);
    }

    bool RenderSystem::IsMultiThreaded() const { return m_backendPlugin->IsMultiThreaded(); }

    void RenderSystem::SetFlagEnabled(const RendererConfigFlagBits flag, const bool enabled) const
    {
        m_backendPlugin->SetFlagEnabled(flag, enabled);
    }

    bool RenderSystem::IsFlagEnabled(const RendererConfigFlagBits flag) const { return m_backendPlugin->IsFlagEnabled(flag); }
}  // namespace C3D
