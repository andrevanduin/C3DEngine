
#pragma once
#include "../renderer_backend.h"
#include "vulkan_types.h"

struct SDL_Window;

namespace C3D
{
	class RendererVulkan : public RendererBackend
	{
	public:
		bool Init(Application* application) override;

		void OnResize(u16 width, u16 height) override;

		bool BeginFrame(f32 deltaTime) override;
		bool EndFrame(f32 deltaTime) override;

		void Shutdown() override;

	private:
		void CreateCommandBuffers();
		void RegenerateFrameBuffers(const VulkanSwapChain* swapChain, VulkanRenderPass* renderPass);

		bool RecreateSwapChain();

		VulkanContext m_context;
		VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
	};
}