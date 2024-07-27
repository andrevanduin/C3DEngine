
#include "renderer_frontend.h"

#include <core/function/function.h>

#include "core/engine.h"
#include "core/logger.h"
#include "geometry.h"
#include "platform/platform.h"
#include "renderer_utils.h"
#include "resources/loaders/text_loader.h"
#include "resources/shaders/shader.h"
#include "systems/cvars/cvar_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"
#include "vertex.h"
#include "viewport.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "RENDERER";

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
        rendererPluginConfig.flags           = m_config.flags;

        if (!m_backendPlugin->Init(rendererPluginConfig, &m_windowRenderTargetCount))
        {
            FATAL_LOG("Failed to Initialize Renderer Backend.");
            return false;
        }

        // Create and bind our buffers
        constexpr u64 vertexBufferSize = sizeof(Vertex3D) * 4096 * 4096;
        m_geometryVertexBuffer = m_backendPlugin->CreateRenderBuffer("GEOMETRY_VERTEX_BUFFER", RenderBufferType::Vertex, vertexBufferSize,
                                                                     RenderBufferTrackType::FreeList);
        if (!m_geometryVertexBuffer)
        {
            ERROR_LOG("Error creating vertex buffer.");
            return false;
        }
        m_geometryVertexBuffer->Bind(0);

        constexpr u64 indexBufferSize = sizeof(u32) * 8192 * 8192;
        m_geometryIndexBuffer = m_backendPlugin->CreateRenderBuffer("GEOMETRY_INDEX_BUFFER", RenderBufferType::Index, indexBufferSize,
                                                                    RenderBufferTrackType::FreeList);
        if (!m_geometryIndexBuffer)
        {
            ERROR_LOG("Error creating index buffer.");
            return false;
        }
        m_geometryIndexBuffer->Bind(0);

        auto& vSync = CVars.Get("vsync");
        vSync.AddOnChangeCallback([this](const CVar& cvar) { SetFlagEnabled(FlagVSyncEnabled, cvar.GetValue<bool>()); });

        INFO_LOG("Successfully initialized Rendering System.");
        return true;
    }

    void RenderSystem::OnShutdown()
    {
        INFO_LOG("Shutting down.");
        // Destroy our render buffers
        m_backendPlugin->DestroyRenderBuffer(m_geometryVertexBuffer);
        m_backendPlugin->DestroyRenderBuffer(m_geometryIndexBuffer);
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

    void RenderSystem::SetStencilTestingEnabled(bool enabled) const { m_backendPlugin->SetStencilTestingEnabled(enabled); }

    void RenderSystem::SetStencilReference(u32 reference) const { m_backendPlugin->SetStencilReference(reference); }

    void RenderSystem::SetStencilCompareMask(u8 compareMask) const { m_backendPlugin->SetStencilCompareMask(compareMask); }

    void RenderSystem::SetStencilWriteMask(u8 writeMask) const { m_backendPlugin->SetStencilWriteMask(writeMask); }

    void RenderSystem::SetStencilOperation(StencilOperation failOp, StencilOperation passOp, StencilOperation depthFailOp,
                                           CompareOperation compareOp) const
    {
        m_backendPlugin->SetStencilOperation(failOp, passOp, depthFailOp, compareOp);
    }

    void RenderSystem::SetDepthTestingEnabled(bool enabled) const { m_backendPlugin->SetDepthTestingEnabled(enabled); }

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
        geometry.generation = INVALID_ID_U16;

        // Take a copy of our vertex data
        geometry.vertexCount = vertexCount;
        geometry.vertexSize  = vertexSize;
        geometry.vertices    = Memory.AllocateBlock(MemoryType::RenderSystem, vertexSize * vertexCount);
        std::memcpy(geometry.vertices, vertices, vertexSize * vertexCount);
        geometry.vertexBufferOffset = INVALID_ID_U64;

        geometry.indexCount        = indexCount;
        geometry.indexSize         = indexSize;
        geometry.indices           = nullptr;
        geometry.indexBufferOffset = INVALID_ID_U64;

        // If index data is supplied we take a copy of it
        if (indexSize && indexCount)
        {
            geometry.indices = Memory.AllocateBlock(MemoryType::RenderSystem, indexSize * indexCount);
            std::memcpy(geometry.indices, indices, indexSize * indexCount);
        }

        return true;
    }

    bool RenderSystem::UploadGeometry(Geometry& geometry)
    {
        // Check if this is a reupload. If it is we don't need to allocate
        bool isReupload = geometry.generation != INVALID_ID_U16;
        u64 vertexSize  = geometry.vertexSize * geometry.vertexCount;
        u64 indexSize   = geometry.indexSize * geometry.indexCount;

        if (!isReupload)
        {
            // Allocate space in the buffer.
            if (!m_geometryVertexBuffer->Allocate(vertexSize, geometry.vertexBufferOffset))
            {
                ERROR_LOG("Failed to allocate memory frome the vertex buffer.");
                return false;
            }
        }

        // Load the data
        if (!m_geometryVertexBuffer->LoadRange(geometry.vertexBufferOffset, vertexSize, geometry.vertices))
        {
            ERROR_LOG("Failed to upload to the vertex buffer.");
            return false;
        }

        if (geometry.indexCount && geometry.indices && indexSize)
        {
            if (!isReupload)
            {
                // Allocate space in the buffer.
                if (!m_geometryIndexBuffer->Allocate(indexSize, geometry.indexBufferOffset))
                {
                    ERROR_LOG("Failed to allocate memory frome the index buffer.");
                    return false;
                }
            }

            // Load the data
            if (!m_geometryIndexBuffer->LoadRange(geometry.indexBufferOffset, indexSize, geometry.indices))
            {
                ERROR_LOG("Failed to upload to the index buffer.");
                return false;
            }
        }

        // Increment the generation since we now have changed this geometry
        geometry.generation++;

        return true;
    }

    void RenderSystem::UpdateGeometryVertices(const Geometry& geometry, u32 offset, u32 vertexCount, const void* vertices) const
    {
        u64 vertexSize = geometry.vertexSize * vertexCount;
        if (!m_geometryVertexBuffer->LoadRange(geometry.vertexBufferOffset + offset, vertexSize, vertices))
        {
            ERROR_LOG("Failed to LoadRange for the provided vertices.");
        }
    }

    void RenderSystem::DestroyGeometry(Geometry& geometry) const
    {
        if (geometry.generation != INVALID_ID_U16)
        {
            u64 vertexDataSize = geometry.vertexSize * geometry.vertexCount;
            if (vertexDataSize > 0)
            {
                if (!m_geometryVertexBuffer->Free(vertexDataSize, geometry.vertexBufferOffset))
                {
                    ERROR_LOG("Failed to free Geometry Vertex Buffer data.");
                }
            }

            u64 indexDataSize = geometry.indexSize * geometry.indexCount;
            if (indexDataSize > 0)
            {
                if (!m_geometryIndexBuffer->Free(indexDataSize, geometry.indexBufferOffset))
                {
                    ERROR_LOG("Failed to free Geometry Index Buffer data.");
                }
            }

            geometry.generation = INVALID_ID_U16;
            geometry.name.Destroy();
        }

        if (geometry.vertices)
        {
            Memory.Free(geometry.vertices);
            geometry.vertices    = nullptr;
            geometry.vertexCount = 0;
            geometry.vertexSize  = 0;
        }

        if (geometry.indices)
        {
            Memory.Free(geometry.indices);
            geometry.indices    = nullptr;
            geometry.indexCount = 0;
            geometry.indexSize  = 0;
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

        bool includesIndexData = data.indexCount > 0;

        if (!m_geometryVertexBuffer->Draw(data.vertexBufferOffset, data.vertexCount, includesIndexData))
        {
            ERROR_LOG("Failed to draw Vertex Buffer.");
            return;
        }

        if (includesIndexData)
        {
            if (!m_geometryIndexBuffer->Draw(data.indexBufferOffset, data.indexCount, !includesIndexData))
            {
                ERROR_LOG("Failed to draw Index Buffer.");
            }
        }
    }

    void RenderSystem::BeginRenderpass(void* pass, const RenderTarget& target) const
    {
        m_backendPlugin->BeginRenderpass(pass, GetActiveViewport(), target);
    }

    void RenderSystem::EndRenderpass(void* pass) const { m_backendPlugin->EndRenderpass(pass); }

    bool RenderSystem::CreateShader(Shader& shader, const ShaderConfig& config, void* pass) const
    {
        // Get the uniform count
        shader.globalUniformCount        = 0;
        shader.globalUniformSamplerCount = 0;
        shader.globalSamplerIndices.Clear();

        shader.instanceUniformCount        = 0;
        shader.instanceUniformSamplerCount = 0;
        shader.instanceSamplerIndices.Clear();

        shader.localUniformCount = 0;

        for (u32 i = 0; i < config.uniforms.Size(); ++i)
        {
            auto& uniform = config.uniforms[i];

            switch (uniform.scope)
            {
                case ShaderScope::Global:
                    if (UniformTypeIsASampler(uniform.type))
                    {
                        shader.globalUniformSamplerCount++;

                        auto index = static_cast<u16>(shader.uniforms.GetIndex(uniform.name));
                        shader.globalSamplerIndices.PushBack(index);
                    }
                    else
                    {
                        shader.globalUniformCount++;
                    }
                    break;
                case ShaderScope::Instance:
                    if (UniformTypeIsASampler(uniform.type))
                    {
                        shader.instanceUniformSamplerCount++;

                        auto index = static_cast<u16>(shader.uniforms.GetIndex(uniform.name));
                        shader.instanceSamplerIndices.PushBack(index);
                    }
                    else
                    {
                        shader.instanceUniformCount++;
                    }
                    break;
                case ShaderScope::Local:
                    shader.localUniformCount++;
                    break;
            }
        }

        // Load the shader stage files and feed them to the backend to be compiled
        // TODO: We should handle #include directives in the shaders here so it independent from renderer backend

        shader.stageConfigs = config.stageConfigs;

        for (auto& stageConfig : shader.stageConfigs)
        {
            TextResource source;
            if (!Resources.Load(stageConfig.fileName, source))
            {
                ERROR_LOG("Failed to read shader file: '{}'.", stageConfig.fileName);
            }

            stageConfig.source = source.text;

            Resources.Unload(source);
        }

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

    bool RenderSystem::BindShaderLocal(Shader& shader) const { return m_backendPlugin->BindShaderLocal(shader); }

    bool RenderSystem::ShaderApplyGlobals(const FrameData& frameData, const Shader& shader, bool needsUpdate) const
    {
        return m_backendPlugin->ShaderApplyGlobals(frameData, shader, needsUpdate);
    }

    bool RenderSystem::ShaderApplyInstance(const FrameData& frameData, const Shader& shader, const bool needsUpdate) const
    {
        return m_backendPlugin->ShaderApplyInstance(frameData, shader, needsUpdate);
    }

    bool RenderSystem::ShaderApplyLocal(const FrameData& frameData, const Shader& shader) const
    {
        return m_backendPlugin->ShaderApplyLocal(frameData, shader);
    }

    bool RenderSystem::AcquireShaderInstanceResources(const Shader& shader, const ShaderInstanceResourceConfig& config,
                                                      u32& outInstanceId) const
    {
        return m_backendPlugin->AcquireShaderInstanceResources(shader, config, outInstanceId);
    }

    bool RenderSystem::ReleaseShaderInstanceResources(const Shader& shader, const u32 instanceId) const
    {
        return m_backendPlugin->ReleaseShaderInstanceResources(shader, instanceId);
    }

    bool RenderSystem::AcquireTextureMapResources(TextureMap& map) const { return m_backendPlugin->AcquireTextureMapResources(map); }

    void RenderSystem::ReleaseTextureMapResources(TextureMap& map) const { m_backendPlugin->ReleaseTextureMapResources(map); }

    bool RenderSystem::SetUniform(Shader& shader, const ShaderUniform& uniform, u32 arrayIndex, const void* value) const
    {
        return m_backendPlugin->SetUniform(shader, uniform, arrayIndex, value);
    }

    void RenderSystem::CreateRenderTarget(void* pass, RenderTarget& target, u16 layerIndex, u32 width, u32 height) const
    {
        m_backendPlugin->CreateRenderTarget(pass, target, layerIndex, width, height);
    }

    void RenderSystem::DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory) const
    {
        m_backendPlugin->DestroyRenderTarget(target, freeInternalMemory);
    }

    void RenderSystem::CreateRenderpassInternals(const RenderpassConfig& config, void** internalData) const
    {
        m_backendPlugin->CreateRenderpassInternals(config, internalData);
    }

    void RenderSystem::DestroyRenderpassInternals(void* internalData) const { m_backendPlugin->DestroyRenderpassInternals(internalData); }

    Texture* RenderSystem::GetWindowAttachment(const u8 index) const { return m_backendPlugin->GetWindowAttachment(index); }

    Texture* RenderSystem::GetDepthAttachment(const u8 index) const { return m_backendPlugin->GetDepthAttachment(index); }

    u8 RenderSystem::GetWindowAttachmentIndex() const { return m_backendPlugin->GetWindowAttachmentIndex(); }

    u8 RenderSystem::GetWindowAttachmentCount() const { return m_backendPlugin->GetWindowAttachmentCount(); }

    RenderBuffer* RenderSystem::CreateRenderBuffer(const String& name, const RenderBufferType type, const u64 totalSize,
                                                   RenderBufferTrackType trackType) const
    {
        return m_backendPlugin->CreateRenderBuffer(name, type, totalSize, trackType);
    }

    bool RenderSystem::AllocateInRenderBuffer(RenderBufferType type, u64 size, u64& offset)
    {
        switch (type)
        {
            case RenderBufferType::Vertex:
                return m_geometryVertexBuffer->Allocate(size, offset);
            case RenderBufferType::Index:
                return m_geometryIndexBuffer->Allocate(size, offset);
            default:
                ERROR_LOG("Invalid RenderBufferType provided.");
                return false;
        }
    }

    bool RenderSystem::FreeInRenderBuffer(RenderBufferType type, u64 size, u64 offset)
    {
        switch (type)
        {
            case RenderBufferType::Vertex:
                return m_geometryVertexBuffer->Free(size, offset);
            case RenderBufferType::Index:
                return m_geometryIndexBuffer->Free(size, offset);
            default:
                ERROR_LOG("Invalid RenderBufferType provided.");
                return false;
        }
    }

    bool RenderSystem::LoadRangeInRenderBuffer(RenderBufferType type, u64 offset, u64 size, const void* data)
    {
        switch (type)
        {
            case RenderBufferType::Vertex:
                return m_geometryVertexBuffer->LoadRange(offset, size, data);
            case RenderBufferType::Index:
                return m_geometryIndexBuffer->LoadRange(offset, size, data);
            default:
                ERROR_LOG("Invalid RenderBufferType provided.");
                return false;
        }
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
