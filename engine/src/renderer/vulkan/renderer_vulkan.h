
#pragma once
#include "vulkan_types.h"
#include "vulkan_buffer.h"
#include "vulkan_shader.h"

#include "renderer/renderer_backend.h"

namespace C3D
{
	class RendererVulkan final : public RendererBackend
	{
	public:
		RendererVulkan();

		RendererVulkan(const RendererVulkan& other) = delete;
		RendererVulkan(RendererVulkan&& other) = delete;

		RendererVulkan& operator=(const RendererVulkan& other) = delete;
		RendererVulkan& operator=(RendererVulkan&& other) = delete;

		~RendererVulkan() override = default;

		bool Init(const RendererBackendConfig& config, u8* outWindowRenderTargetCount) override;
		void Shutdown() override;

		void OnResize(u16 width, u16 height) override;

		bool BeginFrame(f32 deltaTime) override;
		bool EndFrame(f32 deltaTime) override;

		void SetViewport(const vec4& rect) override;
		void ResetViewport() override;
		void SetScissor(const ivec4& rect) override;
		void ResetScissor() override;

		bool BeginRenderPass(RenderPass* pass, RenderTarget* target) override;
		bool EndRenderPass(RenderPass* pass) override;

		void DrawGeometry(const GeometryRenderData& data) override;

		void CreateTexture(const u8* pixels, Texture* texture) override;
		void CreateWritableTexture(Texture* texture) override;

		void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels) override;
		void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) override;

		void ReadDataFromTexture(Texture* texture, u32 offset, u32 size, void** outMemory) override;
		void ReadPixelFromTexture(Texture* texture, u32 x, u32 y, u8** outRgba) override;

		void DestroyTexture(Texture* texture) override;

		bool CreateGeometry(Geometry* geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount, const void* indices) override;
		void DestroyGeometry(Geometry* geometry) override;

		bool CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const override;
		void DestroyShader(Shader* shader) override;

		bool InitializeShader(Shader* shader) override;
		bool UseShader(Shader* shader) override;

		bool ShaderBindGlobals(Shader* shader) override;
		bool ShaderBindInstance(Shader* shader, u32 instanceId) override;

		bool ShaderApplyGlobals(Shader* shader) override;
		bool ShaderApplyInstance(Shader* shader, bool needsUpdate) override;

		bool AcquireShaderInstanceResources(Shader* shader, TextureMap** maps, u32* outInstanceId) override;
		bool ReleaseShaderInstanceResources(Shader* shader, u32 instanceId) override;

		bool AcquireTextureMapResources(TextureMap* map) override;
		void ReleaseTextureMapResources(TextureMap* map) override;

		bool SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value) override;

		void CreateRenderTarget(u8 attachmentCount, RenderTargetAttachment* attachments, RenderPass* pass, u32 width, u32 height, RenderTarget* outTarget) override;
		void DestroyRenderTarget(RenderTarget* target, bool freeInternalMemory) override;

		RenderPass* CreateRenderPass(const RenderPassConfig& config) override;
		bool DestroyRenderPass(RenderPass* pass) override;

		RenderBuffer* CreateRenderBuffer(RenderBufferType bufferType, u64 totalSize, bool useFreelist) override;
		bool DestroyRenderBuffer(RenderBuffer* buffer) override;

		Texture* GetWindowAttachment(u8 index) override;
		Texture* GetDepthAttachment(u8 index) override;

		u8 GetWindowAttachmentIndex() override;
		u8 GetWindowAttachmentCount() override;

		[[nodiscard]] bool IsMultiThreaded() const override;
	private:
		void CreateCommandBuffers();

		bool RecreateSwapChain();

		bool CreateModule(VulkanShaderStageConfig config, VulkanShaderStage* shaderStage) const;

		VkSamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat) const;
		VkFilter ConvertFilterType(const char* op, TextureFilter filter) const;

		VulkanContext m_context;
		VulkanBuffer m_objectVertexBuffer, m_objectIndexBuffer;

		// TODO: make dynamic
		VulkanGeometryData m_geometries[VULKAN_MAX_GEOMETRY_COUNT];

#ifdef _DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
#endif
	};
}