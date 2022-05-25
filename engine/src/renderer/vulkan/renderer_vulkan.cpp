
#include "renderer_vulkan.h"
#include "VkBootstrap.h"

#include <SDL2/SDL_vulkan.h>

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"

#include "core/logger.h"

namespace C3D
{
	bool RendererVulkan::Init(const string& applicationName, SDL_Window* window)
	{
		Logger::PushPrefix("VULKAN_RENDERER");

		type = RendererBackendType::Vulkan;

		// TODO: Possibly add a custom allocator
		m_context.allocator = nullptr;

		vkb::InstanceBuilder instanceBuilder;

		auto vkbInstanceResult = instanceBuilder
			.set_app_name("C3DEngine")
		#if defined(_DEBUG)
			.request_validation_layers(true)
			.set_debug_callback(Logger::VkDebugLog)
		#endif
			.require_api_version(1, 2)
			.set_allocation_callbacks(m_context.allocator)
			.build();

		Logger::Info("Instance Initialized");
		const vkb::Instance vkbInstance = vkbInstanceResult.value();

		m_context.instance = vkbInstance.instance;
		m_debugMessenger = vkbInstance.debug_messenger;

		if (!SDL_Vulkan_CreateSurface(window, m_context.instance, &m_context.surface))
		{
			Logger::Error("Failed to create Vulkan Surface");
			return false;
		}

		Logger::Info("SDL Surface Initialized");

		if (!VulkanDeviceManager::Create(vkbInstance, &m_context))
		{
			Logger::Error("Failed to create Vulkan Device");
			return false;
		}

		VulkanSwapChainManager::Create(&m_context, m_context.frameBufferWidth, m_context.frameBufferHeight, &m_context.swapChain);

		VulkanRenderPassManager::Create(&m_context, &m_context.mainRenderPass, 0, 0, m_context.frameBufferWidth, 
			m_context.frameBufferHeight, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);

		CreateCommandBuffers();
		Logger::Info("Command Buffers Initialized");

		Logger::Info("Successfully Initialized");
		Logger::PopPrefix();
		return true;
	}

	void RendererVulkan::Shutdown()
	{
		Logger::PrefixInfo("VULKAN_RENDERER", "Shutting Down");

		vkDestroyCommandPool(m_context.device.logicalDevice, m_context.device.graphicsCommandPool, m_context.allocator);

		VulkanRenderPassManager::Destroy(&m_context, &m_context.mainRenderPass);

		VulkanSwapChainManager::Destroy(&m_context, &m_context.swapChain);

		VulkanDeviceManager::Destroy(&m_context);

		vkDestroySurfaceKHR(m_context.instance, m_context.surface, m_context.allocator);

		vkb::destroy_debug_utils_messenger(m_context.instance, m_debugMessenger);

		vkDestroyInstance(m_context.instance, m_context.allocator);
	}

	void RendererVulkan::CreateCommandBuffers()
	{
		if (m_context.graphicsCommandBuffers.empty())
		{
			m_context.graphicsCommandBuffers.resize(m_context.swapChain.imageCount);
		}

		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			if (m_context.graphicsCommandBuffers[i].handle)
			{
				VulkanCommandBufferManager::Free(&m_context, m_context.device.graphicsCommandPool, &m_context.graphicsCommandBuffers[i]);
			}

			VulkanCommandBufferManager::Allocate(&m_context, m_context.device.graphicsCommandPool, true, &m_context.graphicsCommandBuffers[i]);
		}
	}
}
