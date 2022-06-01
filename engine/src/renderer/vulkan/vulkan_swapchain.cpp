
#include "vulkan_swapchain.h"

#include "vulkan_device.h"
#include "vulkan_image.h"
#include "vulkan_types.h"

#include "core/memory.h"
#include "core/logger.h"

namespace C3D
{
	VkSurfaceFormatKHR GetSurfaceFormat(const VulkanContext* context)
	{
		for (u32 i = 0; i < context->device.swapChainSupport.formatCount; i++)
		{
			const VkSurfaceFormatKHR format = context->device.swapChainSupport.formats[i];
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		Logger::Warn("[VULKAN_SWAP_CHAIN] - Could not find Preferred SwapChain ImageFormat. Falling back to first format in the list");
		return context->device.swapChainSupport.formats[0];
	}

	VkPresentModeKHR GetPresentMode(const VulkanContext* context)
	{
		for (u32 i = 0; i < context->device.swapChainSupport.presentModeCount; i++)
		{
			const VkPresentModeKHR mode = context->device.swapChainSupport.presentModes[i];
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return mode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VulkanSwapChain::VulkanSwapChain()
		: handle(nullptr), imageFormat(), imageCount(0), maxFramesInFlight(0), views(nullptr), m_presentMode(), m_images(nullptr)
	{
	}

	void VulkanSwapChain::Create(VulkanContext* context, const u32 width, const u32 height)
	{
		CreateInternal(context, width, height);
	}

	void VulkanSwapChain::Recreate(VulkanContext* context, const u32 width, const u32 height)
	{
		DestroyInternal(context);
		CreateInternal(context, width, height);
	}

	void VulkanSwapChain::Destroy(const VulkanContext* context)
	{
		Logger::Info("[VULKAN_SWAP_CHAIN] - Destroying SwapChain");
		DestroyInternal(context);
	}

	bool VulkanSwapChain::AcquireNextImageIndex(VulkanContext* context, const u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex)
	{
		const auto result = vkAcquireNextImageKHR(context->device.logicalDevice, handle, timeoutNs, imageAvailableSemaphore, fence, outImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Recreate(context, context->frameBufferWidth, context->frameBufferHeight);
			return false;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			Logger::Fatal("[VULKAN_SWAP_CHAIN] - Failed to acquire SwapChain image");
			return false;
		}
		return true;
	}

	void VulkanSwapChain::Present(VulkanContext* context, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, const u32 presentImageIndex)
	{
		// Return the image to the SwapChain for presentation
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &handle;
		presentInfo.pImageIndices = &presentImageIndex;
		presentInfo.pResults = nullptr;

		const auto result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			// Our SwapChain is out of date, suboptimal or a FrameBuffer resize has occurred.
			// We trigger a SwapChain recreation.
			Recreate(context, context->frameBufferWidth, context->frameBufferHeight);
		}
		else if (result != VK_SUCCESS)
		{
			Logger::Fatal("[VULKAN_SWAP_CHAIN] - Failed to present SwapChain image");
		}

		context->currentFrame = (context->currentFrame + 1) % maxFramesInFlight;
	}

	void VulkanSwapChain::CreateInternal(VulkanContext* context, const u32 width, const u32 height)
	{
		VkExtent2D extent = { width, height };
		imageFormat = GetSurfaceFormat(context);
		m_presentMode = GetPresentMode(context);

		// Query SwapChain support again to see if anything changed since last time (for example different resolution or monitor)
		context->device.QuerySwapChainSupport(context->surface, &context->device.swapChainSupport);

		if (context->device.swapChainSupport.capabilities.currentExtent.width != UINT32_MAX)
		{
			extent = context->device.swapChainSupport.capabilities.currentExtent;
		}

		const VkExtent2D min = context->device.swapChainSupport.capabilities.minImageExtent;
		const VkExtent2D max = context->device.swapChainSupport.capabilities.maxImageExtent;

		extent.width = C3D_CLAMP(extent.width, min.width, max.width);
		extent.height = C3D_CLAMP(extent.height, min.height, max.height);

		u32 imgCount = context->device.swapChainSupport.capabilities.minImageCount + 1;
		if (context->device.swapChainSupport.capabilities.maxImageCount > 0 && imgCount > context->device.swapChainSupport.capabilities.maxImageCount)
		{
			imgCount = context->device.swapChainSupport.capabilities.maxImageCount;
		}

		maxFramesInFlight = static_cast<u8>(imgCount) - 1;

		VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapChainCreateInfo.surface = context->surface;
		swapChainCreateInfo.minImageCount = imgCount;
		swapChainCreateInfo.imageFormat = imageFormat.format;
		swapChainCreateInfo.imageColorSpace = imageFormat.colorSpace;
		swapChainCreateInfo.imageExtent = extent;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if (context->device.graphicsQueueIndex != context->device.presentQueueIndex)
		{
			const u32 queueFamilyIndices[] = { context->device.graphicsQueueIndex, context->device.presentQueueIndex };

			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainCreateInfo.queueFamilyIndexCount = 2;
			swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCreateInfo.queueFamilyIndexCount = 0;
			swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		swapChainCreateInfo.preTransform = context->device.swapChainSupport.capabilities.currentTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = m_presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = nullptr; // TODO: pass the old SwapChain here for better performance

		VK_CHECK(vkCreateSwapchainKHR(context->device.logicalDevice, &swapChainCreateInfo, context->allocator, &handle));

		context->currentFrame = 0;

		imageCount = 0;
		VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevice, handle, &imageCount, nullptr));
		if (!m_images)
		{
			m_images = Memory::Allocate<VkImage>(imageCount, MemoryType::Renderer);
		}
		if (!views)
		{
			views = Memory::Allocate<VkImageView>(imageCount, MemoryType::Renderer);
		}
		VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevice, handle, &imageCount, m_images));

		for (u32 i = 0; i < imageCount; i++)
		{
			VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			viewInfo.image = m_images[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = imageFormat.format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK(vkCreateImageView(context->device.logicalDevice, &viewInfo, context->allocator, &views[i]));
		}

		if (!context->device.DetectDepthFormat())
		{
			context->device.depthFormat = VK_FORMAT_UNDEFINED;
			Logger::Fatal("[VULKAN_SWAP_CHAIN] - Failed to find a supported Depth Format");
		}

		depthAttachment.Create(context, VK_IMAGE_TYPE_2D, extent.width, extent.height, context->device.depthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true, VK_IMAGE_ASPECT_DEPTH_BIT);

		Logger::Info("[VULKAN_SWAP_CHAIN] - Successfully created");
	}

	void VulkanSwapChain::DestroyInternal(const VulkanContext* context)
	{
		vkDeviceWaitIdle(context->device.logicalDevice);
		depthAttachment.Destroy(context);

		for (u32 i = 0; i < imageCount; i++)
		{
			vkDestroyImageView(context->device.logicalDevice, views[i], context->allocator);
		}

		vkDestroySwapchainKHR(context->device.logicalDevice, handle, context->allocator);
	}
}
