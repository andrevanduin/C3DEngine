
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

		virtual ~RendererVulkan() = default;

		bool Init(const RendererBackendConfig& config, u8* outWindowRenderTargetCount) override;
		void Shutdown() override;

		void OnResize(u16 width, u16 height) override;

		bool BeginFrame(f32 deltaTime) override;
		bool EndFrame(f32 deltaTime) override;

		bool BeginRenderPass(RenderPass* pass, RenderTarget* target) override;
		bool EndRenderPass(RenderPass* pass) override;
		RenderPass* GetRenderPass(const char* name) override;

		void DrawGeometry(const GeometryRenderData& data) override;

		void CreateTexture(const u8* pixels, Texture* texture) override;
		void CreateWritableTexture(Texture* texture) override;

		void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels) override;
		void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) override;

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

		void CreateRenderTarget(u8 attachmentCount, Texture** attachments, RenderPass* pass, u32 width, u32 height, RenderTarget* outTarget) override;
		void DestroyRenderTarget(RenderTarget* target, bool freeInternalMemory) override;

		Texture* GetWindowAttachment(u8 index) override;
		Texture* GetDepthAttachment() override;

		u8 GetWindowAttachmentIndex() override;

		[[nodiscard]] bool IsMultiThreaded() const override;
	private:
		void CreateCommandBuffers();

		bool RecreateSwapChain();
		bool CreateBuffers();

		bool CreateModule(VulkanShaderStageConfig config, VulkanShaderStage* shaderStage) const;

		// TEMP
		bool UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer* buffer, u64* outOffset, u64 size, const void* data) const;
		static void FreeDataRange(VulkanBuffer* buffer, u64 offset, u64 size);

		VkSamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat) const;
		VkFilter ConvertFilterType(const char* op, TextureFilter filter) const;

		VulkanContext m_context;
		VulkanBuffer m_objectVertexBuffer, m_objectIndexBuffer;

		// TODO: make dynamic
		VulkanGeometryData m_geometries[VULKAN_MAX_GEOMETRY_COUNT];

#if defined(_DEBUG)
		VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
#endif
	};
}