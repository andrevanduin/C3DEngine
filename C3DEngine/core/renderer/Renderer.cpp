
#include "Renderer.h"
#include "../VkInitializers.h"
#include "../Logger.h"

namespace C3D
{
	void Renderer::Init(VkDevice device, const VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		Logger::Info("Renderer::Init()");

		m_device = device;
		m_physicalDevice = physicalDevice;
		m_surface = surface;

		m_scene.Init();

		InitSwapChain();

		InitForwardRenderPass();
		InitCopyRenderPass();
		InitShadowRenderPass();

		InitFrameBuffers();
		InitCommands();
		InitSyncStructures();
		InitDescriptors();
		InitPipelines();
	}

	void Renderer::InitSwapChain()
	{
		Logger::Info("Renderer::InitSwapChain()");

		vkb::SwapchainBuilder swapChainBuilder{ m_physicalDevice, m_device, m_surface };

		vkb::Swapchain vkbSwapChain = swapChainBuilder
			.use_default_format_selection()
			.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
			.set_desired_extent(m_windowExtent.width, m_windowExtent.height)
			.build()
			.value();

		m_swapChain = vkbSwapChain.swapchain;
		m_swapChainImages = vkbSwapChain.get_images().value();
		m_swapChainImageViews = vkbSwapChain.get_image_views().value();

		m_swapchainImageFormat = vkbSwapChain.image_format;

		VkExtent3D renderImageExtent = { m_windowExtent.width, m_windowExtent.height, 1 };

		m_renderFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

		auto imageCreateInfo = VkInit::ImageCreateInfo(m_renderFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
			VK_IMAGE_USAGE_SAMPLED_BIT, renderImageExtent);

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vmaCreateImage()
	}
}
