
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
#include "renderer/renderer_frontend.h"

#include "resources/texture.h"
#include "resources/shader.h"

#include "renderer/vertex.h"
#include "resources/loaders/binary_loader.h"

#include "services/services.h"
#include "systems/resource_system.h"
#include "systems/texture_system.h"

#ifndef C3D_VULKAN_ALLOCATOR_TRACE
#undef C3D_VULKAN_ALLOCATOR_TRACE
#endif

#ifndef C3D_VULKAN_USE_CUSTOM_ALLOCATOR
#define C3D_VULKAN_USE_CUSTOM_ALLOCATOR 1
#endif

namespace C3D
{
#ifdef C3D_VULKAN_USE_CUSTOM_ALLOCATOR
	/*
	 * @brief Implementation of PFN_vkAllocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkAllocationFunction.html
	 */
	void* VulkanAllocatorAllocate(void* userData, const size_t size, const size_t alignment, const VkSystemAllocationScope allocationScope)
	{
		// The spec states that we should return nullptr if size == 0
		if (size == 0) return nullptr;

		void* result = Memory.AllocateAligned(size, static_cast<u16>(alignment), MemoryType::Vulkan);
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
		Logger::Trace("[VULKAN_ALLOCATE] - {} (Size = {}B, Alignment = {}).", fmt::ptr(result), size, alignment);
#endif

		return result;
	}

	/*
	 * @brief Implementation of PFN_vkFreeFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkFreeFunction.html
	 */
	void VulkanAllocatorFree(void* userData, void* memory)
	{
		if (!memory)
		{
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
			Logger::Trace("[VULKAN_FREE] - Block was null. Nothing to free.");
#endif
			return;
		}

		u64 size;
		u16 alignment;
		if (Memory.GetSizeAlignment(memory, &size, &alignment))
		{
			Memory.FreeAligned(memory, size, MemoryType::Vulkan);
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
			Logger::Trace("[VULKAN_FREE] - Block at: {} was Freed.", fmt::ptr(memory));
#endif
		}
		else
		{
			Logger::Error("[VULKAN_FREE] - Failed to get alignment lookup for block: {}.", fmt::ptr(memory));
		}
	}

	/*
	 * @brief Implementation of PFN_vkReallocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkReallocationFunction.html
	 */
	void* VulkanAllocatorReallocate(void* userData, void* original, const size_t size, const size_t alignment, const VkSystemAllocationScope allocationScope)
	{
		// The spec states that we should do an simple allocation if original is nullptr
		if (!original)
		{
			return VulkanAllocatorAllocate(userData, size, alignment, allocationScope);
		}

		// The spec states that we should return nullptr if size == 0
		if (size == 0) return nullptr;

		u64 allocSize;
		u16 allocAlignment;
		if (!Memory.GetSizeAlignment(original, &allocSize, &allocAlignment))
		{
			Logger::Error("[VULKAN_REALLOCATE] - Tried to do a reallocation of an unaligned block: {}.", fmt::ptr(original));
			return nullptr;
		}

		// The spec states that the alignment provided should not differ from the original memory's alignment.
		if (alignment != allocAlignment)
		{
			Logger::Error("[VULKAN_REALLOCATE] - Attempted to do a reallocation with a different alignment of: {}. Original alignment was: {}.", alignment, allocAlignment);
			return nullptr;
		}

#ifdef C3D_VULKAN_ALLOCATOR_TRACE
		Logger::Trace("[VULKAN_REALLOCATE] - Reallocating block: {}", fmt::ptr(original));
#endif

		void* result = VulkanAllocatorAllocate(userData, size, alignment, allocationScope);
		if (result)
		{
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
			Logger::Trace("[VULKAN_REALLOCATE] - Successfully reallocated to: {}. Copying data.", fmt::ptr(result));
#endif
			Memory.Copy(result, original, allocSize);
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
			Logger::Trace("[VULKAN_REALLOCATE] - Freeing original block: {}.", fmt::ptr(original));
#endif
			Memory.FreeAligned(original, allocSize, MemoryType::Vulkan);
		}
		else
		{
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
			Logger::Trace("[VULKAN_REALLOCATE] - Failed to Reallocate: {}.", fmt::ptr(original));
#endif
		}

		return result;
	}

	/*
	 * @brief Implementation of PFN_vkReallocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
	 */
	void VulkanAllocatorInternalAllocation(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
	{
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
		Logger::Trace("[VULKAN_EXTERNAL_ALLOCATE] - Allocation of size {}.", size);
#endif
		Memory.AllocateReport(size, MemoryType::VulkanExternal);
	}

	/*
	 * @brief Implementation of PFN_vkReallocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
	 */
	void VulkanAllocatorInternalFree(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
	{
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
		Logger::Trace("[VULKAN_EXTERNAL_FREE] - Free of size {}.", size);
#endif
		Memory.FreeReport(size, MemoryType::VulkanExternal);
	}

	bool CreateVulkanAllocator(VkAllocationCallbacks* callbacks)
	{
		if (callbacks)
		{
			callbacks->pfnAllocation = VulkanAllocatorAllocate;
			callbacks->pfnReallocation = VulkanAllocatorReallocate;
			callbacks->pfnFree = VulkanAllocatorFree;
			callbacks->pfnInternalAllocation = VulkanAllocatorInternalAllocation;
			callbacks->pfnInternalFree = VulkanAllocatorInternalFree;
			callbacks->pUserData = nullptr;
			return true;
		}
		return false;
	}
#endif

	RendererVulkan::RendererVulkan()
		: RendererBackend("VULKAN_RENDERER"), m_context(), m_objectVertexBuffer(&m_context), m_objectIndexBuffer(&m_context), m_geometries{}
	{}

	bool RendererVulkan::Init(const RendererBackendConfig& config, u8* outWindowRenderTargetCount)
	{
		type = RendererBackendType::Vulkan;

#if C3D_VULKAN_USE_CUSTOM_ALLOCATOR == 1
		m_context.allocator = Memory.Allocate<VkAllocationCallbacks>(MemoryType::RenderSystem);
		if (!CreateVulkanAllocator(m_context.allocator))
		{
			m_logger.Error("Failed to initialize requested CustomVulkanAllocator");
			return false;
		}
#else
		m_context = nullptr;
#endif

		m_context.frontend = config.frontend;
		m_context.frameBufferWidth = 1280;
		m_context.frameBufferHeight = 720;

		vkb::InstanceBuilder instanceBuilder;

		auto vkbInstanceResult = instanceBuilder
			.set_app_name(config.applicationName)
			.set_engine_name("C3DEngine")
			.set_app_version(config.applicationVersion)
			.set_engine_version(VK_MAKE_VERSION(0, 2, 0))
		#if defined(_DEBUG)
			.request_validation_layers(true)
			.set_debug_callback(Logger::VkDebugLog)
		#endif
			.set_allocation_callbacks(m_context.allocator)
			.require_api_version(1, 2)
			.build();

		m_logger.Info("Instance Initialized");
		const vkb::Instance vkbInstance = vkbInstanceResult.value();

		m_context.instance = vkbInstance.instance;
#if defined(_DEBUG)
		m_debugMessenger = vkbInstance.debug_messenger;
#endif

		// TODO: Implement multiThreading
		m_context.multiThreadingEnabled = false;

		if (!SDL_Vulkan_CreateSurface(config.window, m_context.instance, &m_context.surface))
		{
			m_logger.Error("Init() - Failed to create Vulkan Surface.");
			return false;
		}

		m_logger.Info("SDL Surface Initialized");
		if (!m_context.device.Create(vkbInstance, &m_context, m_context.allocator))
		{
			m_logger.Error("Init() - Failed to create Vulkan Device.");
			return false;
		}

		m_context.swapChain.Create(&m_context, m_context.frameBufferWidth, m_context.frameBufferHeight);

		// Save the number of images we have as a the number of render targets required
		*outWindowRenderTargetCount = static_cast<u8>(m_context.swapChain.imageCount);

		// Invalidate all render passes
		for (auto& pass : m_context.registeredRenderPasses)
		{
			pass.id = INVALID_ID_U16;
		}
		// Fill the hashtable with invalid values
		m_context.renderPassTable.Create(VULKAN_MAX_REGISTERED_RENDER_PASSES);
		m_context.renderPassTable.Fill(INVALID_ID);

		// RenderPasses TODO: Move to a function
		for (u32 i = 0; i < config.renderPassCount; i++)
		{
			const auto passConfig = config.passConfigs[i];

			// Ensure there are no collisions with the name
			u32 id = m_context.renderPassTable.Get(passConfig.name);
			if (id != INVALID_ID)
			{
				m_logger.Error("Init() - Collision with RenderPass named '{}'. Initialization failed.", passConfig.name);
				return false;
			}

			// Get a new id for the renderPass
			for (u32 j = 0; j < VULKAN_MAX_REGISTERED_RENDER_PASSES; j++)
			{
				if (m_context.registeredRenderPasses[j].id == INVALID_ID_U16)
				{
					m_context.registeredRenderPasses[j].id = j;
					id = j;
					break;
				}
			}

			// Verify that we found a valid id
			if (id == INVALID_ID)
			{
				m_logger.Error("Init() - No space was found for a new RenderPass: '{}'. Increase VULKAN_MAX_REGISTERED_RENDER_PASSES.", passConfig.name);
				return false;
			}

			// Setup pass
			m_context.registeredRenderPasses[id] = VulkanRenderPass(&m_context, static_cast<u16>(id), passConfig);
			m_context.registeredRenderPasses[id].Create(1.0f, 0);

			// Save the id as an entry in our hash table
			m_context.renderPassTable.Set(passConfig.name, &id);

			m_logger.Info("Init() - Successfully created RenderPass '{}'", passConfig.name);
		}

		CreateCommandBuffers();
		m_logger.Info("Init() - Command Buffers Initialized");

		m_context.imageAvailableSemaphores.resize(m_context.swapChain.maxFramesInFlight);
		m_context.queueCompleteSemaphores.resize(m_context.swapChain.maxFramesInFlight);

		m_logger.Info("Init() - Creating Semaphores and Fences");
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

		// Create and bind our buffers
		constexpr u64 vertexBufferSize = sizeof(Vertex3D) * 1024 * 1024;
		if (!m_objectVertexBuffer.Create(RenderBufferType::Vertex, vertexBufferSize, true))
		{
			m_logger.Error("Init() - Error creating vertex buffer.");
			return false;
		}
		m_objectVertexBuffer.Bind(0);

		constexpr u64 indexBufferSize = sizeof(u32) * 1024 * 1024;
		if (!m_objectIndexBuffer.Create(RenderBufferType::Index, indexBufferSize, true))
		{
			m_logger.Error("Init() - Error creating index buffer.");
			return false;
		}
		m_objectIndexBuffer.Bind(0);

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
		m_logger.Info("Shutdown()");

		// Wait for our device to be finished with it's current frame
		m_context.device.WaitIdle();

		// Destroy stuff in opposite order of creation
		m_objectVertexBuffer.Destroy();
		m_objectIndexBuffer.Destroy();

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

		m_logger.Info("Destroying RenderPasses");
		for (auto& pass : m_context.registeredRenderPasses)
		{
			if (pass.id != INVALID_ID_U16)
			{
				pass.Destroy();
			}
		}
		m_context.renderPassTable.Destroy();

		m_logger.Info("Destroying SwapChain");
		m_context.swapChain.Destroy(&m_context);

		m_logger.Info("Destroying Device");
		m_context.device.Destroy(&m_context);

		// TODO: This should use our custom allocator
		m_logger.Info("Destroying Vulkan Surface");
		vkDestroySurfaceKHR(m_context.instance, m_context.surface, nullptr);

#ifdef _DEBUG
		m_logger.Info("Destroying Vulkan debug messenger");
		vkb::destroy_debug_utils_messenger(m_context.instance, m_debugMessenger, m_context.allocator);
#endif

		m_logger.Info("Destroying Instance");
		vkDestroyInstance(m_context.instance, m_context.allocator);

#if C3D_VULKAN_USE_CUSTOM_ALLOCATOR == 1
		if (m_context.allocator)
		{
			Memory.Free(m_context.allocator, sizeof(VkAllocationCallbacks), MemoryType::RenderSystem);
			m_context.allocator = nullptr;
		}
#endif
	}

	void RendererVulkan::OnResize(const u16 width, const u16 height)
	{
		m_context.frameBufferWidth = width;
		m_context.frameBufferHeight = height;
		m_context.frameBufferSizeGeneration++;

		m_logger.Info("OnResize() - Width: {}, Height: {} and Generation: {}.", width, height, m_context.frameBufferSizeGeneration);
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
		m_context.swapChain.Present(&m_context, m_context.device.presentQueue, 
			m_context.queueCompleteSemaphores[m_context.currentFrame], m_context.imageIndex);

		return true;
	}

	RenderPass* RendererVulkan::GetRenderPass(const char* name)
	{
		if (!name || name[0] == '\0')
		{
			m_logger.Error("GetRenderPass() - Requires a name. Nullptr will be returned");
			return nullptr;
		}
		const u32 id = m_context.renderPassTable.Get(name);
		if (id == INVALID_ID)
		{
			m_logger.Warn("GetRenderPass() - There is not registered RenderPass with name: '{}'", name);
			return nullptr;
		}
		return &m_context.registeredRenderPasses[id];
	}

	void RendererVulkan::CreateRenderTarget(const u8 attachmentCount, Texture** attachments, RenderPass* pass, const u32 width, const u32 height, RenderTarget* outTarget)
	{
		VkImageView attachmentViews[32];
		for (u32 i = 0; i < attachmentCount; i++)
		{
			attachmentViews[i] = static_cast<VulkanImage*>(attachments[i]->internalData)->view;
		}

		// Take a copy of the attachments and count
		outTarget->attachmentCount = attachmentCount;
		if (!outTarget->attachments)
		{
			outTarget->attachments = Memory.Allocate<Texture*>(attachmentCount, MemoryType::Array);
		}
		Memory.Copy(outTarget->attachments, attachments, sizeof(Texture*) * attachmentCount);

		// Setup our frameBuffer creation
		VkFramebufferCreateInfo frameBufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		frameBufferCreateInfo.renderPass = dynamic_cast<VulkanRenderPass*>(pass)->handle;
		frameBufferCreateInfo.attachmentCount = attachmentCount;
		frameBufferCreateInfo.pAttachments = attachmentViews;
		frameBufferCreateInfo.width = width;
		frameBufferCreateInfo.height = height;
		frameBufferCreateInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(m_context.device.logicalDevice, &frameBufferCreateInfo, m_context.allocator, reinterpret_cast<VkFramebuffer*>(&outTarget->internalFrameBuffer)));
	}

	void RendererVulkan::DestroyRenderTarget(RenderTarget* target, const bool freeInternalMemory)
	{
		if (target && target->internalFrameBuffer)
		{
			vkDestroyFramebuffer(m_context.device.logicalDevice, static_cast<VkFramebuffer>(target->internalFrameBuffer), m_context.allocator);
			target->internalFrameBuffer = nullptr;
			if (freeInternalMemory)
			{
				Memory.Free(target->attachments, sizeof(Texture*) * target->attachmentCount, MemoryType::Array);
				target->attachmentCount = 0;
				target->attachments = nullptr;
			}
		}
	}

	RenderBuffer* RendererVulkan::CreateRenderBuffer(const RenderBufferType bufferType, const u64 totalSize, const bool useFreelist)
	{
		const auto buffer = Memory.New<VulkanBuffer>(MemoryType::RenderSystem, &m_context);
		buffer->Create(bufferType, totalSize, useFreelist);
		return buffer;
	}

	bool RendererVulkan::DestroyRenderBuffer(RenderBuffer* buffer)
	{
		buffer->Destroy();
		Memory.FreeAligned(buffer, sizeof(VulkanBuffer), MemoryType::RenderSystem);
		return true;
	}

	Texture* RendererVulkan::GetWindowAttachment(const u8 index)
	{
		if (index >= m_context.swapChain.imageCount)
		{
			m_logger.Fatal("GetWindowAttachment() - Attempting to get attachment index that is out of range: '{}'. Attachment count is: '{}'", index, m_context.swapChain.imageCount);
			return nullptr;
		}
		return m_context.swapChain.renderTextures[index];
	}

	Texture* RendererVulkan::GetDepthAttachment()
	{
		return m_context.swapChain.depthTexture;
	}

	u8 RendererVulkan::GetWindowAttachmentIndex()
	{
		return static_cast<u8>(m_context.imageIndex);
	}

	bool RendererVulkan::IsMultiThreaded() const
	{
		return m_context.multiThreadingEnabled;
	}

	bool RendererVulkan::BeginRenderPass(RenderPass* pass, RenderTarget* target)
	{
		VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
		const auto vulkanPass = dynamic_cast<VulkanRenderPass*>(pass);
		vulkanPass->Begin(commandBuffer, target);
		return true;
	}

	bool RendererVulkan::EndRenderPass(RenderPass* pass)
	{
		VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
		VulkanRenderPass::End(commandBuffer);
		return true;
	}

	void RendererVulkan::DrawGeometry(const GeometryRenderData& data)
	{
		// Simply ignore if there is no geometry to draw
		if (!data.geometry || data.geometry->internalId == INVALID_ID) return;

		const auto bufferData = &m_geometries[data.geometry->internalId];
		const bool includesIndexData = bufferData->indexCount > 0;

		if (!m_objectVertexBuffer.Draw(bufferData->vertexBufferOffset, bufferData->vertexCount, includesIndexData))
		{
			m_logger.Error("DrawGeometry() - Failed to draw vertex buffer.");
			return;
		}

		if (includesIndexData)
		{
			if (!m_objectIndexBuffer.Draw(bufferData->indexBufferOffset, bufferData->indexCount, false))
			{
				m_logger.Error("DrawGeometry() - Failed to draw index buffer.");
			}
		}
	}

	void RendererVulkan::CreateTexture(const u8* pixels, Texture* texture)
	{
		// Internal data creation
		texture->internalData = Memory.Allocate<VulkanImage>(MemoryType::Texture);

		const auto image = static_cast<VulkanImage*>(texture->internalData);
		const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texture->width) * texture->height
			* texture->channelCount * (texture->type == TextureType::TypeCube ? 6 : 1);

		constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM; // NOTE: Assumes 8 bits per channel

		image->Create(&m_context, texture->type, texture->width, texture->height, imageFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT);

		// Load the data
		WriteDataToTexture(texture, 0, static_cast<u32>(imageSize), pixels);
		texture->generation++;
	}

	VkFormat ChannelCountToFormat(const u8 channelCount, const VkFormat defaultFormat)
	{
		switch (channelCount)
		{
			case 1:
				return VK_FORMAT_R8_UNORM;
			case 2:
				return VK_FORMAT_R8G8_UNORM;
			case 3:
				return VK_FORMAT_R8G8B8_UNORM;
			case 4:
				return VK_FORMAT_R8G8B8A8_UNORM;
			default:
				return defaultFormat;
		}
	}

	void RendererVulkan::CreateWritableTexture(Texture* texture)
	{
		texture->internalData = Memory.Allocate<VulkanImage>(MemoryType::Texture);
		const auto image = static_cast<VulkanImage*>(texture->internalData);

		const VkFormat imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);
		// TODO: Lot's of assumptions here
		image->Create(&m_context, texture->type, texture->width, texture->height, imageFormat, VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT);

		texture->generation++;
	}

	void RendererVulkan::WriteDataToTexture(Texture* texture, u32 offset, const u32 size, const u8* pixels)
	{
		const auto image = static_cast<VulkanImage*>(texture->internalData);
		const VkFormat imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);

		// Create a staging buffer and load data into it.
		VulkanBuffer staging(&m_context);
		staging.Create(RenderBufferType::Staging, size, false);
		staging.Bind(0);

		// Load the data into our staging buffer
		staging.LoadRange(0, size, pixels);

		VulkanCommandBuffer tempBuffer;
		VkCommandPool pool = m_context.device.graphicsCommandPool;
		VkQueue queue = m_context.device.graphicsQueue;

		tempBuffer.AllocateAndBeginSingleUse(&m_context, pool);

		// Transition the layout from whatever it is currently to optimal for receiving data.
		image->TransitionLayout(&m_context, &tempBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Copy the data from the buffer.
		image->CopyFromBuffer(texture->type, staging.handle, &tempBuffer);

		// Transition from optimal for receiving data to shader-read-only optimal layout.
		image->TransitionLayout(&m_context, &tempBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		tempBuffer.EndSingleUse(&m_context, pool, queue);

		staging.Unbind();
		staging.Destroy();
		texture->generation++;
	}

	void RendererVulkan::ResizeTexture(Texture* texture, const u32 newWidth, const u32 newHeight)
	{
		if (texture && texture->internalData)
		{
			const auto image = static_cast<VulkanImage*>(texture->internalData);
			image->Destroy(&m_context);

			const VkFormat imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);

			// TODO: Lot's of assumptions here
			image->Create(&m_context, texture->type, newWidth, newHeight, imageFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT);

			texture->generation++;
		}
	}

	void RendererVulkan::DestroyTexture(Texture* texture)
	{
		vkDeviceWaitIdle(m_context.device.logicalDevice);

		const auto data = static_cast<VulkanTextureData*>(texture->internalData);
		if (data)
		{
			data->image.Destroy(&m_context);
			Memory.Zero(&data->image, sizeof(VulkanImage));
			Memory.Free(texture->internalData, sizeof(VulkanTextureData), MemoryType::Texture);
		}
		
		Memory.Zero(texture, sizeof(Texture));
	}

	bool RendererVulkan::CreateGeometry(Geometry* geometry, const u32 vertexSize, const u64 vertexCount, const void* vertices,
	                                    const u32 indexSize, const u64 indexCount, const void* indices)
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
			m_logger.Fatal("CreateGeometry() failed to find a free index for a new geometry upload. Adjust the config to allow for more.");
			return false;
		}

		// Vertex data
		internalData->vertexCount = static_cast<u32>(vertexCount);
		internalData->vertexElementSize = sizeof(Vertex3D);
		u64 totalSize = vertexCount * vertexSize;

		if (!m_objectVertexBuffer.Allocate(totalSize, &internalData->vertexBufferOffset))
		{
			m_logger.Error("CreateGeometry() - Failed to allocate from the vertex buffer.");
			return false;
		}

		if (!m_objectVertexBuffer.LoadRange(internalData->vertexBufferOffset, totalSize, vertices))
		{
			m_logger.Error("CreateGeometry() - Failed to upload vertices to the vertex buffer.");
			return false;
		}

		// Index data, if applicable
		if (indexCount && indices)
		{
			internalData->indexCount = static_cast<u32>(indexCount);
			internalData->indexElementSize = sizeof(u32);
			totalSize = indexCount * indexSize;

			if (!m_objectIndexBuffer.Allocate(totalSize, &internalData->indexBufferOffset))
			{
				m_logger.Error("CreateGeometry() - Failed to allocate from the index buffer.");
				return false;
			}

			if (!m_objectIndexBuffer.LoadRange(internalData->indexBufferOffset, totalSize, indices))
			{
				m_logger.Error("CreateGeometry() - Failed to upload indices to the index buffer.");
				return false;
			}
		}

		if (internalData->generation == INVALID_ID) internalData->generation = 0;
		else internalData->generation++;

		if (isReupload)
		{
			// Free vertex data
			m_objectVertexBuffer.Free(static_cast<u64>(oldRange.vertexElementSize) * oldRange.vertexCount, oldRange.vertexBufferOffset);

			// Free index data, if applicable
			if (oldRange.indexElementSize > 0)
			{
				m_objectIndexBuffer.Free(static_cast<u64>(oldRange.indexElementSize) * oldRange.indexCount, oldRange.indexBufferOffset);
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
			m_objectVertexBuffer.Free(static_cast<u64>(internalData->vertexElementSize) * internalData->vertexCount, internalData->vertexBufferOffset);

			// Free index data, if applicable
			if (internalData->indexElementSize > 0)
			{
				m_objectIndexBuffer.Free(static_cast<u64>(internalData->indexElementSize) * internalData->indexCount, internalData->indexBufferOffset);
			}

			// Clean up data
			Memory.Zero(internalData, sizeof(VulkanGeometryData));
			internalData->id = INVALID_ID;
			internalData->generation = INVALID_ID;
		}
	}

	bool RendererVulkan::CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const
	{
		// Allocate enough memory for the 
		shader->apiSpecificData = Memory.New<VulkanShader>(MemoryType::Shader, &m_context);

		// Translate stages
		VkShaderStageFlags vulkanStages[VULKAN_SHADER_MAX_STAGES]{};
		for (auto i = 0; i < config.stages.size(); i++)
		{
			switch (config.stages[i]) {
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
		vulkanShader->renderPass = dynamic_cast<VulkanRenderPass*>(pass);
		vulkanShader->config.maxDescriptorSetCount = maxDescriptorAllocateCount;

		vulkanShader->config.stageCount = 0;
		for (u32 i = 0; i < config.stageFileNames.size(); i++)
		{
			// Make sure we have enough room left for this stage
			if (vulkanShader->config.stageCount + 1 > VULKAN_SHADER_MAX_STAGES)
			{
				m_logger.Error("CreateShader() - Shaders may have a maximum of {} stages.", VULKAN_SHADER_MAX_STAGES);
				return false;
			}

			// Check if we support this stage
			VkShaderStageFlagBits stageFlag;
			switch (config.stages[i])
			{
				case ShaderStage::Vertex:
					stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
					break;
				case ShaderStage::Fragment:
					stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
					break;
				default:
					m_logger.Error("CreateShader() - Unsupported shader stage {}. Stage ignored.", ToUnderlying(config.stages[i]));
					continue;
			}

			// Set the stage and increment the stage count
			const auto stageIndex = vulkanShader->config.stageCount;
			vulkanShader->config.stages[stageIndex].stage = stageFlag;
			StringNCopy(vulkanShader->config.stages[stageIndex].fileName, config.stageFileNames[i], VULKAN_SHADER_STAGE_CONFIG_FILENAME_MAX_LENGTH);
			vulkanShader->config.stageCount++;
		}

		// Zero out arrays and counts
		Memory.Zero(vulkanShader->config.descriptorSets, sizeof(VulkanDescriptorSetConfig) * 2);
		vulkanShader->config.descriptorSets[0].samplerBindingIndex = INVALID_ID_U8;
		vulkanShader->config.descriptorSets[1].samplerBindingIndex = INVALID_ID_U8;

		// Zero out attribute arrays
		Memory.Zero(vulkanShader->config.attributes, sizeof(VkVertexInputAttributeDescription) * VULKAN_SHADER_MAX_ATTRIBUTES);

		// Get the uniform counts
		vulkanShader->ZeroOutCounts();
		for (auto& uniform : config.uniforms)
		{
			switch (uniform.scope)
			{
			case ShaderScope::Global:
				if (uniform.type == ShaderUniformType::Uniform_Sampler) vulkanShader->globalUniformSamplerCount++;
				else vulkanShader->globalUniformCount++;
				break;
			case ShaderScope::Instance:
				if (uniform.type == ShaderUniformType::Uniform_Sampler) vulkanShader->instanceUniformSamplerCount++;
				else vulkanShader->instanceUniformCount++;
				break;
			case ShaderScope::Local:
				vulkanShader->localUniformCount++;
				break;
			}
		}

		// TODO: For now, shaders will only ever have these 2 types of descriptor pools
		vulkanShader->config.poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 };			// HACK: max number of ubo descriptor sets
		vulkanShader->config.poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 };	// HACK: max number of image sampler descriptor sets

		// Global descriptor set config
		if (vulkanShader->globalUniformCount > 0 || vulkanShader->globalUniformSamplerCount > 0)
		{
			auto& setConfig = vulkanShader->config.descriptorSets[vulkanShader->config.descriptorSetCount];

			// Global UBO binding is first, if present
			if (vulkanShader->globalUniformCount > 0)
			{
				u8 bindingIndex = setConfig.bindingCount;
				setConfig.bindings[bindingIndex].binding = bindingIndex;
				setConfig.bindings[bindingIndex].descriptorCount = 1;
				setConfig.bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				setConfig.bindings[bindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				setConfig.bindingCount++;
			}

			// Add a binding for samplers if we are using them
			if (vulkanShader->globalUniformSamplerCount > 0)
			{
				u8 bindingIndex = setConfig.bindingCount;
				setConfig.bindings[bindingIndex].binding = bindingIndex;
				setConfig.bindings[bindingIndex].descriptorCount = vulkanShader->globalUniformSamplerCount;
				setConfig.bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				setConfig.bindings[bindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				setConfig.samplerBindingIndex = bindingIndex;
				setConfig.bindingCount++;
			}

			// Increment our descriptor set counter
			vulkanShader->config.descriptorSetCount++;
		}

		// If using instance uniforms, add a UBO descriptor set
		if (vulkanShader->instanceUniformCount > 0 || vulkanShader->instanceUniformSamplerCount > 0)
		{
			auto& setConfig = vulkanShader->config.descriptorSets[vulkanShader->config.descriptorSetCount];

			// Add a binding for UBO if it's used
			if (vulkanShader->instanceUniformCount > 0)
			{
				u8 bindingIndex = setConfig.bindingCount;
				setConfig.bindings[bindingIndex].binding = bindingIndex;
				setConfig.bindings[bindingIndex].descriptorCount = 1;
				setConfig.bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				setConfig.bindings[bindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				setConfig.bindingCount++;
			}

			// Add a binding for samplers if used
			if (vulkanShader->instanceUniformSamplerCount > 0)
			{
				u8 bindingIndex = setConfig.bindingCount;
				setConfig.bindings[bindingIndex].binding = bindingIndex;
				setConfig.bindings[bindingIndex].descriptorCount = vulkanShader->instanceUniformSamplerCount;
				setConfig.bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				setConfig.bindings[bindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				setConfig.samplerBindingIndex = bindingIndex;
				setConfig.bindingCount++;
			}

			// Increment our descriptor set counter
			vulkanShader->config.descriptorSetCount++;
		}

		// Invalidate all instance states
		// TODO: make this dynamic
		for (auto& instanceState : vulkanShader->instanceStates)
		{
			instanceState.id = INVALID_ID;
		}

		// Copy over our cull mode
		vulkanShader->config.cullMode = config.cullMode;
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
			vulkanShader->uniformBuffer.UnMapMemory(0, VK_WHOLE_SIZE);
			vulkanShader->mappedUniformBufferBlock = nullptr;
			vulkanShader->uniformBuffer.Destroy();

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

		// Create descriptor pool
		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = vulkanShader->config.poolSizes;
		poolInfo.maxSets = vulkanShader->config.maxDescriptorSetCount;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkResult result = vkCreateDescriptorPool(logicalDevice, &poolInfo, vkAllocator, &vulkanShader->descriptorPool);
		if (!VulkanUtils::IsSuccess(result))
		{
			m_logger.Error("InitializeShader() - Failed to create descriptor pool: {}.", VulkanUtils::ResultString(result));
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
				m_logger.Error("InitializeShader() - Failed to create Descriptor Set Layout: {}.", VulkanUtils::ResultString(result));
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
			vulkanShader->config.stageCount, stageCreateInfos, viewport, scissor, vulkanShader->config.cullMode, false, true, 
			shader->pushConstantRangeCount, shader->pushConstantRanges
		);

		if (!pipelineCreateResult)
		{
			m_logger.Error("InitializeShader() - Failed to load graphics pipeline.");
			return false;
		}

		// Grab the UBO alignment requirement from our device
		shader->requiredUboAlignment = m_context.device.properties.limits.minUniformBufferOffsetAlignment;

		// Make sure the UBO is aligned according to device requirements
		shader->globalUboStride = GetAligned(shader->globalUboSize, shader->requiredUboAlignment);
		shader->uboStride = GetAligned(shader->uboSize, shader->requiredUboAlignment);

		// Uniform buffer
		// TODO: max count should be configurable, or perhaps long term support of buffer resizing
		const u64 totalBufferSize = shader->globalUboStride + shader->uboStride * VULKAN_MAX_MATERIAL_COUNT;
		if (!vulkanShader->uniformBuffer.Create(RenderBufferType::Uniform, totalBufferSize, true))
		{
			m_logger.Error("InitializeShader() - Failed to create VulkanBuffer.");
			return false;
		}
		vulkanShader->uniformBuffer.Bind(0);

		// Allocate space for the global UBO, which should occupy the stride space and not the actual size need
		if (!vulkanShader->uniformBuffer.Allocate(shader->globalUboStride, &shader->globalUboOffset))
		{
			m_logger.Error("InitializeShader() - Failed to allocate space for the uniform buffer.");
			return false;
		}

		// Map the entire buffer's memory
		vulkanShader->mappedUniformBufferBlock = vulkanShader->uniformBuffer.MapMemory(0, VK_WHOLE_SIZE);

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

	bool RendererVulkan::ShaderApplyInstance(Shader* shader, const bool needsUpdate)
	{
		const auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);
		if (internal->instanceUniformCount == 0 && internal->instanceUniformSamplerCount == 0)
		{
			m_logger.Error("ShaderApplyInstance() - This shader does not use instances");
			return false;
		}

		const u32 imageIndex = m_context.imageIndex;
		VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[imageIndex].handle;

		// Obtain instance data
		VulkanShaderInstanceState* objectState = &internal->instanceStates[shader->boundInstanceId];
		VkDescriptorSet objectDescriptorSet = objectState->descriptorSetState.descriptorSets[imageIndex];

		// We only update if it is needed
		if (needsUpdate)
		{
			VkWriteDescriptorSet descriptorWrites[2]; // Always a max of 2 descriptor sets
			Memory.Zero(descriptorWrites, sizeof(VkWriteDescriptorSet) * 2);
			u32 descriptorCount = 0;
			u32 descriptorIndex = 0;

			VkDescriptorBufferInfo bufferInfo;

			// Descriptor 0 - Uniform buffer
			if (internal->instanceUniformCount > 0)
			{
				// Only do this if the descriptor has not yet been updated
				u8* instanceUboGeneration = &objectState->descriptorSetState.descriptorStates[descriptorIndex].generations[imageIndex];
				if (*instanceUboGeneration == INVALID_ID_U8)
				{
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
			}
			
			// Iterate samplers.
			if (internal->instanceUniformSamplerCount > 0)
			{
				const auto samplerBindingIndex = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].samplerBindingIndex;
				const u32 totalSamplerCount = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindings[samplerBindingIndex].descriptorCount;
				u32 updateSamplerCount = 0;
				VkDescriptorImageInfo imageInfos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];
				for (u32 i = 0; i < totalSamplerCount; i++)
				{
					// TODO: only update in the list if actually needing an update
					const TextureMap* map = internal->instanceStates[shader->boundInstanceId].instanceTextureMaps[i];
					const Texture* t = map->texture;

					// Ensure the texture is valid.
					if (t->generation == INVALID_ID)
					{
						switch (map->use)
						{
							case TextureUse::Diffuse:
								t = Textures.GetDefaultDiffuse();
								break;
							case TextureUse::Specular:
								t = Textures.GetDefaultSpecular();
								break;
							case TextureUse::Normal:
								t = Textures.GetDefaultNormal();
								break;
							case TextureUse::Unknown:
							default:
								m_logger.Warn("Undefined TextureUse {}", ToUnderlying(map->use));
								t = Textures.GetDefault();
								break;
						}
					}

					const auto internalData = static_cast<VulkanTextureData*>(t->internalData);
					imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfos[i].imageView = internalData->image.view;
					imageInfos[i].sampler = static_cast<VkSampler>(map->internalData);

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
		}

		// We always bind for every instance however
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internal->pipeline.layout, 1, 1, &objectDescriptorSet, 0, nullptr);
		return true;
	}

	VkSamplerAddressMode RendererVulkan::ConvertRepeatType(const char* axis, const TextureRepeat repeat) const
	{
		switch (repeat)
		{
			case TextureRepeat::Repeat:
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureRepeat::MirroredRepeat:
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case TextureRepeat::ClampToEdge:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case TextureRepeat::ClampToBorder:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			default:
				m_logger.Warn("ConvertRepeatType(axis = {}) - Type '{}' is not supported. Defaulting to repeat.", axis, ToUnderlying(repeat));
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VkFilter RendererVulkan::ConvertFilterType(const char* op, const TextureFilter filter) const
	{
		switch (filter)
		{
			case TextureFilter::ModeNearest:
				return VK_FILTER_NEAREST;
			case TextureFilter::ModeLinear:
				return VK_FILTER_LINEAR;
			default:
				m_logger.Warn("ConvertRepeatType(op = {}) - Filter '{}' is not supported. Defaulting to linear.", op, ToUnderlying(filter));
				return VK_FILTER_LINEAR;
		}
	}

	bool RendererVulkan::AcquireShaderInstanceResources(Shader* shader, TextureMap** maps, u32* outInstanceId)
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
		auto samplerBindingIndex = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].samplerBindingIndex;
		const u32 instanceTextureCount = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindings[samplerBindingIndex].descriptorCount;
		// Wipe out the memory for the entire array, even if it isn't all used.
		instanceState->instanceTextureMaps = Memory.Allocate<TextureMap*>(shader->instanceTextureCount, MemoryType::Array);
		Texture* defaultTexture = Textures.GetDefault();
		// Set all the texture pointers to default until assigned
		for (u32 i = 0; i < instanceTextureCount; i++)
		{
			instanceState->instanceTextureMaps[i] = maps[i];
			// Set the texture to the default texture if it does not exist
			if (!instanceState->instanceTextureMaps[i]->texture) instanceState->instanceTextureMaps[i]->texture = defaultTexture;
		}

		// Allocate some space in the UBO - by the stride, not the size
		const u64 size = shader->uboStride;
		if (size > 0)
		{
			if (!internal->uniformBuffer.Allocate(size, &instanceState->offset))
			{
				m_logger.Error("AcquireShaderInstanceResources() - failed to acquire UBO space.");
				return false;
			}
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

	bool RendererVulkan::ReleaseShaderInstanceResources(Shader* shader, const u32 instanceId)
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
		if (instanceState->instanceTextureMaps)
		{
			Memory.Free(instanceState->instanceTextureMaps, sizeof(TextureMap*) * shader->instanceTextureCount, MemoryType::Array);
			instanceState->instanceTextureMaps = nullptr;
		}

		internal->uniformBuffer.Free(shader->uboStride, instanceState->offset);
		instanceState->offset = INVALID_ID;
		instanceState->id = INVALID_ID;

		return true;
	}

	bool RendererVulkan::AcquireTextureMapResources(TextureMap* map)
	{
		VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		samplerInfo.minFilter = ConvertFilterType("min", map->minifyFilter);
		samplerInfo.magFilter = ConvertFilterType("mag", map->magnifyFilter);

		samplerInfo.addressModeU = ConvertRepeatType("U", map->repeatU);
		samplerInfo.addressModeV = ConvertRepeatType("V", map->repeatV);
		samplerInfo.addressModeW = ConvertRepeatType("W", map->repeatW);

		// TODO: Configurable
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		const VkResult result = vkCreateSampler(m_context.device.logicalDevice, &samplerInfo, m_context.allocator, reinterpret_cast<VkSampler*>(&map->internalData));
		if (!VulkanUtils::IsSuccess(result))
		{
			m_logger.Error("AcquireTextureMapResources() - Error creating texture sampler: '{}'", VulkanUtils::ResultString(result));
			return false;
		}

		return true;
	}

	void RendererVulkan::ReleaseTextureMapResources(TextureMap* map)
	{
		if (map)
		{
			// NOTE: This ensures there's no way this is in use.
			vkDeviceWaitIdle(m_context.device.logicalDevice);
			vkDestroySampler(m_context.device.logicalDevice, static_cast<VkSampler>(map->internalData), m_context.allocator);
			map->internalData = nullptr;
		}
	}

	bool RendererVulkan::SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value)
	{
		const auto internal = static_cast<VulkanShader*>(shader->apiSpecificData);
		if (uniform->type == Uniform_Sampler)
		{
			if (uniform->scope == ShaderScope::Global)
			{
				shader->globalTextureMaps[uniform->location] = (TextureMap*)value;
			}
			else
			{
				internal->instanceStates[shader->boundInstanceId].instanceTextureMaps[uniform->location] = (TextureMap*)value;
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

		m_context.swapChain.Recreate(&m_context, m_context.frameBufferWidth, m_context.frameBufferHeight);

		// Update the size generation so that they are in sync again
		m_context.frameBufferSizeLastGeneration = m_context.frameBufferSizeGeneration;

		// Cleanup SwapChain
		for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
		{
			m_context.graphicsCommandBuffers[i].Free(&m_context, m_context.device.graphicsCommandPool);
		}

		// Tell the renderer that a refresh is required
		m_context.frontend->RegenerateRenderTargets();

		CreateCommandBuffers();

		m_context.recreatingSwapChain = false;
		return true;
	}

	bool RendererVulkan::CreateModule(VulkanShaderStageConfig config, VulkanShaderStage* shaderStage) const
	{
		// Read the resource
		BinaryResource res{};
		if (!Resources.Load(config.fileName, &res))
		{
			m_logger.Error("CreateModule() - Unable to read shader module: '{}'", config.fileName);
			return false;
		}

		shaderStage->createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shaderStage->createInfo.codeSize = res.size;
		shaderStage->createInfo.pCode = reinterpret_cast<u32*>(res.data);

		VK_CHECK(vkCreateShaderModule(m_context.device.logicalDevice, &shaderStage->createInfo, m_context.allocator, &shaderStage->handle));

		// Release our resource
		Resources.Unload(&res);

		//Shader stage info
		shaderStage->shaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderStage->shaderStageCreateInfo.stage = config.stage;
		shaderStage->shaderStageCreateInfo.module = shaderStage->handle;
		shaderStage->shaderStageCreateInfo.pName = "main"; // TODO: make this configurable?

		return true;
	}
}
