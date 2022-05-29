
#pragma once
#include "vulkan_types.h"
#include "vulkan_buffer.h"

#include "resources/texture.h"

#include "../renderer_backend.h"
#include "shaders/vulkan_object_shader.h"

struct SDL_Window;

namespace C3D
{
	class RendererVulkan final : public RendererBackend
	{
	public:
		RendererVulkan();
		virtual ~RendererVulkan() = default;

		bool Init(Application* application, Texture* defaultDiffuse) override;

		void OnResize(u16 width, u16 height) override;

		bool BeginFrame(f32 deltaTime) override;

		void UpdateGlobalState(mat4 projection, mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode) override;

		void UpdateObject(GeometryRenderData data) override;

		bool EndFrame(f32 deltaTime) override;

		void Shutdown() override;

		void CreateTexture(const string& name, bool autoRelease, i32 width, i32 height, i32 channelCount, const u8* pixels, bool hasTransparency, Texture* outTexture) override;

		void DestroyTexture(Texture* texture) override;

	private:
		void CreateCommandBuffers();
		void RegenerateFrameBuffers(const VulkanSwapChain* swapChain, VulkanRenderPass* renderPass);
		
		bool RecreateSwapChain();
		bool CreateBuffers();

		// TEMP
		void UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, const VulkanBuffer* buffer, u64 offset, u64 size, const void* data);

		VulkanContext m_context;

		VulkanObjectShader m_objectShader;

		VulkanBuffer m_objectVertexBuffer, m_objectIndexBuffer;

		u64 m_geometryVertexOffset;
		u64 m_geometryIndexOffset;

		VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
	};
}