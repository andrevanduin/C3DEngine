
#include "vulkan_command_buffer.h"
#include "services/services.h"

namespace C3D
{
	void VulkanCommandBufferManager::Allocate(VulkanContext* context, VkCommandPool pool, const bool isPrimary, VulkanCommandBuffer* commandBuffer)
	{
		Memory::Zero(commandBuffer, sizeof(VulkanCommandBuffer));

		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = pool;
		allocateInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocateInfo.commandBufferCount = 1;
		allocateInfo.pNext = nullptr;

		commandBuffer->state = VulkanCommandBufferState::NotAllocated;
		VK_CHECK(vkAllocateCommandBuffers(context->device.logicalDevice, &allocateInfo, &commandBuffer->handle));

		commandBuffer->state = VulkanCommandBufferState::Ready;
	}

	void VulkanCommandBufferManager::Free(const VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* commandBuffer)
	{
		vkFreeCommandBuffers(context->device.logicalDevice, pool, 1, &commandBuffer->handle);

		commandBuffer->handle = nullptr;
		commandBuffer->state = VulkanCommandBufferState::NotAllocated;
	}

	void VulkanCommandBufferManager::Begin(VulkanCommandBuffer* commandBuffer, const bool isSingleUse, const bool isRenderPassContinue, const bool isSimultaneousUse)
	{
		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = 0;

		if (isSingleUse) beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (isRenderPassContinue) beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		if (isSimultaneousUse) beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VK_CHECK(vkBeginCommandBuffer(commandBuffer->handle, &beginInfo));
		commandBuffer->state = VulkanCommandBufferState::Recording;
	}

	void VulkanCommandBufferManager::End(VulkanCommandBuffer* commandBuffer)
	{
		VK_CHECK(vkEndCommandBuffer(commandBuffer->handle));
		commandBuffer->state = VulkanCommandBufferState::RecordingEnded;
	}

	void VulkanCommandBufferManager::UpdateSubmitted(VulkanCommandBuffer* commandBuffer)
	{
		commandBuffer->state = VulkanCommandBufferState::Submitted;
	}

	void VulkanCommandBufferManager::Reset(VulkanCommandBuffer* commandBuffer)
	{
		commandBuffer->state = VulkanCommandBufferState::Ready;
	}

	void VulkanCommandBufferManager::AllocateAndBeginSingleUse(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* commandBuffer, VkQueue queue)
	{
		Allocate(context, pool, true, commandBuffer);
		Begin(commandBuffer, true, false, false);
	}

	void VulkanCommandBufferManager::EndSingleUse(const VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* commandBuffer, VkQueue queue)
	{
		End(commandBuffer);

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer->handle;
		VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, 0));

		VK_CHECK(vkQueueWaitIdle(queue));

		Free(context, pool, commandBuffer);
	}
}
