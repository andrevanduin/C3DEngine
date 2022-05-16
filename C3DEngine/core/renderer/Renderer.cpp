
#include "Renderer.h"
#include "../VkInitializers.h"
#include "../Logger.h"
#include "../Math.h"

#define VK_CHECK(x)															\
	do																		\
	{																		\
		VkResult err = x;													\
		if (err)															\
		{																	\
			Logger::Error("VK_CHECK failed: {}", err);						\
			abort();														\
		}																	\
	} while (0)

namespace C3D
{
	void Renderer::Init(Allocator* allocator, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		Logger::Info("Renderer::Init()");

		m_allocator = allocator;

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

		const VkExtent3D renderImageExtent = { m_windowExtent.width, m_windowExtent.height, 1 };

		// Create Image and View for m_rawRenderImage
		m_renderFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
		CreateImageAndView(m_renderFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			renderImageExtent, VK_IMAGE_ASPECT_COLOR_BIT, m_rawRenderImage);

		m_deletionQueue.Push([=] { vkDestroySwapchainKHR(m_device, m_swapChain, nullptr); });

		const VkExtent3D depthExtent = { m_windowExtent.width, m_windowExtent.height, 1 };
		const VkExtent3D shadowExtent = { m_shadowExtent.width, m_shadowExtent.height, 1 };

		m_depthFormat = VK_FORMAT_D32_SFLOAT;

		// Create Image and View for m_depthImage
		CreateImageAndView(m_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			depthExtent, VK_IMAGE_ASPECT_DEPTH_BIT, m_depthImage);

		// Create Image and View for m_shadowImage
		CreateImageAndView(m_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			shadowExtent, VK_IMAGE_ASPECT_DEPTH_BIT, m_shadowImage);

		m_depthPyramid.width = Math::PreviousPow2(m_windowExtent.width);
		m_depthPyramid.height = Math::PreviousPow2(m_windowExtent.height);
		m_depthPyramid.levels = GetImageMipLevels(m_depthPyramid.width, m_depthPyramid.height);

		const VkExtent3D pyramidExtent = { m_depthPyramid.width, m_depthPyramid.height, 1 };

		auto pyramidImageCreateInfo = VkInit::ImageCreateInfo(VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | 
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT, pyramidExtent);
		pyramidImageCreateInfo.mipLevels = m_depthPyramid.levels;

		m_allocator->CreateImage(pyramidImageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthPyramid.image);

		auto pyramidImageViewCreateInfo = VkInit::ImageViewCreateInfo(VK_FORMAT_R32_SFLOAT, m_depthPyramid.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
		pyramidImageViewCreateInfo.subresourceRange.levelCount = m_depthPyramid.levels;

		VK_CHECK(vkCreateImageView(m_device, &pyramidImageViewCreateInfo, nullptr, &m_depthPyramid.image.defaultView));

		for (uint32_t i = 0; i < m_depthPyramid.levels; i++)
		{
			auto levelInfo = VkInit::ImageViewCreateInfo(VK_FORMAT_R32_SFLOAT, m_depthPyramid.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
			levelInfo.subresourceRange.levelCount = 1;
			levelInfo.subresourceRange.baseMipLevel = i;

			VkImageView pyramid;
			vkCreateImageView(m_device, &levelInfo, nullptr, &pyramid);

			m_depthPyramid.mips[i] = pyramid;
		}

		auto depthSamplerCreateInfo = VkInit::SamplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		depthSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		depthSamplerCreateInfo.minLod = 0;
		depthSamplerCreateInfo.maxLod = 16.0f;

		VkSamplerReductionModeCreateInfoEXT reductionModeCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };
		reductionModeCreateInfo.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
		depthSamplerCreateInfo.pNext = &reductionModeCreateInfo;

		VK_CHECK(vkCreateSampler(m_device, &depthSamplerCreateInfo, nullptr, &m_depthSampler));

		auto smoothSamplerCreateInfo = VkInit::SamplerCreateInfo(VK_FILTER_LINEAR);
		smoothSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		VK_CHECK(vkCreateSampler(m_device, &smoothSamplerCreateInfo, nullptr, &m_smoothSampler));

		auto shadowSamplerCreateInfo = VkInit::SamplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
		shadowSamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		shadowSamplerCreateInfo.compareEnable = VK_TRUE;
		shadowSamplerCreateInfo.compareOp = VK_COMPARE_OP_LESS;

		VK_CHECK(vkCreateSampler(m_device, &shadowSamplerCreateInfo, nullptr, &m_shadowSampler));

		m_deletionQueue.Push([=]
			{
				vkDestroyImageView(m_device, m_depthImage.defaultView, nullptr);
				m_allocator->DestroyImage(m_depthImage);
			}
		);
	}

	void Renderer::InitFrameBuffers()
	{
		Logger::Info("Renderer::InitFrameBuffers()");

		const auto swapChainImageCount = static_cast<uint32_t>(m_swapChainImages.size());
		m_frameBuffers = std::vector<VkFramebuffer>(swapChainImageCount);

		auto frameBufferCreateInfo = VkInit::FrameBufferCreateInfo(m_renderPass, m_windowExtent);

		VkImageView attachments[2];
		attachments[0] = m_rawRenderImage.defaultView;
		attachments[1] = m_depthImage.defaultView;

		frameBufferCreateInfo.pAttachments = attachments;
		frameBufferCreateInfo.attachmentCount = 2;

		VK_CHECK(vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr, &m_forwardFrameBuffer));

		auto shadowFrameBufferCreateInfo = VkInit::FrameBufferCreateInfo(m_shadowPass, m_shadowExtent);
		shadowFrameBufferCreateInfo.pAttachments = &m_shadowImage.defaultView;
		shadowFrameBufferCreateInfo.attachmentCount = 1;

		VK_CHECK(vkCreateFramebuffer(m_device, &shadowFrameBufferCreateInfo, nullptr, &m_shadowFrameBuffer));

		for (uint32_t i = 0; i < swapChainImageCount; i++)
		{
			auto createInfo = VkInit::FrameBufferCreateInfo(m_copyPass, m_windowExtent);
			createInfo.pAttachments = &m_swapChainImageViews[i];
			createInfo.attachmentCount = 1;

			VK_CHECK(vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_frameBuffers[i]));

			 m_deletionQueue.Push([=]
				{
					vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
					vkDestroyImageView(m_device, m_swapChainImageViews[i], nullptr);
				}
			);
		}
	}

	void Renderer::CreateImageAndView(const VkFormat format, const VkImageUsageFlags imageUsageFlags, const VkExtent3D extent, 
	                                  const VkImageAspectFlags aspectFlags, AllocatedImage& outImage) const
	{
		const auto imageCreateInfo = VkInit::ImageCreateInfo(format, imageUsageFlags, extent);
		m_allocator->CreateImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outImage);

		const auto imageViewCreateInfo = VkInit::ImageViewCreateInfo(format, outImage.image, aspectFlags);
		VK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &outImage.defaultView));
	}

	uint32_t Renderer::GetImageMipLevels(uint32_t width, uint32_t height)
	{
		uint32_t result = 1;
		while (width > 1 || height > 1)
		{
			result++;
			width /= 2;
			height /= 2;
		}
		return result;
	}
}
