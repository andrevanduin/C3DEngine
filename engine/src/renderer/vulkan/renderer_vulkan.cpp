
#include "renderer_vulkan.h"

#include <VkBootstrap.h>
#include <SDL2/SDL_vulkan.h>

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

#include "core/logger.h"
#include "core/application.h"
#include "core/memory.h"

#include "resources/texture.h"

#include "renderer/vertex.h"
#include "services/services.h"

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
		m_debugMessenger = vkbInstance.debug_messenger;

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

		// Create our builtin shader
		if (!m_materialShader.Create(&m_context))
		{
			m_logger.Error("Loading built-in material shader failed");
			return false;
		}
		if (!m_uiShader.Create(&m_context))
		{
			m_logger.Error("Loading built-in ui shader failed");
			return false;
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

	void RendererVulkan::OnResize(const u16 width, const u16 height)
	{
		m_context.cachedFrameBufferWidth = width;
		m_context.cachedFrameBufferHeight = height;
		m_context.frameBufferSizeGeneration++;

		m_logger.Info("OnResize() w/h/gen {}/{}/{}", width, height, m_context.frameBufferSizeGeneration);
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

		m_context.mainRenderPass.area.z = static_cast<i32>(m_context.frameBufferWidth);
		m_context.mainRenderPass.area.w = static_cast<i32>(m_context.frameBufferHeight);

		return true;
	}

	void RendererVulkan::UpdateGlobalWorldState(const mat4 projection, const mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode)
	{
		m_materialShader.Use(&m_context);

		m_materialShader.globalUbo.projection = projection;
		m_materialShader.globalUbo.view = view;
		// TODO: other ubo properties here

		m_materialShader.UpdateGlobalState(&m_context, m_context.frameDeltaTime);
	}

	void RendererVulkan::UpdateGlobalUiState(const mat4 projection, const mat4 view, i32 mode)
	{
		m_uiShader.Use(&m_context);

		m_uiShader.globalUbo.projection = projection;
		m_uiShader.globalUbo.view = view;

		// TODO: other ubo properties

		m_uiShader.UpdateGlobalState(&m_context, m_context.frameDeltaTime);
;	}

	void RendererVulkan::DrawGeometry(const GeometryRenderData data)
	{
		if (!data.geometry || data.geometry->internalId == INVALID_ID) return;

		const auto bufferData = &m_geometries[data.geometry->internalId];
		const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

		switch (Material* m = data.geometry->material ? data.geometry->material : Materials.GetDefault(); m->type)
		{
			case MaterialType::World:
				m_materialShader.SetModel(&m_context, data.model);
				m_materialShader.ApplyMaterial(&m_context, m);
				break;
			case MaterialType::Ui:
				m_uiShader.SetModel(&m_context, data.model);
				m_uiShader.ApplyMaterial(&m_context, m);
				break;
			default:
				m_logger.Error("DrawGeometry() - unknown material type {}", static_cast<u8>(m->type));
				break;
		}
		
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

		VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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

		renderPass->Begin(commandBuffer, frameBuffer);

		// Use the appropriate Shader
		switch (renderPassId)
		{
			case BuiltinRenderPass::World:
				m_materialShader.Use(&m_context);
				break;
			case BuiltinRenderPass::Ui:
				m_uiShader.Use(&m_context);
				break;
		}

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

		renderPass->End(commandBuffer);
		return true;
	}

	void RendererVulkan::Shutdown()
	{
		m_logger.Info("Shutting Down");

		vkDeviceWaitIdle(m_context.device.logicalDevice);

		// Destroy stuff in opposite order of creation
		m_objectVertexBuffer.Destroy(&m_context);
		m_objectIndexBuffer.Destroy(&m_context);

		// Destroy builtin shaders
		m_uiShader.Destroy(&m_context);
		m_materialShader.Destroy(&m_context);

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

		m_context.swapChain.Destroy(&m_context);

		m_context.device.Destroy(&m_context);

		vkDestroySurfaceKHR(m_context.instance, m_context.surface, m_context.allocator);

		vkb::destroy_debug_utils_messenger(m_context.instance, m_debugMessenger);

		vkDestroyInstance(m_context.instance, m_context.allocator);
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
			m_logger.Error("Error creating texture sampler: {}", VulkanUtils::ResultString(result, true));
			return;
		}

		texture->generation++;
	}

	bool RendererVulkan::CreateMaterial(Material* material)
	{
		if (material)
		{
			switch (material->type)
			{
				case MaterialType::World:
					if (!m_materialShader.AcquireResources(&m_context, material))
					{
						m_logger.Error("CreateMaterial() failed to acquire world shader resources");
						return false;
					}
					break;
				case MaterialType::Ui:
					if (!m_uiShader.AcquireResources(&m_context, material))
					{
						m_logger.Error("CreateMaterial() failed to acquire UI shader resources");
						return false;
					}
					break;
				default:
					m_logger.Error("CreateMaterial() unknown material type");
					break;
			}

			m_logger.Trace("Material Created");
			return true;
		}

		m_logger.Error("CreateMaterial() called with nullptr. Creation failed");
		return false;
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
			FreeDataRange(&m_objectVertexBuffer, oldRange.vertexBufferOffset, oldRange.vertexElementSize * oldRange.vertexCount);

			// Free index data, if applicable
			if (oldRange.indexElementSize > 0)
			{
				FreeDataRange(&m_objectIndexBuffer, oldRange.indexBufferOffset, oldRange.indexElementSize * oldRange.indexCount);
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
			Memory.Zero(&data->image, sizeof(VulkanImage));

			vkDestroySampler(m_context.device.logicalDevice, data->sampler, m_context.allocator);
			data->sampler = nullptr;

			Memory.Free(texture->internalData, sizeof(VulkanTextureData), MemoryType::Texture);
		}
		
		Memory.Zero(texture, sizeof(Texture));
	}

	void RendererVulkan::DestroyMaterial(Material* material)
	{
		if (material)
		{
			if (material->internalId != INVALID_ID)
			{
				switch (material->type)
				{
					case MaterialType::World:
						m_materialShader.ReleaseResources(&m_context, material);
						break;
					case MaterialType::Ui:
						m_uiShader.ReleaseResources(&m_context, material);
						break;
					default:
						m_logger.Error("CreateMaterial() unknown material type");
						break;
				}
			}
			else
			{
				m_logger.Warn("DestroyMaterial() called with internalId = INVALID_ID. Ignoring request");
			}
		}
		else
		{
			m_logger.Warn("DestroyMaterial() called with nullptr. Ignoring request");
		}
	}

	void RendererVulkan::DestroyGeometry(Geometry* geometry)
	{
		if (geometry && geometry->internalId != INVALID_ID)
		{
			vkDeviceWaitIdle(m_context.device.logicalDevice);

			VulkanGeometryData* internalData = &m_geometries[geometry->internalId];

			// Free vertex data
			FreeDataRange(&m_objectVertexBuffer, internalData->vertexBufferOffset, internalData->vertexElementSize * internalData->vertexCount);

			// Free index data, if applicable
			if (internalData->indexElementSize > 0)
			{
				FreeDataRange(&m_objectIndexBuffer, internalData->indexBufferOffset, internalData->indexElementSize * internalData->indexCount);
			}

			// Clean up data
			Memory.Zero(internalData, sizeof(VulkanGeometryData));
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
			m_logger.Debug("RecreateSwapChain called when already recreating.");
			return false;
		}

		if (m_context.frameBufferWidth == 0 || m_context.frameBufferHeight == 0)
		{
			m_logger.Debug("RecreateSwapChain called when at least one of the window dimensions is < 1");
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
		if (!m_objectVertexBuffer.Create(&m_context, vertexBufferSize, baseFlagBits | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryPropertyFlagBits, true))
		{
			m_logger.Error("Error creating vertex buffer");
			return false;
		}

		// Geometry index buffer
		constexpr u64 indexBufferSize = sizeof(u32) * 1024 * 1024;
		if (!m_objectIndexBuffer.Create(&m_context, indexBufferSize, baseFlagBits | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryPropertyFlagBits, true))
		{
			m_logger.Error("Error creating index buffer");
			return false;
		}

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
		staging.Create(&m_context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true);

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
