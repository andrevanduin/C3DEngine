
#pragma once
#include "vulkan_types.h"
#include "vulkan_buffer.h"
#include "vulkan_shader.h"

#include "../renderer_backend.h"

struct SDL_Window;

namespace C3D
{
	class RendererVulkan final : public RendererBackend
	{
	public:
		RendererVulkan();
		virtual ~RendererVulkan() = default;

		bool Init(Application* application) override;
		void Shutdown() override;

		void OnResize(u16 width, u16 height) override;

		bool BeginFrame(f32 deltaTime) override;
		bool EndFrame(f32 deltaTime) override;

		bool BeginRenderPass(u8 renderPassId) override;
		bool EndRenderPass(u8 renderPassId) override;

		void DrawGeometry(GeometryRenderData data) override;

		void CreateTexture(const u8* pixels, Texture* texture) override;
		void DestroyTexture(Texture* texture) override;

		bool CreateGeometry(Geometry* geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount, const void* indices) override;
		void DestroyGeometry(Geometry* geometry) override;

		bool CreateShader(Shader* shader, u8 renderPassId, const std::vector<char*>& stageFileNames, const std::vector<ShaderStage>& stages) override;
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
		
	private:
		void CreateCommandBuffers();
		void RegenerateFrameBuffers();
		
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

		VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
	};
}