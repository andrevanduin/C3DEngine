
#pragma once
#include "../renderer_backend.h"
#include "vulkan_types.h"

struct SDL_Window;

namespace C3D
{
	class RendererVulkan : public RendererBackend
	{
	public:
		bool Init(const string& applicationName, SDL_Window* window) override;

		void Shutdown() override;

	private:
		void CreateCommandBuffers();

		VulkanContext m_context;
		VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
	};
}