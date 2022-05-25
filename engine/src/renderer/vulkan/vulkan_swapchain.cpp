
#include "vulkan_swapchain.h"

#include "../../core/logger.h"
#include "../../core/memory.h"

#include "vulkan_device.h"
#include "vulkan_image.h"

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

		Logger::Warn("Could not find Preferred SwapChain ImageFormat. Falling back to first format in the list");
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

	void CreateInternal(VulkanContext* context, const u32 width, const u32 height, VulkanSwapChain* swapChain)
	{
		Logger::PushPrefix("VULAN_SWAPCHAIN_MANAGER");

		VkExtent2D extent = { width, height };
		swapChain->maxFramesInFlight = 2;
		swapChain->imageFormat = GetSurfaceFormat(context);
		swapChain->presentMode = GetPresentMode(context);

		// Query SwapChain support again to see if anything changed since last time (for example different resolution or monitor)
		VulkanDeviceManager::QuerySwapChainSupport(context->device.physicalDevice, context->surface, &context->device.swapChainSupport);

		if (context->device.swapChainSupport.capabilities.currentExtent.width != UINT32_MAX)
		{
			extent = context->device.swapChainSupport.capabilities.currentExtent;
		}

		const VkExtent2D min = context->device.swapChainSupport.capabilities.minImageExtent;
		const VkExtent2D max = context->device.swapChainSupport.capabilities.maxImageExtent;

		extent.width = C3D_CLAMP(extent.width, min.width, max.width);
		extent.height = C3D_CLAMP(extent.height, min.height, max.height);

		u32 imageCount = context->device.swapChainSupport.capabilities.minImageCount + 1;
		if (context->device.swapChainSupport.capabilities.maxImageCount > 0 && imageCount > context->device.swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = context->device.swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapChainCreateInfo.surface = context->surface;
		swapChainCreateInfo.minImageCount = imageCount;
		swapChainCreateInfo.imageFormat = swapChain->imageFormat.format;
		swapChainCreateInfo.imageColorSpace = swapChain->imageFormat.colorSpace;
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
		swapChainCreateInfo.presentMode = swapChain->presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = nullptr; // TODO: pass the old SwapChain here for better performance

		VK_CHECK(vkCreateSwapchainKHR(context->device.logicalDevice, &swapChainCreateInfo, context->allocator, &swapChain->handle));

		context->currentFrame = 0;

		swapChain->imageCount = 0;
		VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevice, swapChain->handle, &swapChain->imageCount, nullptr));
		if (!swapChain->images)
		{
			swapChain->images = Memory::Allocate<VkImage>(swapChain->imageCount, MemoryType::Renderer);
		}
		if (!swapChain->views)
		{
			swapChain->views = Memory::Allocate<VkImageView>(swapChain->imageCount, MemoryType::Renderer);
		}
		VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevice, swapChain->handle, &swapChain->imageCount, swapChain->images));

		for (u32 i = 0; i < swapChain->imageCount; i++)
		{
			VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			viewInfo.image = swapChain->images[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChain->imageFormat.format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK(vkCreateImageView(context->device.logicalDevice, &viewInfo, context->allocator, &swapChain->views[i]));
		}

		if (!VulkanDeviceManager::DetectDepthFormat(&context->device))
		{
			context->device.depthFormat = VK_FORMAT_UNDEFINED;
			Logger::Fatal("Failed to find a supported Depth Format");
		}

		VulkanImageManager::CreateImage(context, VK_IMAGE_TYPE_2D, extent.width, extent.height, context->device.depthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true, VK_IMAGE_ASPECT_DEPTH_BIT, &swapChain->depthAttachment);

		Logger::Info("SwapChain successfully created");
		Logger::PopPrefix();
	}

	void DestroyInternal(const VulkanContext* context, VulkanSwapChain* swapChain)
	{
		VulkanImageManager::Destroy(context, &swapChain->depthAttachment);

		for (u32 i = 0; i < swapChain->imageCount; i++)
		{
			vkDestroyImageView(context->device.logicalDevice, swapChain->views[i], context->allocator);
		}

		vkDestroySwapchainKHR(context->device.logicalDevice, swapChain->handle, context->allocator);
	}

	void VulkanSwapChainManager::Create(VulkanContext* context, const u32 width, const u32 height, VulkanSwapChain* outSwapChain)
	{
		CreateInternal(context, width, height, outSwapChain);
	}

	void VulkanSwapChainManager::Recreate(VulkanContext* context, const u32 width, const u32 height, VulkanSwapChain* swapChain)
	{
		DestroyInternal(context, swapChain);
		CreateInternal(context, width, height, swapChain);
	}

	void VulkanSwapChainManager::Destroy(const VulkanContext* context, VulkanSwapChain* swapChain)
	{
		Logger::PrefixInfo("VULKAN_SWAPCHAIN", "Destroying SwapChain");
		DestroyInternal(context, swapChain);
	}

	bool VulkanSwapChainManager::AcquireNextImageIndex(VulkanContext* context, VulkanSwapChain* swapChain,
	    const u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex)
	{
		const auto result = vkAcquireNextImageKHR(context->device.logicalDevice, swapChain->handle, timeoutNs,
			imageAvailableSemaphore, fence, outImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Recreate(context, context->frameBufferWidth, context->frameBufferHeight, swapChain);
			return false;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			Logger::Fatal("Failed to acquire SwapChain image");
			return false;
		}
		return true;
	}

	void VulkanSwapChainManager::Present(VulkanContext* context, VulkanSwapChain* swapChain, VkQueue graphicsQueue,
		VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImageIndex)
	{
		// Return the image to the SwapChain for presentation
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain->handle;
		presentInfo.pImageIndices = &presentImageIndex;
		presentInfo.pResults = nullptr;

		const auto result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			// Our SwapChain is out of date, suboptimal or a FrameBuffer resize has occurred.
			// We trigger a SwapChain recreation.
			Recreate(context, context->frameBufferWidth, context->frameBufferHeight, swapChain);
		}
		else if (result != VK_SUCCESS)
		{
			Logger::Fatal("Failed to present SwapChain image");
		}
	}
}
