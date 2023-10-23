
#include "renderer_frontend.h"

#include <core/function/function.h>

#include "core/engine.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "resources/shaders/shader.h"
#include "systems/cvars/cvar_system.h"
#include "systems/render_views/render_view_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    RenderSystem::RenderSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager, "RENDERER") {}

    bool RenderSystem::Init(const RenderSystemConfig& config)
    {
        m_config        = config;
        m_backendPlugin = config.rendererPlugin;
        if (!m_backendPlugin)
        {
            m_logger.Fatal("Init() - No valid renderer plugin provided");
            return false;
        }

        RendererPluginConfig rendererPluginConfig{};
        rendererPluginConfig.applicationName = m_config.applicationName;
        rendererPluginConfig.pSystemsManager = m_pSystemsManager;
        rendererPluginConfig.flags           = m_config.flags;

        if (!m_backendPlugin->Init(rendererPluginConfig, &m_windowRenderTargetCount))
        {
            m_logger.Fatal("Init() - Failed to Initialize Renderer Backend.");
            return false;
        }

        auto& vSync = CVars.Get<bool>("vsync");
        vSync.AddOnChangedCallback([this](const bool& b) { SetFlagEnabled(FlagVSyncEnabled, b); });

        m_logger.Info("Init() - Successfully initialized Rendering System");
        return true;
    }

    void RenderSystem::Shutdown()
    {
        m_logger.Info("Shutting Down");

        m_backendPlugin->Shutdown();
        Memory.Delete(MemoryType::RenderSystem, m_backendPlugin);
    }

    void RenderSystem::OnResize(const u32 width, const u32 height)
    {
        m_resizing          = true;
        m_frameBufferWidth  = width;
        m_frameBufferHeight = height;
        m_framesSinceResize = 0;
    }

    bool RenderSystem::DrawFrame(RenderPacket* packet, const FrameData& frameData)
    {
        m_backendPlugin->frameNumber++;

        if (m_resizing)
        {
            m_framesSinceResize++;

            if (m_framesSinceResize >= 5)
            {
                // Notify our views of the resize
                Views.OnWindowResize(m_frameBufferWidth, m_frameBufferHeight);
                m_backendPlugin->OnResize(m_frameBufferWidth, m_frameBufferHeight);

                m_framesSinceResize = 0;
                m_resizing          = false;
            }
            else
            {
                // Simulate a 60FPS frame
                Platform::SleepMs(16);
                // Skip rendering this frame
                return true;
            }
        }

        if (m_backendPlugin->BeginFrame(frameData))
        {
            const u8 attachmentIndex = m_backendPlugin->GetWindowAttachmentIndex();

            // Render each view
            for (auto& viewPacket : packet->views)
            {
                if (!Views.OnRender(frameData, viewPacket.view, &viewPacket, m_backendPlugin->frameNumber, attachmentIndex))
                {
                    m_logger.Error("DrawFrame() - Failed on calling OnRender() for view: '{}'.", viewPacket.view->GetName());
                    return false;
                }
            }

            // End frame
            if (!m_backendPlugin->EndFrame(frameData))
            {
                m_logger.Error("DrawFrame() - EndFrame() failed");
                return false;
            }
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
            m_logger.Error("CreateGeometry() - Invalid vertex data was supplied.");
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

    void RenderSystem::DestroyGeometry(Geometry& geometry) const { m_backendPlugin->DestroyGeometry(geometry); }

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

    bool RenderSystem::BeginRenderPass(RenderPass* pass, RenderTarget* target) const
    {
        return m_backendPlugin->BeginRenderPass(pass, target);
    }

    bool RenderSystem::EndRenderPass(RenderPass* pass) const { return m_backendPlugin->EndRenderPass(pass); }

    bool RenderSystem::CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const
    {
        return m_backendPlugin->CreateShader(shader, config, pass);
    }

    void RenderSystem::DestroyShader(Shader& shader) const { return m_backendPlugin->DestroyShader(shader); }

    bool RenderSystem::InitializeShader(Shader& shader) const { return m_backendPlugin->InitializeShader(shader); }

    bool RenderSystem::UseShader(const Shader& shader) const { return m_backendPlugin->UseShader(shader); }

    bool RenderSystem::ShaderBindGlobals(Shader& shader) const { return m_backendPlugin->ShaderBindGlobals(shader); }

    bool RenderSystem::ShaderBindInstance(Shader& shader, u32 instanceId) const
    {
        return m_backendPlugin->ShaderBindInstance(shader, instanceId);
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

    void RenderSystem::CreateRenderTarget(const u8 attachmentCount, RenderTargetAttachment* attachments, RenderPass* pass, const u32 width,
                                          const u32 height, RenderTarget* outTarget) const
    {
        m_backendPlugin->CreateRenderTarget(attachmentCount, attachments, pass, width, height, outTarget);
    }

    void RenderSystem::DestroyRenderTarget(RenderTarget* target, const bool freeInternalMemory) const
    {
        m_backendPlugin->DestroyRenderTarget(target, freeInternalMemory);
        if (freeInternalMemory)
        {
            std::memset(target, 0, sizeof(RenderTarget));
        }
    }

    RenderPass* RenderSystem::CreateRenderPass(const RenderPassConfig& config) const { return m_backendPlugin->CreateRenderPass(config); }

    bool RenderSystem::DestroyRenderPass(RenderPass* pass) const
    {
        // Destroy this pass's render targets first
        for (u8 i = 0; i < pass->renderTargetCount; i++)
        {
            DestroyRenderTarget(&pass->targets[i], true);
        }
        return m_backendPlugin->DestroyRenderPass(pass);
    }

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

    bool RenderSystem::IsMultiThreaded() const { return m_backendPlugin->IsMultiThreaded(); }

    void RenderSystem::SetFlagEnabled(const RendererConfigFlagBits flag, const bool enabled) const
    {
        m_backendPlugin->SetFlagEnabled(flag, enabled);
    }

    bool RenderSystem::IsFlagEnabled(const RendererConfigFlagBits flag) const { return m_backendPlugin->IsFlagEnabled(flag); }
}  // namespace C3D
