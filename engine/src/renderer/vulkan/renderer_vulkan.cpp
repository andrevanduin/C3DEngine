
#include "renderer_vulkan.h"

#include <VkBootstrap.h>
#include <SDL2/SDL_vulkan.h>

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_shader.h"
#include "vulkan_utils.h"

#include "core/logger.h"
#include "core/application.h"
#include "core/c3d_string.h"
#include "core/memory.h"

#include "resources/texture.h"
#include "resources/shader.h"

#include "renderer/vertex.h"
#include "resources/resource_types.h"

#include "services/services.h"
#include "systems/resource_system.h"
#include "systems/texture_system.h"

namespace C3D
{
	RendererVulkan::RendererVulkan()
		: RendererBackend("VULKAN_RENDERER"), m_context(), m_geometries{}
	{}

	bool RendererVulkan::Init(Application* application)
	{
		type = RendererBackendType::Vulkan;

		// TODO: Possibly add a custom allocator
		m_context.allocator = nullptr;

		application->GetFrameBufferSize(&m_context.cachedFrameBufferWidth, &m_context.cachedFrameBufferHeight);
		m_context.frameBufferWidth = (m_context.cachedFrameBufferWidth != 0) ? m_context.cachedFrameBufferWidth : 1280;
		m_context.frameBufferHeight = (m_context.cachedFrameBufferHeight != 0) ? m_context.cachedFrameBufferHeight : 720;
		m_context.cachedFrameBufferWidth = 0;
		m_context.cachedFrameBufferHeight = 0;

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

		m_logger.Info("Instance Initialized");
		const vkb::Instance vkbInstance = vkbInstanceResult.value();

		m_context.instance = vkbInstance.instance;
#if defined(_DEBUG)
		m_debugMessenger = vkbInstance.debug_messenger;
#endif

		if (!SDL_Vulkan_CreateSurface(application->GetWindow(), m_context.instance, &m_context.surface))
		{
			m_logger.Error("Failed to create Vulkan Surface");
			return false;
		}

		m_logger.Info("SDL Surface Initialized");
		if (!m_context.device.Create(vkbInstance, &m_context))
		{
			m_logger.Error("Failed to create Vulkan Device");
			return false;
		}

		m_context.swapChain.Create(&m_context, m_context.frameBufferWidth, m_context.frameBufferHeight);

		const auto area = ivec4(0, 0, static_cast<f32>(m_context.frameBufferWidth), static_cast<f32>(m_context.frameBufferHeight));
		constexpr auto clearColor = vec4(0.0f, 0.0f, 0.2f, 1.0f);

		// World RenderPass
		m_context.mainRenderPass.Create(&m_context, area, clearColor, 1.0f, 0, ClearColor | ClearDepth | ClearStencil, false, true);
		// UI RenderPass
		m_context.uiRenderPass.Create(&m_context, area, vec4(0), 1.0f, 0, ClearNone, true, false);

		// Regenerate SwapChain and World FrameBuffers
		RegenerateFrameBuffers();

		CreateCommandBuffers();
		m_logger.Info("Command Buffers Initialized");

		m_context.imageAvailableSemaphores.resize(m_context.swapChain.maxFramesInFlight);
		m_context.queueCompleteSemaphores.resize(m_context.swapChain.maxFramesInFlight);

		m_logger.Info("Creating Semaphores and Fences");
		for (u8 i = 0; i < m_context.swapChain.maxFramesInFlight; i++)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			vkCreateSemaphore(m_context.device.logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.imageAvailableSemaphores[i]);
			vkCreateSemaphore(m_context.device.logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.queueCompleteSemaphores[i]);

			VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK(vkCreateFence(m_context.device.logicalDevice, &fenceCreateInfo, m_context.allocator, &m_context.inFlightFences[i]));
		}

		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			m_context.imagesInFlight[i] = nullptr;
		}

		CreateBuffers();

		// Mark all the geometry as invalid
		for (auto& geometry : m_geometries)
		{
			geometry.id = INVALID_ID;
		}

		m_logger.Info("Successfully Initialized");
		return true;
	}

	void RendererVulkan::Shutdown()
	{
		m_logger.Info("Shutting Down");

		// Wait for our device to be finished with it's current frame
		m_context.device.WaitIdle();

		// Destroy stuff in opposite order of creation
		m_objectVertexBuffer.Destroy(&m_context);
		m_objectIndexBuffer.Destroy(&m_context);

		m_logger.Info("Destroying Semaphores and Fences");
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
			vkDestroyFence(m_context.device.logicalDevice, m_context.inFlightFences[i], m_context.allocator);
		}
		m_context.imageAvailableSemaphores.clear();
		m_context.queueCompleteSemaphores.clear();

		vkDestroyCommandPool(m_context.device.logicalDevice, m_context.device.graphicsCommandPool, m_context.allocator);
		m_context.graphicsCommandBuffers.clear();

		m_logger.Info("Destroying FrameBuffers");
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			vkDestroyFramebuffer(m_context.device.logicalDevice, m_context.worldFrameBuffers[i], m_context.allocator);
			vkDestroyFramebuffer(m_context.device.logicalDevice, m_context.swapChain.frameBuffers[i], m_context.allocator);
		}

		m_logger.Info("Destroying RenderPasses");
		m_context.uiRenderPass.Destroy(&m_context);
		m_context.mainRenderPass.Destroy(&m_context);

		m_logger.Info("Destroying SwapChain");
		m_context.swapChain.Destroy(&m_context);

		m_logger.Info("Destroying Device");
		m_context.device.Destroy(&m_context);

		m_logger.Info("Destroying Vulkan Surface");
		vkDestroySurfaceKHR(m_context.instance, m_context.surface, m_context.allocator);

#ifdef _DEBUG
		m_logger.Info("Destroying Vulkan debug messenger");
		vkb::destroy_debug_utils_messenger(m_context.instance, m_debugMessenger);
#endif

		m_logger.Info("Destroying Instance");
		vkDestroyInstance(m_context.instance, m_context.allocator);
	}

	void RendererVulkan::OnResize(const u16 width, const u16 height)
	{
		m_context.cachedFrameBufferWidth = width;
		m_context.cachedFrameBufferHeight = height;
		m_context.frameBufferSizeGeneration++;

		m_logger.Info("OnResize() with Width: {}, Height: {} and Generation: {}", width, height, m_context.frameBufferSizeGeneration);
	}

	bool RendererVulkan::BeginFrame(const f32 deltaTime)
	{
		m_context.frameDeltaTime = deltaTime;
		const auto& device = m_context.device;

		// If we are recreating the SwapChain we should stop this frame
		if (m_context.recreatingSwapChain)
		{
			const auto result = vkDeviceWaitIdle(device.logicalDevice);
			if (!VulkanUtils::IsSuccess(result))
			{
				m_logger.Error("vkDeviceWaitIdle (1) failed: {}", VulkanUtils::ResultString(result, true));
				return false;
			}
			m_logger.Info("Recreating SwapChain. Stopping BeginFrame()");
			return false;
		}

		// If the FrameBuffer was resized we must also create a new SwapChain.
		if (m_context.frameBufferSizeGeneration != m_context.frameBufferSizeLastGeneration)
		{
			// FrameBuffer was resized. We need to recreate it.
			const auto result = vkDeviceWaitIdle(device.logicalDevice);
			if (!VulkanUtils::IsSuccess(result))
			{
				m_logger.Error("vkDeviceWaitIdle (2) failed: {}", VulkanUtils::ResultString(result, true));
				return false;
			}

			if (!RecreateSwapChain())
			{
				return false;
			}

			m_logger.Info("SwapChain Resized successfully. Stopping BeginFrame()");
			return false;
		}

		// Wait for the execution of the current frame to complete.
		const VkResult result = vkWaitForFences(m_context.device.logicalDevice, 1, &m_context.inFlightFences[m_context.currentFrame], true, UINT64_MAX);
		if (!VulkanUtils::IsSuccess(result))
		{
			m_logger.Fatal("vkWaitForFences() failed: '{}'", VulkanUtils::ResultString(result));
			return false;
		}

		// Acquire the next image from the SwapChain. Pass along the semaphore that should be signaled when this completes.
		// This same semaphore will later be waited on by the queue submission to ensure this image is available.
		if (!m_context.swapChain.AcquireNextImageIndex(&m_context, UINT64_MAX, 
			m_context.imageAvailableSemaphores[m_context.currentFrame], nullptr, &m_context.imageIndex))
		{
			m_logger.Error("Failed to acquire next image index.");
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

		// Update the main dimensions
		m_context.mainRenderPass.area.z = static_cast<i32>(m_context.frameBufferWidth);
		m_context.mainRenderPass.area.w = static_cast<i32>(m_context.frameBufferHeight);

		// Update the UI dimensions
		m_context.uiRenderPass.area.z = static_cast<i32>(m_context.frameBufferWidth);
		m_context.uiRenderPass.area.w = static_cast<i32>(m_context.frameBufferHeight);

		return true;
	}

	bool RendererVulkan::EndFrame(f32 deltaTime)
	{
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		// End the CommandBuffer
		commandBuffer->End();

		// Ensure that the previous frame is not using this image
		if (m_context.imagesInFlight[m_context.imageIndex] != VK_NULL_HANDLE)
		{
			const VkResult result = vkWaitForFences(m_context.device.logicalDevice, 1, m_context.imagesInFlight[m_context.imageIndex], true, UINT64_MAX);
			if (!VulkanUtils::IsSuccess(result))
			{
				m_logger.Fatal("vkWaitForFences() failed: '{}'", VulkanUtils::ResultString(result));
			}
		}

		// Mark the image fence as in-use by this frame
		m_context.imagesInFlight[m_context.imageIndex] = &m_context.inFlightFences[m_context.currentFrame];

		// Reset the fence for use on the next frame
		VK_CHECK(vkResetFences(m_context.device.logicalDevice, 1, &m_context.inFlightFences[m_context.currentFrame]));

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer->handle;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_context.queueCompleteSemaphores[m_context.currentFrame];

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_context.imageAvailableSemaphores[m_context.currentFrame];

		constexpr VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.pWaitDstStageMask = flags;

		// Submit all the commands that we have queued
		const auto result = vkQueueSubmit(m_context.device.graphicsQueue, 1, &submitInfo, 
			m_context.inFlightFences[m_context.currentFrame]);

		if (result != VK_SUCCESS)
		{
			m_logger.Error("vkQueueSubmit failed with result: {}", VulkanUtils::ResultString(result, true));
			return false;
		}

		// Queue submission is done
		commandBuffer->UpdateSubmitted();

		// Present the image (and give it back to the SwapChain)
		m_context.swapChain.Present(&m_context, m_context.device.graphicsQueue,
			m_context.device.presentQueue, m_context.queueCompleteSemaphores[m_context.currentFrame], m_context.imageIndex);

		state.frameNumber++;
		return true;
	}

	bool RendererVulkan::BeginRenderPass(u8 renderPassId)
	{
		const VulkanRenderPass* renderPass;
		VkFramebuffer frameBuffer;
		VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		// Choose a RenderPass based on id
		switch (renderPassId)
		{
			case BuiltinRenderPass::World:
				renderPass = &m_context.mainRenderPass;
				frameBuffer = m_context.worldFrameBuffers[m_context.imageIndex];
				break;
			case BuiltinRenderPass::Ui:
				renderPass = &m_context.uiRenderPass;
				frameBuffer = m_context.swapChain.frameBuffers[m_context.imageIndex];
				break;
			default:
				m_logger.Error("BeginRenderPass() called with unrecognized RenderPass id {}", renderPassId);
				return false;
		}

		// Begin the RenderPass
		renderPass->Begin(commandBuffer, frameBuffer);
		return true;
	}

	bool RendererVulkan::EndRenderPass(u8 renderPassId)
	{
		const VulkanRenderPass* renderPass;
		VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		// Choose a RenderPass based on id
		switch (renderPassId)
		{
		case BuiltinRenderPass::World:
			renderPass = &m_context.mainRenderPass;
			break;
		case BuiltinRenderPass::Ui:
			renderPass = &m_context.uiRenderPass;
			break;
		default:
			m_logger.Error("EndRenderPass() called with unrecognized RenderPass id {}", renderPassId);
			return false;
		}

		// End our RenderPass
		renderPass->End(commandBuffer);
		return true;
	}

	void RendererVulkan::DrawGeometry(const GeometryRenderData data)
	{
		// Simply ignore if there is no geometry to draw
		if (!data.geometry || data.geometry->internalId == INVALID_ID) return;

		const auto bufferData = &m_geometries[data.geometry->internalId];
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		// Bind vertex buffer at offset
		const VkDeviceSize offsets[1] = { bufferData->vertexBufferOffset };
		vkCmdBindVertexBuffers(commandBuffer->handle, 0, 1, &m_objectVertexBuffer.handle, offsets);

		if (bufferData->indexCount > 0)
		{
			// We have indices so we should draw indexed

			// Bind index buffer at offset
			vkCmdBindIndexBuffer(commandBuffer->handle, m_objectIndexBuffer.handle, bufferData->indexBufferOffset, VK_INDEX_TYPE_UINT32);

			// Issue the draw
			vkCmdDrawIndexed(commandBuffer->handle, bufferData->indexCount, 1, 0, 0, 0);
		}
		else
		{
			// No indices so we do a simple non-indexed draw
			vkCmdDraw(commandBuffer->handle, bufferData->vertexCount, 1, 0, 0);
		}
	}

	void RendererVulkan::CreateTexture(const u8* pixels, Texture* texture)
	{
		// Internal data creation
		texture->internalData = Memory.Allocate(sizeof(VulkanTextureData), MemoryType::Texture);

		const auto data = static_cast<VulkanTextureData*>(texture->internalData);

		const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texture->width) * texture->height * texture->channelCount;
		constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM; // NOTE: Assumes 8 bits per channel

		constexpr VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		constexpr VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		VulkanBuffer staging;
		staging.Create(&m_context, imageSize, usage, memoryPropertyFlags, true, false);

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
			m_logger.Error("Error creating texture sampler: {}", VulkanUtils::ResultString(result, true));
			return;
		}

		texture->generation++;
	}

	void RendererVulkan::DestroyTexture(Texture* texture)
	{
		vkDeviceWaitIdle(m_context.device.logicalDevice);

		const auto data = static_cast<VulkanTextureData*>(texture->internalData);
		if (data)
		{
			data->image.Destroy(&m_context);
			Memory.Zero(&data->image, sizeof(VulkanImage));

			vkDestroySampler(m_context.device.logicalDevice, data->sampler, m_context.allocator);
			data->sampler = nullptr;

			Memory.Free(texture->internalData, sizeof(VulkanTextureData), MemoryType::Texture);
		}
		
		Memory.Zero(texture, sizeof(Texture));
	}

	bool RendererVulkan::CreateGeometry(Geometry* geometry, u32 vertexSize, const u32 vertexCount, const void* vertices,
		u32 indexSize, const u32 indexCount, const void* indices)
	{
		if (!vertexCount || !vertices)
		{
			m_logger.Error("CreateGeometry() requires vertex data and none was supplied.");
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
			oldRange.indexElementSize = internalData->indexElementSize;
			oldRange.vertexBufferOffset = internalData->vertexBufferOffset;
			oldRange.vertexCount = internalData->vertexCount;
			oldRange.vertexElementSize = internalData->vertexElementSize;
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
			m_logger.Fatal("CreateGeometry() failed to find a free index for a new geometry upload. Adjust the config to allow for more");
			return false;
		}

		VkCommandPool pool = m_context.device.graphicsCommandPool;
		VkQueue queue = m_context.device.graphicsQueue;

		// Vertex data
		internalData->vertexCount = vertexCount;
		internalData->vertexElementSize = sizeof(Vertex3D);

		u32 totalSize = vertexCount * vertexSize;
		if (!UploadDataRange(pool, nullptr, queue, &m_objectVertexBuffer, &internalData->vertexBufferOffset, totalSize, vertices))
		{
			m_logger.Error("CreateGeometry() failed to upload to the vertex buffer");
			return false;
		}

		// Index data, if applicable
		if (indexCount && indices)
		{
			internalData->indexCount = indexCount;
			internalData->indexElementSize = sizeof(u32);

			totalSize = indexCount * indexSize;
			if (!UploadDataRange(pool, nullptr, queue, &m_objectIndexBuffer, &internalData->indexBufferOffset, totalSize, indices))
			{
				m_logger.Error("CreateGeometry() failed to upload to the index buffer");
				return false;
			}
		}

		if (internalData->generation == INVALID_ID) internalData->generation = 0;
		else internalData->generation++;

		if (isReupload)
		{
			// Free vertex data
			FreeDataRange(&m_objectVertexBuffer, oldRange.vertexBufferOffset, static_cast<u64>(oldRange.vertexElementSize) * oldRange.vertexCount);

			// Free index data, if applicable
			if (oldRange.indexElementSize > 0)
			{
				FreeDataRange(&m_objectIndexBuffer, oldRange.indexBufferOffset, static_cast<u64>(oldRange.indexElementSize) * oldRange.indexCount);
			}
		}

		return true;
	}

	void RendererVulkan::DestroyGeometry(Geometry* geometry)
	{
		if (geometry && geometry->internalId != INVALID_ID)
		{
			vkDeviceWaitIdle(m_context.device.logicalDevice);

			VulkanGeometryData* internalData = &m_geometries[geometry->internalId];

			// Free vertex data
			FreeDataRange(&m_objectVertexBuffer, internalData->vertexBufferOffset, static_cast<u64>(internalData->vertexElementSize) * internalData->vertexCount);

			// Free index data, if applicable
			if (internalData->indexElementSize > 0)
			{
				FreeDataRange(&m_objectIndexBuffer, internalData->indexBufferOffset, static_cast<u64>(internalData->indexElementSize) * internalData->indexCount);
			}

			// Clean up data
			Memory.Zero(internalData, sizeof(VulkanGeometryData));
			internalData->id = INVALID_ID;
			internalData->generation = INVALID_ID;
		}
	}

	bool RendererVulkan::CreateShader(Shader* shader, const u8 renderPassId, const std::vector<char*>& stageFileNames, const std::vector<ShaderStage>& stages)
	{
		// Allocate enough memory for the 
		shader->apiSpecificData = Memory.Allocate<VulkanShader>(MemoryType::Shader);

		// TODO: dynamic RenderPasses
		VulkanRenderPass* renderPass = renderPassId == 1 ? &m_context.mainRenderPass : &m_context.uiRenderPass;

		// Translate stages
		VkShaderStageFlags vulkanStages[VULKAN_SHADER_MAX_STAGES];
		for (u8 i = 0; i < stageFileNames.size(); i++) {
			switch (stages[i]) {
			case ShaderStage::Fragment:
				vulkanStages[i] = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			case ShaderStage::Vertex:
				vulkanStages[i] = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStage::Geometry:
				m_logger.Warn("CreateShader() - VK_SHADER_STAGE_GEOMETRY_BIT is set but not yet supported.");
				vulkanStages[i] = VK_SHADER_STAGE_GEOMETRY_BIT;
				break;
			case ShaderStage::Compute:
				m_logger.Warn("CreateShader() - SHADER_STAGE_COMPUTE is set but not yet supported.");
				vulkanStages[i] = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			}
		}

		// TODO: Make the max descriptor allocate count configurable
		constexpr u32 maxDescriptorAllocateCount = 1024;

		// Get a pointer to our Vulkan specific shader stuff
		const auto vulkanShader = static_cast<VulkanShader*>(shader->apiSpecificData);
		vulkanShader->renderPass = renderPass;
		vulkanShader->config.maxDescriptorSetCount = maxDescriptorAllocateCount;

		vulkanShader->config.stageCount = 0;
		for (u32 i = 0; i < stageFileNames.size(); i++)
		{
			// Make sure we have enough room left for this stage
			if (vulkanShader->config.stageCount + 1 > VULKAN_SHADER_MAX_STAGES)
			{
				m_logger.Error("CreateShader() - Shaders may have a maximum of {} stages.", VULKAN_SHADER_MAX_STAGES);
				return false;
			}

			// Check if we support this stage
			VkShaderStageFlagBits stageFlag;
			switch (stages[i])
			{
				case ShaderStage::Vertex:
					stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
					break;
				case ShaderStage::Fragment:
					stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
					break;
				default:
					m_logger.Error("CreateShader() - Unsupported shader stage {}. Stage ignored.", ToUnderlying(stages[i]));
					continue;
			}

			// Set the stage and increment the stage count
			const auto stageIndex = vulkanShader->config.stageCount;
			vulkanShader->config.stages[stageIndex].stage = stageFlag;
			StringNCopy(vulkanShader->config.stages[stageIndex].fileName, stageFileNames[i], VULKAN_SHADER_STAGE_CONFIG_FILENAME_MAX_LENGTH);
			vulkanShader->config.stageCount++;
		}

		// TODO: For now, shaders will only ever have these 2 types of descriptor pools
		vulkanShader->config.poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 };			// HACK: max number of ubo descriptor sets
		vulkanShader->config.poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 };	// HACK: max number of image sampler descriptor sets

		// Global descriptor set config
		VulkanDescriptorSetConfig globalDescriptorSetConfig{};
		// We will always have a uniform buffer object available first
		globalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
		globalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
		globalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		globalDescriptorSetConfig.bindingCount++;

		vulkanShader->config.descriptorSets[DESC_SET_INDEX_GLOBAL] = globalDescriptorSetConfig;
		vulkanShader->config.descriptorSetCount++;

		if (shader->useInstances)
		{
			// If we are using instances we need to add a second descriptor set
			VulkanDescriptorSetConfig instanceDescriptorSetConfig{};
			// Add a uniform buffer object to it, as instances should always have one available
			// NOTE: might be a good idea to only add this if it is going to be used...
			instanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
			instanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
			instanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			instanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			instanceDescriptorSetConfig.bindingCount++;

			vulkanShader->config.descriptorSets[DESC_SET_INDEX_INSTANCE] = instanceDescriptorSetConfig;
			vulkanShader->config.descriptorSetCount++;
		}

		// Invalidate all instance states.
		// TODO: make this dynamic
		for (auto& instanceState : vulkanShader->instanceStates)
		{
			instanceState.id = INVALID_ID;
		}

		return true;
	}

	void RendererVulkan::DestroyShader(Shader* shader)
	{
		// Make sure there is something to destroy
		if (shader && shader->apiSpecificData)
		{
			const auto vulkanShader = static_cast<VulkanShader*>(shader->apiSpecificData);

			VkDevice logicalDevice = m_context.device.logicalDevice;
			const VkAllocationCallbacks* vkAllocator = m_context.allocator;

			// Cleanup the descriptor set layouts
			for (u32 i = 0; i < vulkanShader->config.descriptorSetCount; i++)
			{
				if (vulkanShader->descriptorSetLayouts[i])
				{
					vkDestroyDescriptorSetLayout(logicalDevice, vulkanShader->descriptorSetLayouts[i], vkAllocator);
					vulkanShader->descriptorSetLayouts[i] = nullptr;
				}
			}

			// Cleanup descriptor pool
			if (vulkanShader->descriptorPool)
			{
				vkDestroyDescriptorPool(logicalDevice, vulkanShader->descriptorPool, vkAllocator);
			}

			// Cleanup Uniform buffer
			vulkanShader->uniformBuffer.UnlockMemory(&m_context);
			vulkanShader->mappedUniformBufferBlock = nullptr;
			vulkanShader->uniformBuffer.Destroy(&m_context);

			// Cleanup Pipeline
			vulkanShader->pipeline.Destroy(&m_context);

			// Cleanup Shader Modules
			for (u32 i = 0; i < vulkanShader->config.stageCount; i++)
			{
				vkDestroyShaderModule(logicalDevice, vulkanShader->stages[i].handle, vkAllocator);
			}

			// Destroy the configuration
			Memory.Zero(&vulkanShader->config, sizeof(VulkanShaderConfig));

			// Free the api (Vulkan in this case) specific data from the shader
			Memory.Free(shader->apiSpecificData, sizeof(VulkanShader), MemoryType::Shader);
			shader->apiSpecificData = nullptr;
		}
	}

	bool RendererVulkan::InitializeShader(Shader* shader)
	{
		VkDevice logicalDevice = m_context.device.logicalDevice;
		const VkAllocationCallbacks* vkAllocator = m_context.allocator;
		const auto vulkanShader = static_cast<VulkanShader*>(shader->apiSpecificData);

		for (u32 i = 0; i < vulkanShader->config.stageCount; i++)
		{
			if (!CreateModule(vulkanShader->config.stages[i], &vulkanShader->stages[i]))
			{
				m_logger.Error("InitializeShader() - Unable to create '{}' shader module for '{}'. Shader will be destroyed.", 
					vulkanShader->config.stages[i].fileName, shader->name);
				return false;
			}
		}

		// Static lookup table for our types -> Vulkan ones.
		static VkFormat* types = nullptr;
		static VkFormat t[11];
		if (!types) {
			t[Attribute_Float32]	= VK_FORMAT_R32_SFLOAT;
			t[Attribute_Float32_2]	= VK_FORMAT_R32G32_SFLOAT;
			t[Attribute_Float32_3]	= VK_FORMAT_R32G32B32_SFLOAT;
			t[Attribute_Float32_4]	= VK_FORMAT_R32G32B32A32_SFLOAT;
			t[Attribute_Int8]		= VK_FORMAT_R8_SINT;
			t[Attribute_UInt8]		= VK_FORMAT_R8_UINT;
			t[Attribute_Int16]		= VK_FORMAT_R16_SINT;
			t[Attribute_UInt16]		= VK_FORMAT_R16_UINT;
			t[Attribute_Int32]		= VK_FORMAT_R32_SINT;
			t[Attribute_UInt32]		= VK_FORMAT_R32_UINT;
			types = t;
		}

		// Process attributes
		const u64 attributeCount = shader->attributes.Size();
		u32 offset = 0;
		for (u32 i = 0; i < attributeCount; i++)
		{
			// Setup the new attribute
			VkVertexInputAttributeDescription attribute{};
			attribute.location = i;
			attribute.binding = 0;
			attribute.offset = offset;
			attribute.format = types[shader->attributes[i].type];

			vulkanShader->config.attributes[i] = attribute;
			offset += shader->attributes[i].size;
		}

		const u64 uniformCount = shader->uniforms.Size();
		for (u32 i = 0; i < uniformCount; i++)
		{
			// For Samplers, the descriptor bindings need to be updated. Other types of uniforms don't need anything to be done here.
			if (shader->uniforms[i].type == Uniform_Sampler)
			{
				const u32 setIndex = shader->uniforms[i].scope == ShaderScope::Global ? DESC_SET_INDEX_GLOBAL : DESC_SET_INDEX_INSTANCE;
				VulkanDescriptorSetConfig* setConfig = &vulkanShader->config.descriptorSets[setIndex];
				if (setConfig->bindingCount < 2)
				{
					// There isn't a binding yet, meaning this is the first sampler to be added.
					// Create the binding with a single descriptor for this sampler.
					setConfig->bindings[BINDING_INDEX_SAMPLER].binding = BINDING_INDEX_SAMPLER; // Will always be the second one.
					setConfig->bindings[BINDING_INDEX_SAMPLER].descriptorCount = 1;
					setConfig->bindings[BINDING_INDEX_SAMPLER].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					setConfig->bindings[BINDING_INDEX_SAMPLER].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
					setConfig->bindingCount++;
				}
				else
				{
					// There is already a binding for samplers, so we just add a descriptor to it.
					setConfig->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
				}
			}
		}

		// Create descriptor pool
		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = vulkanShader->config.poolSizes;
		poolInfo.maxSets = vulkanShader->config.maxDescriptorSetCount;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkResult result = vkCreateDescriptorPool(logicalDevice, &poolInfo, vkAllocator, &vulkanShader->descriptorPool);
		if (!VulkanUtils::IsSuccess(result))
		{
			m_logger.Error("InitializeShader() - Failed to create descriptor pool: {}", VulkanUtils::ResultString(result));
			return false;
		}

		// Create descriptor set layouts
		for (u32 i = 0; i < vulkanShader->config.descriptorSetCount; i++)
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			layoutInfo.bindingCount = vulkanShader->config.descriptorSets[i].bindingCount;
			layoutInfo.pBindings = vulkanShader->config.descriptorSets[i].bindings;
			result = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, vkAllocator, &vulkanShader->descriptorSetLayouts[i]);
			if (!VulkanUtils::IsSuccess(result))
			{
				m_logger.Error("InitializeShader() - Failed to create Descriptor Set Layout: {}", VulkanUtils::ResultString(result));
				return false;
			}
		}

		// TODO: This shouldn't be here :(
		const auto fWidth  = static_cast<f32>(m_context.frameBufferWidth);
		const auto fHeight = static_cast<f32>(m_context.frameBufferHeight);

		// Viewport
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = fHeight;
		viewport.width = fWidth;
		viewport.height = -fHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// Scissor
		VkRect2D scissor;
		scissor.offset.x = scissor.offset.y = 0;
		scissor.extent.width = m_context.frameBufferWidth;
		scissor.extent.height = m_context.frameBufferHeight;

		VkPipelineShaderStageCreateInfo stageCreateInfos[VULKAN_SHADER_MAX_STAGES];
		Memory.Zero(stageCreateInfos, sizeof(VkPipelineShaderStageCreateInfo)* VULKAN_SHADER_MAX_STAGES);
		for (u32 i = 0; i < vulkanShader->config.stageCount; i++)
		{
			stageCreateInfos[i] = vulkanShader->stages[i].shaderStageCreateInfo;
		}

		const bool pipelineCreateResult = vulkanShader->pipeline.Create(
			&m_context, vulkanShader->renderPass,
			shader->attributeStride, static_cast<u32>(shader->attributes.Size()), vulkanShader->config.attributes,
			vulkanShader->config.descriptorSetCount, vulkanShader->descriptorSetLayouts,
			vulkanShader->config.stageCount, stageCreateInfos, viewport, scissor, false, true, 
			shader->pushConstantRangeCount, shader->pushConstantRanges
		);

		if (!pipelineCreateResult)
		{
			m_logger.Error("InitializeShader() - Failed to load graphics pipeline");
			return false;
		}

		// Grab the UBO alignment requirement from our device
		shader->requiredUboAlignment = m_context.device.properties.limits.minUniformBufferOffsetAlignment;

		// Make sure the UBO is aligned according to device requirements
		shader->globalUboStride = GetAligned(shader->globalUboSize, shader->requiredUboAlignment);
		shader->uboStride = GetAligned(shader->uboSize, shader->requiredUboAlignment);

		// Uniform buffer
		const u32 deviceLocalBits = m_context.device.supportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
		// TODO: max count should be configurable, or perhaps long term support of buffer resizing
		u64 totalBufferSize = shader->globalUboStride + shader->uboStride * VULKAN_MAX_MATERIAL_COUNT;
		if (!vulkanShader->uniformBuffer.Create(&m_context, totalBufferSize, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | deviceLocalBits,
			true, true))
		{
			m_logger.Error("InitializeShader() - Failed to create VulkanBuffer");
			return false;
		}

		// Allocate space for the global UBO, which should occupy the stride space and not the actual size need
		if (!vulkanShader->uniformBuffer.Allocate(shader->globalUboStride, &shader->globalUboOffset))
		{
			m_logger.Error("InitializeShader() - Failed to allocate space for the uniform buffer");
			return false;
		}

		// Map the entire buffer's memory
		vulkanShader->mappedUniformBufferBlock = vulkanShader->uniformBuffer.LockMemory(&m_context, 0, VK_WHOLE_SIZE, 0);

		const VkDescriptorSetLayout globalLayouts[3] =
		{
			vulkanShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
			vulkanShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
			vulkanShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
		};

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = vulkanShader->descriptorPool;
		allocInfo.descriptorSetCount = 3;
		allocInfo.pSetLayouts = globalLayouts;
		VK_CHECK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, vulkanShader->globalDescriptorSets));

		return true;
	}

	bool RendererVulkan::UseShader(Shader* shader)
	{
		const auto vulkanShader = static_cast<VulkanShader*>(shader->apiSpecificData);
		vulkanShader->pipeline.Bind(&m_context.graphicsCommandBuffers[m_context.imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS);
		return true;
	}

	bool RendererVulkan::ShaderBindGlobals(Shader* shader)
	{
		if (!shader) return false;
		shader->boundUboOffset = static_cast<u32>(shader->globalUboOffset);
		return true;
	}

	bool RendererVulkan::ShaderBindInstance(Shader* shader, const u32 instanceId)
	{
		if (!shader)
		{
			m_logger.Error("ShaderBindInstance() - No valid shader");
			return false;
		}

		const auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);
		const VulkanShaderInstanceState* instanceState = &internal->instanceStates[instanceId];
		shader->boundUboOffset = static_cast<u32>(instanceState->offset);
		return true;
	}

	bool RendererVulkan::ShaderApplyGlobals(Shader* shader)
	{
		const u32 imageIndex = m_context.imageIndex;
		const auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);

		VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[imageIndex].handle;
		VkDescriptorSet globalDescriptor = internal->globalDescriptorSets[imageIndex];

		// Apply UBO first
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = internal->uniformBuffer.handle;
		bufferInfo.offset = shader->globalUboOffset;
		bufferInfo.range  = shader->globalUboStride;

		// Update descriptor sets
		VkWriteDescriptorSet uboWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		uboWrite.dstSet = internal->globalDescriptorSets[imageIndex];
		uboWrite.dstBinding = 0;
		uboWrite.dstArrayElement = 0;
		uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboWrite.descriptorCount = 1;
		uboWrite.pBufferInfo = &bufferInfo;

		VkWriteDescriptorSet descriptorWrites[2];
		descriptorWrites[0] = uboWrite;

		u32 globalSetBindingCount = internal->config.descriptorSets[DESC_SET_INDEX_GLOBAL].bindingCount;
		if (globalSetBindingCount > 1)
		{
			// TODO: There are samplers to be written.
			globalSetBindingCount = 1;
			m_logger.Error("ShaderApplyGlobals() - Global image samplers are not yet supported.");
		}

		vkUpdateDescriptorSets(m_context.device.logicalDevice, globalSetBindingCount, descriptorWrites, 0, nullptr);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internal->pipeline.layout, 0, 1, &globalDescriptor, 0, nullptr);
		return true;
	}

	bool RendererVulkan::ShaderApplyInstance(Shader* shader)
	{
		if (!shader->useInstances)
		{
			m_logger.Error("ShaderApplyInstance() - This shader does not use instances");
			return false;
		}

		auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);
		const u32 imageIndex = m_context.imageIndex;
		VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[imageIndex].handle;

		// Obtain instance data
		VulkanShaderInstanceState* objectState = &internal->instanceStates[shader->boundInstanceId];
		VkDescriptorSet objectDescriptorSet = objectState->descriptorSetState.descriptorSets[imageIndex];

		// TODO: if needs update
		VkWriteDescriptorSet descriptorWrites[2]; // Always a max of 2 descriptor sets
		Memory.Zero(descriptorWrites, sizeof(VkWriteDescriptorSet) * 2);
		u32 descriptorCount = 0;
		u32 descriptorIndex = 0;

		// Descriptor 0 - Uniform buffer
		// Only do this if the descriptor has not yet been updated
		u8* instanceUboGeneration = &objectState->descriptorSetState.descriptorStates[descriptorIndex].generations[imageIndex];
		if (*instanceUboGeneration == INVALID_ID_U8)
		{
			VkDescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = internal->uniformBuffer.handle;
			bufferInfo.offset = objectState->offset;
			bufferInfo.range = shader->uboStride;

			VkWriteDescriptorSet uboDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			uboDescriptor.dstSet = objectDescriptorSet;
			uboDescriptor.dstBinding = descriptorIndex;
			uboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboDescriptor.descriptorCount = 1;
			uboDescriptor.pBufferInfo = &bufferInfo;

			descriptorWrites[descriptorCount] = uboDescriptor;
			descriptorCount++;

			*instanceUboGeneration = 1; // material->generation; // TODO: some generation from... somewhere
		}
		descriptorIndex++;

		// Samplers will always be in the binding. If the binding count is less than 2, there are no samplers
		if (internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindingCount > 1)
		{
			// Iterate samplers.
			u32 totalSamplerCount = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
			u32 updateSamplerCount = 0;
			VkDescriptorImageInfo imageInfos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];
			for (u32 i = 0; i < totalSamplerCount; i++)
			{
				// TODO: only update in the list if actually needing an update
				const Texture* t = internal->instanceStates[shader->boundInstanceId].instanceTextures[i];
				const auto internalData = static_cast<VulkanTextureData*>(t->internalData);
				imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfos[i].imageView = internalData->image.view;
				imageInfos[i].sampler = internalData->sampler;

				// TODO: change up descriptor state to handle this properly.
				// Sync frame generation if not using a default texture.

				updateSamplerCount++;
			}

			VkWriteDescriptorSet samplerDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			samplerDescriptor.dstSet = objectDescriptorSet;
			samplerDescriptor.dstBinding = descriptorIndex;
			samplerDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerDescriptor.descriptorCount = updateSamplerCount;
			samplerDescriptor.pImageInfo = imageInfos;

			descriptorWrites[descriptorCount] = samplerDescriptor;
			descriptorCount++;
		}

		if (descriptorCount > 0)
		{
			vkUpdateDescriptorSets(m_context.device.logicalDevice, descriptorCount, descriptorWrites, 0, nullptr);
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internal->pipeline.layout, 1, 1, &objectDescriptorSet, 0, nullptr);
		return true;
	}

	bool RendererVulkan::AcquireShaderInstanceResources(Shader* shader, u32* outInstanceId)
	{
		const auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);
		// TODO: dynamic
		*outInstanceId = INVALID_ID;
		for (u32 i = 0; i < VULKAN_MAX_MATERIAL_COUNT; i++)
		{
			if (internal->instanceStates[i].id == INVALID_ID)
			{
				internal->instanceStates[i].id = i;
				*outInstanceId = i;
				break;
			}
		}

		if (*outInstanceId == INVALID_ID)
		{
			m_logger.Error("AcquireShaderInstanceResources() - Failed to acquire new id");
			return false;
		}

		VulkanShaderInstanceState* instanceState = &internal->instanceStates[*outInstanceId];
		const u32 instanceTextureCount = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
		// Wipe out the memory for the entire array, even if it isn't all used.
		instanceState->instanceTextures = Memory.Allocate<const Texture*>(shader->instanceTextureCount, MemoryType::Array);
		const Texture* defaultTexture = Textures.GetDefault();
		// Set all the texture pointers to default until assigned
		for (u32 i = 0; i < instanceTextureCount; i++)
		{
			instanceState->instanceTextures[i] = defaultTexture;
		}

		// Allocate some space in the UBO - by the stride, not the size
		const u64 size = shader->uboStride;
		if (!internal->uniformBuffer.Allocate(size, &instanceState->offset))
		{
			m_logger.Error("AcquireShaderInstanceResources() - failed to acquire UBO space.");
			return false;
		}

		VulkanShaderDescriptorSetState* setState = &instanceState->descriptorSetState;

		// Each descriptor binding in the set
		const u32 bindingCount = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindingCount;
		Memory.Zero(setState->descriptorStates, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);
		for (u32 i = 0; i < bindingCount; i++)
		{
			for (u32 j = 0; j < 3; j++)
			{
				setState->descriptorStates[i].generations[j] = INVALID_ID_U8;
				setState->descriptorStates[i].ids[j] = INVALID_ID;
			}
		}

		// Allocate 3 descriptor sets (one per frame)
		const VkDescriptorSetLayout layouts[3] =
		{
			internal->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
			internal->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
			internal->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
		};

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = internal->descriptorPool;
		allocInfo.descriptorSetCount = 3;
		allocInfo.pSetLayouts = layouts;
		const VkResult result = vkAllocateDescriptorSets(m_context.device.logicalDevice, &allocInfo, instanceState->descriptorSetState.descriptorSets);
		if (result != VK_SUCCESS)
		{
			m_logger.Error("AcquireShaderInstanceResources() - Error allocating descriptor sets in shader: '{}'.", VulkanUtils::ResultString(result));
			return false;
		}
		return true;
	}

	bool RendererVulkan::ReleaseShaderInstanceResources(Shader* shader, u32 instanceId)
	{
		const auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);
		VulkanShaderInstanceState* instanceState = &internal->instanceStates[instanceId];

		// Wait for any pending operations using the descriptor set to finish
		vkDeviceWaitIdle(m_context.device.logicalDevice);

		// Free 3 descriptor sets (one per frame)
		const VkResult result = vkFreeDescriptorSets(m_context.device.logicalDevice, internal->descriptorPool, 
		    3, instanceState->descriptorSetState.descriptorSets);

		if (!result == VK_SUCCESS)
		{
			m_logger.Error("ReleaseShaderInstanceResources() - Error while freeing shader descriptor sets.");
		}

		// Destroy descriptor states
		Memory.Zero(instanceState->descriptorSetState.descriptorStates, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);

		// Free the memory for the instance texture pointer array
		if (instanceState->instanceTextures)
		{
			Memory.Free(instanceState->instanceTextures, sizeof(Texture*) * shader->instanceTextureCount, MemoryType::Array);
			instanceState->instanceTextures = nullptr;
		}

		internal->uniformBuffer.Free(shader->uboStride, instanceState->offset);
		instanceState->offset = INVALID_ID;
		instanceState->id = INVALID_ID;

		return true;
	}

	bool RendererVulkan::SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value)
	{
		const auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);
		if (uniform->type == Uniform_Sampler)
		{
			if (uniform->scope == ShaderScope::Global)
			{
				shader->globalTextures[uniform->location] = static_cast<const Texture*>(value);
			}
			else
			{
				internal->instanceStates[shader->boundInstanceId].instanceTextures[uniform->location] = static_cast<const Texture*>(value);
			}
		}
		else
		{
			if (uniform->scope == ShaderScope::Local)
			{
				// Is local, using push constants. Do this immediately.
				VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex].handle;
				vkCmdPushConstants(commandBuffer, internal->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<u32>(uniform->offset), uniform->size, value);
			}
			else
			{
				// Map the appropriate memory location and copy the data over.
				auto address = static_cast<u8*>(internal->mappedUniformBufferBlock);
				address += shader->boundUboOffset + uniform->offset;
				Memory.Copy(address, value, uniform->size);
			}
		}

		return true;
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

	void RendererVulkan::RegenerateFrameBuffers()
	{
		const u32 imageCount = m_context.swapChain.imageCount;
		for (u32 i = 0; i < imageCount; i++)
		{
			const VkImageView worldAttachments[2] = { m_context.swapChain.views[i], m_context.swapChain.depthAttachment.view };
			VkFramebufferCreateInfo frameBufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			frameBufferCreateInfo.renderPass = m_context.mainRenderPass.handle;
			frameBufferCreateInfo.attachmentCount = 2;
			frameBufferCreateInfo.pAttachments = worldAttachments;
			frameBufferCreateInfo.width = m_context.frameBufferWidth;
			frameBufferCreateInfo.height = m_context.frameBufferHeight;
			frameBufferCreateInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(m_context.device.logicalDevice, &frameBufferCreateInfo, m_context.allocator, &m_context.worldFrameBuffers[i]));

			const VkImageView uiAttachments[1] = { m_context.swapChain.views[i] };
			VkFramebufferCreateInfo swapChainFrameBufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			swapChainFrameBufferCreateInfo.renderPass = m_context.uiRenderPass.handle;
			swapChainFrameBufferCreateInfo.attachmentCount = 1;
			swapChainFrameBufferCreateInfo.pAttachments = uiAttachments;
			swapChainFrameBufferCreateInfo.width = m_context.frameBufferWidth;
			swapChainFrameBufferCreateInfo.height = m_context.frameBufferHeight;
			swapChainFrameBufferCreateInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(m_context.device.logicalDevice, &swapChainFrameBufferCreateInfo, m_context.allocator, &m_context.swapChain.frameBuffers[i]))
		}
	}

	bool RendererVulkan::RecreateSwapChain()
	{
		if (m_context.recreatingSwapChain)
		{
			m_logger.Debug("RecreateSwapChain() called when already recreating.");
			return false;
		}

		if (m_context.frameBufferWidth == 0 || m_context.frameBufferHeight == 0)
		{
			m_logger.Debug("RecreateSwapChain() called when at least one of the window dimensions is < 1");
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
			vkDestroyFramebuffer(m_context.device.logicalDevice, m_context.worldFrameBuffers[i], m_context.allocator);
			vkDestroyFramebuffer(m_context.device.logicalDevice, m_context.swapChain.frameBuffers[i], m_context.allocator);
		}

		m_context.mainRenderPass.area.x = 0;
		m_context.mainRenderPass.area.y = 0;
		m_context.mainRenderPass.area.z = static_cast<i32>(m_context.frameBufferWidth);
		m_context.mainRenderPass.area.w = static_cast<i32>(m_context.frameBufferHeight);

		RegenerateFrameBuffers();
		CreateCommandBuffers();

		m_context.recreatingSwapChain = false;
		return true;
	}

	bool RendererVulkan::CreateBuffers()
	{
		constexpr auto baseFlagBits = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		constexpr auto memoryPropertyFlagBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		// Geometry vertex buffer
		constexpr u64 vertexBufferSize = sizeof(Vertex3D) * 1024 * 1024;
		if (!m_objectVertexBuffer.Create(&m_context, vertexBufferSize, baseFlagBits | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryPropertyFlagBits, true, true))
		{
			m_logger.Error("Error creating vertex buffer");
			return false;
		}

		// Geometry index buffer
		constexpr u64 indexBufferSize = sizeof(u32) * 1024 * 1024;
		if (!m_objectIndexBuffer.Create(&m_context, indexBufferSize, baseFlagBits | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryPropertyFlagBits, true, true))
		{
			m_logger.Error("Error creating index buffer");
			return false;
		}

		return true;
	}

	bool RendererVulkan::CreateModule(VulkanShaderStageConfig config, VulkanShaderStage* shaderStage) const
	{
		// Read the resource
		Resource binaryResource{};
		if (!Resources.Load(config.fileName, ResourceType::Binary, &binaryResource))
		{
			m_logger.Error("CreateModule() - Unable to read shader module: '{}'", config.fileName);
			return false;
		}

		shaderStage->createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shaderStage->createInfo.codeSize = binaryResource.dataSize;
		shaderStage->createInfo.pCode = binaryResource.GetData<u32*>();

		VK_CHECK(vkCreateShaderModule(m_context.device.logicalDevice, &shaderStage->createInfo, m_context.allocator, &shaderStage->handle));

		// Release our resource
		Resources.Unload(&binaryResource);

		//Shader stage info
		shaderStage->shaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderStage->shaderStageCreateInfo.stage = config.stage;
		shaderStage->shaderStageCreateInfo.module = shaderStage->handle;
		shaderStage->shaderStageCreateInfo.pName = "main"; // TODO: make this configurable?

		return true;
	}

	bool RendererVulkan::UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer* buffer, u64* outOffset, const u64 size, const void* data) const
	{
		if (!buffer || !buffer->Allocate(size, outOffset))
		{
			m_logger.Error("UploadDataRange() failed to allocate from the provided buffer");
			return false;
		}

		constexpr VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		VulkanBuffer staging;
		staging.Create(&m_context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, false);

		staging.LoadData(&m_context, 0, size, 0, data);
		staging.CopyTo(&m_context, pool, fence, queue, 0, buffer->handle, *outOffset, size);

		staging.Destroy(&m_context);

		return true;
	}

	void RendererVulkan::FreeDataRange(VulkanBuffer* buffer, const u64 offset, const u64 size)
	{
		if (buffer) buffer->Free(size, offset);
	}
}
