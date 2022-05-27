
#include "renderer_vulkan.h"

#include <VkBootstrap.h>
#include <SDL2/SDL_vulkan.h>

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_fence.h"
#include "vulkan_framebuffer.h"
#include "vulkan_utils.h"

#include "core/logger.h"
#include "core/application.h"
#include "core/memory.h"

#include "renderer/vertex.h"

namespace C3D
{
	bool RendererVulkan::Init(Application* application)
	{
		Logger::PushPrefix("VULKAN_RENDERER");

		type = RendererBackendType::Vulkan;

		// TODO: Possibly add a custom allocator
		m_context.allocator = nullptr;

		application->GetFrameBufferSize(&m_context.cachedFrameBufferWidth, &m_context.cachedFrameBufferHeight);
		m_context.frameBufferWidth = (m_context.cachedFrameBufferWidth != 0) ? m_context.cachedFrameBufferWidth : 1280;
		m_context.frameBufferHeight = (m_context.cachedFrameBufferHeight != 0) ? m_context.cachedFrameBufferHeight : 720;

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

		if (!SDL_Vulkan_CreateSurface(application->GetWindow(), m_context.instance, &m_context.surface))
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

		VulkanRenderPassManager::Create(&m_context, &m_context.mainRenderPass, 0, 0, static_cast<f32>(m_context.frameBufferWidth), 
			static_cast<f32>(m_context.frameBufferHeight), 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);

		m_context.swapChain.frameBuffers.resize(m_context.swapChain.imageCount);
		RegenerateFrameBuffers(&m_context.swapChain, &m_context.mainRenderPass);

		CreateCommandBuffers();
		Logger::Info("Command Buffers Initialized");

		m_context.imageAvailableSemaphores.resize(m_context.swapChain.maxFramesInFlight);
		m_context.queueCompleteSemaphores.resize(m_context.swapChain.maxFramesInFlight);
		m_context.inFlightFences.resize(m_context.swapChain.maxFramesInFlight);

		Logger::Info("Creating Semaphores and Fences");
		for (u8 i = 0; i < m_context.swapChain.maxFramesInFlight; i++)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			vkCreateSemaphore(m_context.device.logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.imageAvailableSemaphores[i]);
			vkCreateSemaphore(m_context.device.logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.queueCompleteSemaphores[i]);

			VulkanFenceManager::Create(&m_context, true, &m_context.inFlightFences[i]);
		}

		m_context.imagesInFlight.resize(m_context.swapChain.imageCount);
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			m_context.imagesInFlight[i] = nullptr;
		}

		if (!m_objectShader.Create(&m_context))
		{
			Logger::Error("Loading built-in object shader failed");
			return false;
		}

		CreateBuffers();

		// TODO: Temporary test code
		constexpr u32 vertexCount = 4;
		Vertex3D vertices[vertexCount];
		Memory::Zero(vertices, sizeof(Vertex3D) * vertexCount);

		vertices[0].position.x = 0.0f;
		vertices[0].position.y = -0.5f;

		vertices[1].position.x = 0.5f;
		vertices[1].position.y = 0.5f;

		vertices[2].position.x = 0.0f;
		vertices[2].position.y = 0.5f;

		vertices[3].position.x = 0.5f;
		vertices[3].position.y = -0.5f;

		constexpr u32 indexCount = 6;
		u32 indices[indexCount] = { 0, 1, 2, 0, 3, 1 };

		UploadDataRange(m_context.device.graphicsCommandPool, nullptr, m_context.device.graphicsQueue, &m_objectVertexBuffer, 0, sizeof(Vertex3D) * vertexCount, vertices);
		UploadDataRange(m_context.device.graphicsCommandPool, nullptr, m_context.device.graphicsQueue, &m_objectIndexBuffer, 0, sizeof(u32) * indexCount, indices);
		// TODO End of temporary test code

		Logger::Info("Successfully Initialized");
		Logger::PopPrefix();
		return true;
	}

	void RendererVulkan::OnResize(const u16 width, const u16 height)
	{
		m_context.cachedFrameBufferWidth = width;
		m_context.cachedFrameBufferHeight = height;
		m_context.frameBufferSizeGeneration++;

		Logger::PrefixInfo("VULKAN_RENDERER", "OnResize() w/h/gen {}/{}/{}", width, height, m_context.frameBufferSizeGeneration);
	}

	bool RendererVulkan::BeginFrame(f32 deltaTime)
	{
		const auto& device = m_context.device;
		Logger::PushPrefix("VULKAN_RENDERER");

		// If we are recreating the SwapChain we should stop this frame
		if (m_context.recreatingSwapChain)
		{
			const auto result = vkDeviceWaitIdle(device.logicalDevice);
			if (!VulkanUtils::ResultIsSuccess(result))
			{
				Logger::Error("vkDeviceWaitIdle (1) failed: {}", VulkanUtils::ResultString(result, true));
				return false;
			}
			Logger::Info("Recreating SwapChain. Stopping BeginFrame()");
			return false;
		}

		// If the FrameBuffer was resized we must also create a new SwapChain.
		if (m_context.frameBufferSizeGeneration != m_context.frameBufferSizeLastGeneration)
		{
			// FrameBuffer was resized. We need to recreate it.
			const auto result = vkDeviceWaitIdle(device.logicalDevice);
			if (!VulkanUtils::ResultIsSuccess(result))
			{
				Logger::Error("vkDeviceWaitIdle (2) failed: {}", VulkanUtils::ResultString(result, true));
				return false;
			}

			if (!RecreateSwapChain())
			{
				return false;
			}

			Logger::Info("SwapChain Resized successfully. Stopping BeginFrame()");
			return false;
		}

		// Wait for the execution of the current frame to complete.
		if (!VulkanFenceManager::Wait(&m_context, &m_context.inFlightFences[m_context.currentFrame], UINT64_MAX))
		{
			Logger::Warn("Waiting for In-Flight fences failed");
			return false;
		}

		// Acquire the next image from the SwapChain. Pass along the semaphore that should be signaled when this completes.
		// This same semaphore will later be waited on by the queue submission to ensure this image is available.
		if (!VulkanSwapChainManager::AcquireNextImageIndex(&m_context, &m_context.swapChain, UINT64_MAX, 
			m_context.imageAvailableSemaphores[m_context.currentFrame], nullptr, &m_context.imageIndex))
		{
			return false;
		}

		// We can begin recording commands
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
		VulkanCommandBufferManager::Reset(commandBuffer);
		VulkanCommandBufferManager::Begin(commandBuffer, false, false, false);

		// Dynamic state
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = static_cast<f32>(m_context.frameBufferHeight);
		viewport.width = static_cast<f32>(m_context.frameBufferWidth);
		viewport.height = -static_cast<f32>(m_context.frameBufferHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset.x = scissor.offset.y = 0;
		scissor.extent.width = m_context.frameBufferWidth;
		scissor.extent.height = m_context.frameBufferHeight;

		vkCmdSetViewport(commandBuffer->handle, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer->handle, 0, 1, &scissor);

		m_context.mainRenderPass.w = m_context.frameBufferWidth;
		m_context.mainRenderPass.h = m_context.frameBufferHeight;

		// Begin the RenderPass
		VulkanRenderPassManager::Begin(commandBuffer, &m_context.mainRenderPass, 
			m_context.swapChain.frameBuffers[m_context.imageIndex].handle);

		// TODO: Temporary test code
		m_objectShader.Use(&m_context);

		constexpr VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer->handle, 0, 1, &m_objectVertexBuffer.handle, offsets);

		vkCmdBindIndexBuffer(commandBuffer->handle, m_objectIndexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer->handle, 6, 1, 0, 0, 0);
		// TODO: End temporary test code

		return true;
	}

	bool RendererVulkan::EndFrame(f32 deltaTime)
	{
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		// End the RenderPass
		VulkanRenderPassManager::End(commandBuffer, &m_context.mainRenderPass);
		// End the CommandBuffer
		VulkanCommandBufferManager::End(commandBuffer);

		// Ensure that the previous frame is not using this image
		if (m_context.imagesInFlight[m_context.imageIndex] != VK_NULL_HANDLE)
		{
			VulkanFenceManager::Wait(&m_context, m_context.imagesInFlight[m_context.imageIndex], UINT64_MAX);
		}

		// Mark the image fence as in-use by this frame
		m_context.imagesInFlight[m_context.imageIndex] = &m_context.inFlightFences[m_context.currentFrame];

		// Reset the fence for use on the next frame
		VulkanFenceManager::Reset(&m_context, &m_context.inFlightFences[m_context.currentFrame]);

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer->handle;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_context.queueCompleteSemaphores[m_context.currentFrame];

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_context.imageAvailableSemaphores[m_context.currentFrame];

		VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.pWaitDstStageMask = flags;

		// Submit all the commands that we have queued
		const auto result = vkQueueSubmit(m_context.device.graphicsQueue, 1, &submitInfo, 
			m_context.inFlightFences[m_context.currentFrame].handle);

		if (result != VK_SUCCESS)
		{
			Logger::Error("vkQueueSubmit failed with result: {}", VulkanUtils::ResultString(result, true));
			return false;
		}

		// Queue submission is done
		VulkanCommandBufferManager::UpdateSubmitted(commandBuffer);

		// Present the image (and give it back to the SwapChain)
		VulkanSwapChainManager::Present(&m_context, &m_context.swapChain, m_context.device.graphicsQueue, 
			m_context.device.presentQueue, m_context.queueCompleteSemaphores[m_context.currentFrame], m_context.imageIndex);

		return true;
	}

	void RendererVulkan::Shutdown()
	{
		Logger::PushPrefix("VULKAN_RENDERER");
		Logger::Info("Shutting Down");

		vkDeviceWaitIdle(m_context.device.logicalDevice);

		// Destroy stuff in opposite order of creation
		m_objectVertexBuffer.Destroy(&m_context);
		m_objectIndexBuffer.Destroy(&m_context);

		m_objectShader.Destroy(&m_context);

		Logger::Info("Destroying Semaphores and Fences");
		for (u8 i = 0; i < m_context.swapChain.maxFramesInFlight; i++)
		{
			if (m_context.imageAvailableSemaphores[i])
			{
				vkDestroySemaphore(m_context.device.logicalDevice, m_context.imageAvailableSemaphores[i], m_context.allocator);
				m_context.imageAvailableSemaphores[i] = nullptr;
			}
			if (m_context.queueCompleteSemaphores[i])
			{
				vkDestroySemaphore(m_context.device.logicalDevice, m_context.queueCompleteSemaphores[i], m_context.allocator);
				m_context.queueCompleteSemaphores[i] = nullptr;
			}
			VulkanFenceManager::Destroy(&m_context, &m_context.inFlightFences[i]);
		}
		m_context.imageAvailableSemaphores.clear();
		m_context.queueCompleteSemaphores.clear();
		m_context.inFlightFences.clear();
		m_context.imagesInFlight.clear();

		vkDestroyCommandPool(m_context.device.logicalDevice, m_context.device.graphicsCommandPool, m_context.allocator);
		m_context.graphicsCommandBuffers.clear();

		Logger::Info("Destroying FrameBuffers");
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			VulkanFrameBufferManager::Destroy(&m_context, &m_context.swapChain.frameBuffers[i]);
		}

		VulkanRenderPassManager::Destroy(&m_context, &m_context.mainRenderPass);

		VulkanSwapChainManager::Destroy(&m_context, &m_context.swapChain);

		VulkanDeviceManager::Destroy(&m_context);

		vkDestroySurfaceKHR(m_context.instance, m_context.surface, m_context.allocator);

		vkb::destroy_debug_utils_messenger(m_context.instance, m_debugMessenger);

		vkDestroyInstance(m_context.instance, m_context.allocator);

		Logger::PopPrefix();
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

	void RendererVulkan::RegenerateFrameBuffers(const VulkanSwapChain* swapChain, VulkanRenderPass* renderPass)
	{
		for (u32 i = 0; i < swapChain->imageCount; i++)
		{
			// TODO: Make this dynamic based on the currently configured attachments
			constexpr u32 attachmentCount = 2;
			const VkImageView attachments[] = { swapChain->views[i], swapChain->depthAttachment.view };

			VulkanFrameBufferManager::Create(&m_context, renderPass, m_context.frameBufferWidth, m_context.frameBufferHeight, 
				attachmentCount, attachments, &m_context.swapChain.frameBuffers[i]);
		}
	}

	bool RendererVulkan::RecreateSwapChain()
	{
		if (m_context.recreatingSwapChain)
		{
			Logger::Debug("RecreateSwapChain called when already recreating.");
			return false;
		}

		if (m_context.frameBufferWidth == 0 || m_context.frameBufferHeight == 0)
		{
			Logger::Debug("RecreateSwapChain called when at least one of the window dimensions is < 1");
			return false;
		}

		m_context.recreatingSwapChain = true;

		// Ensure that our device is not busy
		vkDeviceWaitIdle(m_context.device.logicalDevice);

		// Clear out all the in-flight images since the size of the FrameBuffer will change
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			m_context.imagesInFlight[i] = nullptr;
		}

		// Re-query the SwapChain support and depth format since it might have changed
		VulkanDeviceManager::QuerySwapChainSupport(m_context.device.physicalDevice, m_context.surface, &m_context.device.swapChainSupport);
		VulkanDeviceManager::DetectDepthFormat(&m_context.device);

		VulkanSwapChainManager::Recreate(&m_context, m_context.cachedFrameBufferWidth, m_context.cachedFrameBufferHeight, &m_context.swapChain);

		// Sync the FrameBuffer size with the cached sizes
		m_context.frameBufferWidth = m_context.cachedFrameBufferWidth;
		m_context.frameBufferHeight = m_context.cachedFrameBufferHeight;
		m_context.mainRenderPass.w = m_context.frameBufferWidth;
		m_context.mainRenderPass.h = m_context.frameBufferHeight;
		m_context.cachedFrameBufferWidth = 0;
		m_context.cachedFrameBufferHeight = 0;

		// Update the size generation so that they are in sync again
		m_context.frameBufferSizeLastGeneration = m_context.frameBufferSizeGeneration;

		// Cleanup SwapChain
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			VulkanCommandBufferManager::Free(&m_context, m_context.device.graphicsCommandPool, &m_context.graphicsCommandBuffers[i]);
		}

		// Destroy FrameBuffers
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			VulkanFrameBufferManager::Destroy(&m_context, &m_context.swapChain.frameBuffers[i]);
		}

		m_context.mainRenderPass.x = 0;
		m_context.mainRenderPass.y = 0;
		m_context.mainRenderPass.w = m_context.frameBufferWidth;
		m_context.mainRenderPass.h = m_context.frameBufferHeight;

		RegenerateFrameBuffers(&m_context.swapChain, &m_context.mainRenderPass);
		CreateCommandBuffers();

		m_context.recreatingSwapChain = false;
		return true;
	}

	bool RendererVulkan::CreateBuffers()
	{
		constexpr auto baseFlagBits = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		constexpr auto memoryPropertyFlagBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		constexpr u64 vertexBufferSize = sizeof(Vertex3D) * 1024 * 1024;
		if (!m_objectVertexBuffer.Create(&m_context, vertexBufferSize, baseFlagBits | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryPropertyFlagBits, true))
		{
			Logger::PrefixError("VULKAN_RENDERER", "Error creating vertex buffer");
			return false;
		}
		m_geometryVertexOffset = 0;

		constexpr u64 indexBufferSize = sizeof(u32) * 1024 * 1024;
		if (!m_objectIndexBuffer.Create(&m_context, indexBufferSize, baseFlagBits | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryPropertyFlagBits, true))
		{
			Logger::PrefixError("VULKAN_RENDERER", "Error creating index buffer");
			return false;
		}
		m_geometryIndexOffset = 0;

		return true;
	}

	void RendererVulkan::UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, const VulkanBuffer* buffer, const u64 offset, const u64 size, const void* data)
	{
		constexpr VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		VulkanBuffer staging;
		staging.Create(&m_context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true);

		staging.LoadData(&m_context, 0, size, 0, data);
		staging.CopyTo(&m_context, pool, fence, queue, 0, buffer->handle, offset, size);

		staging.Destroy(&m_context);
	}
}
