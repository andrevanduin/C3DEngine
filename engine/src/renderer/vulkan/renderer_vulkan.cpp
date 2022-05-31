
#include "renderer_vulkan.h"

#include <VkBootstrap.h>
#include <SDL2/SDL_vulkan.h>

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_fence.h"
#include "vulkan_utils.h"

#include "core/logger.h"
#include "core/application.h"
#include "core/memory.h"

#include "resources/texture.h"

#include "renderer/vertex.h"
#include "services/services.h"

namespace C3D
{
	RendererVulkan::RendererVulkan(): m_context(), m_geometryVertexOffset(0), m_geometryIndexOffset(0) {}

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
		if (!m_context.device.Create(vkbInstance, &m_context))
		{
			Logger::Error("Failed to create Vulkan Device");
			return false;
		}

		m_context.swapChain.Create(&m_context, m_context.frameBufferWidth, m_context.frameBufferHeight);

		const auto area = ivec4(0, 0, static_cast<f32>(m_context.frameBufferWidth), static_cast<f32>(m_context.frameBufferHeight));
		constexpr auto clearColor = vec4(0.0f, 0.0f, 0.2f, 1.0f);


		m_context.mainRenderPass.Create(&m_context, area, clearColor, 1.0f, 0);

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

		if (!m_materialShader.Create(&m_context))
		{
			Logger::Error("Loading built-in object shader failed");
			return false;
		}

		CreateBuffers();

		// Mark all the geometry as invalid
		for (auto& geometry : m_geometries)
		{
			geometry.id = INVALID_ID;
		}

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

	bool RendererVulkan::BeginFrame(const f32 deltaTime)
	{
		Logger::PushPrefix("VULKAN_RENDERER");

		m_context.frameDeltaTime = deltaTime;
		const auto& device = m_context.device;

		// If we are recreating the SwapChain we should stop this frame
		if (m_context.recreatingSwapChain)
		{
			const auto result = vkDeviceWaitIdle(device.logicalDevice);
			if (!VulkanUtils::IsSuccess(result))
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
			if (!VulkanUtils::IsSuccess(result))
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
		if (!m_context.swapChain.AcquireNextImageIndex(&m_context, UINT64_MAX, 
			m_context.imageAvailableSemaphores[m_context.currentFrame], nullptr, &m_context.imageIndex))
		{
			return false;
		}

		// We can begin recording commands
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
		commandBuffer->Reset();
		commandBuffer->Begin(false, false, false);

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

		m_context.mainRenderPass.area.z = static_cast<i32>(m_context.frameBufferWidth);
		m_context.mainRenderPass.area.w = static_cast<i32>(m_context.frameBufferHeight);

		// Begin the RenderPass
		m_context.mainRenderPass.Begin(commandBuffer, m_context.swapChain.frameBuffers[m_context.imageIndex].handle);
		return true;
	}

	void RendererVulkan::UpdateGlobalState(const mat4 projection, const mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode)
	{
		m_materialShader.Use(&m_context);

		m_materialShader.globalUbo.projection = projection;
		m_materialShader.globalUbo.view = view;
		// TODO: other ubo properties here

		m_materialShader.UpdateGlobalState(&m_context, m_context.frameDeltaTime);
	}

	void RendererVulkan::DrawGeometry(const GeometryRenderData data)
	{
		if (!data.geometry || data.geometry->internalId == INVALID_ID) return;

		const auto bufferData = &m_geometries[data.geometry->internalId];
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		// TODO: check if this is actually needed
		m_materialShader.Use(&m_context);

		m_materialShader.SetModel(&m_context, data.model);

		Material* m = data.geometry->material ? data.geometry->material : Materials.GetDefault();
		m_materialShader.ApplyMaterial(&m_context, m);

		// Bind vertex buffer at offset
		const VkDeviceSize offsets[1] = { bufferData->vertexBufferOffset };
		vkCmdBindVertexBuffers(commandBuffer->handle, 0, 1, &m_objectVertexBuffer.handle, offsets);

		if (bufferData->indexCount > 0)
		{
			// Bind index buffer at offset
			vkCmdBindIndexBuffer(commandBuffer->handle, m_objectIndexBuffer.handle, bufferData->indexBufferOffset, VK_INDEX_TYPE_UINT32);

			// Issue the draw
			vkCmdDrawIndexed(commandBuffer->handle, bufferData->indexCount, 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(commandBuffer->handle, bufferData->vertexCount, 1, 0, 0);
		}
	}

	bool RendererVulkan::EndFrame(f32 deltaTime)
	{
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		// End the RenderPass
		m_context.mainRenderPass.End(commandBuffer);
		// End the CommandBuffer
		commandBuffer->End();

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
		commandBuffer->UpdateSubmitted();

		// Present the image (and give it back to the SwapChain)
		m_context.swapChain.Present(&m_context, m_context.device.graphicsQueue,
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

		m_materialShader.Destroy(&m_context);

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
			m_context.swapChain.frameBuffers[i].Destroy(&m_context);
		}

		m_context.mainRenderPass.Destroy(&m_context);

		m_context.swapChain.Destroy(&m_context);

		m_context.device.Destroy(&m_context);

		vkDestroySurfaceKHR(m_context.instance, m_context.surface, m_context.allocator);

		vkb::destroy_debug_utils_messenger(m_context.instance, m_debugMessenger);

		vkDestroyInstance(m_context.instance, m_context.allocator);

		Logger::PopPrefix();
	}

	void RendererVulkan::CreateTexture(const u8* pixels, Texture* texture)
	{
		// Internal data creation
		// TODO: use an allocator for this
		texture->internalData = Memory::Allocate<VulkanTextureData>(1, MemoryType::Texture);

		const auto data = static_cast<VulkanTextureData*>(texture->internalData);

		const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texture->width) * texture->height * texture->channelCount;
		constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM; // NOTE: Assumes 8 bits per channel

		constexpr VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		constexpr VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		VulkanBuffer staging;
		staging.Create(&m_context, imageSize, usage, memoryPropertyFlags, true);

		staging.LoadData(&m_context, 0, imageSize, 0, pixels);

		// NOTE: Lots of assumptions here, different texture types will require different options here!
		data->image.Create(&m_context, VK_IMAGE_TYPE_2D, texture->width, texture->height, imageFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT);

		VulkanCommandBuffer tempBuffer;
		VkCommandPool pool = m_context.device.graphicsCommandPool;

		tempBuffer.AllocateAndBeginSingleUse(&m_context, pool);

		data->image.TransitionLayout(&m_context, &tempBuffer, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		data->image.CopyFromBuffer(&m_context, staging.handle, &tempBuffer);

		data->image.TransitionLayout(&m_context, &tempBuffer, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		tempBuffer.EndSingleUse(&m_context, pool, m_context.device.graphicsQueue);

		staging.Destroy(&m_context);

		// TODO: These filters should be configurable
		VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = 16;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;

		const VkResult result = vkCreateSampler(m_context.device.logicalDevice, &samplerCreateInfo, m_context.allocator, &data->sampler);
		if (!VulkanUtils::IsSuccess(result))
		{
			Logger::PrefixError("VULKAN_RENDERER", "Error creating texture sampler: {}", VulkanUtils::ResultString(result, true));
			return;
		}

		texture->generation++;
	}

	bool RendererVulkan::CreateMaterial(Material* material)
	{
		if (material)
		{
			if (!m_materialShader.AcquireResources(&m_context, material))
			{
				Logger::PrefixError("VULKAN_RENDERER", "CreateMaterial() failed to acquire resources");
				return false;
			}

			Logger::PrefixTrace("VULKAN_RENDERER", "Material Created");
			return true;
		}

		Logger::PrefixError("VULKAN_RENDERER", "CreateMaterial() called with nullptr. Creation failed");
		return false;
	}

	bool RendererVulkan::CreateGeometry(Geometry* geometry, u32 vertexCount, const Vertex3D* vertices, u32 indexCount, const u32* indices)
	{
		if (!vertexCount || !vertices)
		{
			Logger::PrefixError("VULKAN_RENDERER", "CreateGeometry() requires vertex data and none was supplied.");
			return false;
		}

		// Check if this is a re-upload. If it is we need to free the old data afterwards
		const bool isReupload = geometry->internalId != INVALID_ID;
		VulkanGeometryData oldRange{};
		VulkanGeometryData* internalData = nullptr;
		if (isReupload)
		{
			internalData = &m_geometries[geometry->internalId];

			// Take a copy of the old data
			oldRange.indexBufferOffset = internalData->indexBufferOffset;
			oldRange.indexCount = internalData->indexCount;
			oldRange.indexSize = internalData->indexSize;
			oldRange.vertexBufferOffset = internalData->vertexBufferOffset;
			oldRange.vertexCount = internalData->vertexCount;
			oldRange.vertexSize = internalData->vertexSize;
		}
		else
		{
			for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; i++)
			{
				if (m_geometries[i].id == INVALID_ID)
				{
					geometry->internalId = i;
					m_geometries[i].id = i;
					internalData = &m_geometries[i];
					break;
				}
			}
		}

		if (!internalData)
		{
			Logger::PrefixFatal("VULKAN_RENDERER", "CreateGeometry() failed to find a free index for a new geometry upload. Adjust the config to allow for more");
			return false;
		}

		VkCommandPool pool = m_context.device.graphicsCommandPool;
		VkQueue queue = m_context.device.graphicsQueue;

		// Vertex data
		internalData->vertexBufferOffset = m_geometryVertexOffset;
		internalData->vertexCount = vertexCount;
		internalData->vertexSize = sizeof(Vertex3D) * vertexCount;

		UploadDataRange(pool, nullptr, queue, &m_objectVertexBuffer, internalData->vertexBufferOffset, internalData->vertexSize, vertices);
		// TODO: should maintain a free list instead of this
		m_geometryVertexOffset += internalData->vertexSize;

		// Index data, if applicable
		if (indexCount && indices)
		{
			internalData->indexBufferOffset = m_geometryIndexOffset;
			internalData->indexCount = indexCount;
			internalData->indexSize = sizeof(u32) * indexCount;

			UploadDataRange(pool, nullptr, queue, &m_objectIndexBuffer, internalData->indexBufferOffset, internalData->indexSize, indices);
			// TODO: should maintain a free list instead of this
			m_geometryIndexOffset += internalData->indexSize;
		}

		if (internalData->generation == INVALID_ID) internalData->generation = 0;
		else internalData->generation++;

		if (isReupload)
		{
			// Free vertex data
			FreeDataRange(&m_objectVertexBuffer, oldRange.vertexBufferOffset, oldRange.vertexSize);

			// Free index data, if applicable
			if (oldRange.indexSize > 0)
			{
				FreeDataRange(&m_objectIndexBuffer, oldRange.indexBufferOffset, oldRange.indexSize);
			}
		}

		return true;
	}

	void RendererVulkan::DestroyTexture(Texture* texture)
	{
		vkDeviceWaitIdle(m_context.device.logicalDevice);

		const auto data = static_cast<VulkanTextureData*>(texture->internalData);
		if (data)
		{
			data->image.Destroy(&m_context);
			Memory::Zero(&data->image, sizeof(VulkanImage));

			vkDestroySampler(m_context.device.logicalDevice, data->sampler, m_context.allocator);
			data->sampler = nullptr;

			Memory::Free(texture->internalData, sizeof(VulkanTextureData), MemoryType::Texture);
		}
		
		Memory::Zero(texture, sizeof(Texture));
	}

	void RendererVulkan::DestroyMaterial(Material* material)
	{
		if (material)
		{
			if (material->internalId != INVALID_ID)
			{
				m_materialShader.ReleaseResources(&m_context, material);
			}
			else
			{
				Logger::PrefixWarn("VULKAN_RENDERER", "DestroyMaterial() called with internalId = INVALID_ID. Ignoring request");
			}
		}
		else
		{
			Logger::PrefixWarn("VULKAN_RENDERER", "DestroyMaterial() called with nullptr. Ignoring request");
		}
	}

	void RendererVulkan::DestroyGeometry(Geometry* geometry)
	{
		if (geometry && geometry->internalId != INVALID_ID)
		{
			vkDeviceWaitIdle(m_context.device.logicalDevice);

			VulkanGeometryData* internalData = &m_geometries[geometry->internalId];

			// Free vertex data
			FreeDataRange(&m_objectVertexBuffer, internalData->vertexBufferOffset, internalData->vertexSize);

			// Free index data, if applicable
			if (internalData->indexSize > 0)
			{
				FreeDataRange(&m_objectIndexBuffer, internalData->indexBufferOffset, internalData->indexSize);
			}

			// Clean up data
			Memory::Zero(internalData, sizeof(VulkanGeometryData));
			internalData->id = INVALID_ID;
			internalData->generation = INVALID_ID;
		}
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
				m_context.graphicsCommandBuffers[i].Free(&m_context, m_context.device.graphicsCommandPool);
			}

			m_context.graphicsCommandBuffers[i].Allocate(&m_context, m_context.device.graphicsCommandPool, true);
		}
	}

	void RendererVulkan::RegenerateFrameBuffers(const VulkanSwapChain* swapChain, VulkanRenderPass* renderPass)
	{
		for (u32 i = 0; i < swapChain->imageCount; i++)
		{
			// TODO: Make this dynamic based on the currently configured attachments
			constexpr u32 attachmentCount = 2;
			const VkImageView attachments[] = { swapChain->views[i], swapChain->depthAttachment.view };

			m_context.swapChain.frameBuffers[i].Create(&m_context, renderPass, m_context.frameBufferWidth, 
				m_context.frameBufferHeight, attachmentCount, attachments);
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
		m_context.device.QuerySwapChainSupport(m_context.surface, &m_context.device.swapChainSupport);
		m_context.device.DetectDepthFormat();

		m_context.swapChain.Recreate(&m_context, m_context.cachedFrameBufferWidth, m_context.cachedFrameBufferHeight);

		// Sync the FrameBuffer size with the cached sizes
		m_context.frameBufferWidth = m_context.cachedFrameBufferWidth;
		m_context.frameBufferHeight = m_context.cachedFrameBufferHeight;
		m_context.mainRenderPass.area.z = m_context.frameBufferWidth;
		m_context.mainRenderPass.area.w = m_context.frameBufferHeight;
		m_context.cachedFrameBufferWidth = 0;
		m_context.cachedFrameBufferHeight = 0;

		// Update the size generation so that they are in sync again
		m_context.frameBufferSizeLastGeneration = m_context.frameBufferSizeGeneration;

		// Cleanup SwapChain
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			m_context.graphicsCommandBuffers[i].Free(&m_context, m_context.device.graphicsCommandPool);
		}

		// Destroy FrameBuffers
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			m_context.swapChain.frameBuffers[i].Destroy(&m_context);
		}

		m_context.mainRenderPass.area.x = 0;
		m_context.mainRenderPass.area.y = 0;
		m_context.mainRenderPass.area.z = static_cast<i32>(m_context.frameBufferWidth);
		m_context.mainRenderPass.area.w = static_cast<i32>(m_context.frameBufferHeight);

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

	void RendererVulkan::UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, const VulkanBuffer* buffer, const u64 offset, const u64 size, const void* data) const
	{
		constexpr VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		VulkanBuffer staging;
		staging.Create(&m_context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true);

		staging.LoadData(&m_context, 0, size, 0, data);
		staging.CopyTo(&m_context, pool, fence, queue, 0, buffer->handle, offset, size);

		staging.Destroy(&m_context);
	}

	void RendererVulkan::FreeDataRange(VulkanBuffer* buffer, u64 offset, u64 size)
	{
		// TODO: Free this in the buffer
		// TODO: update free list with this range being free
	}
}
