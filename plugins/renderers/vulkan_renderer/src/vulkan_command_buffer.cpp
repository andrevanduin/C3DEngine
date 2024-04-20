
#include "vulkan_command_buffer.h"

#include "vulkan_types.h"
#include "vulkan_utils.h"

namespace C3D
{
    void VulkanCommandBuffer::Allocate(const VulkanContext* context, VkCommandPool pool, const bool isPrimary)
    {
        VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool                 = pool;
        allocateInfo.level                       = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocateInfo.commandBufferCount          = 1;
        allocateInfo.pNext                       = nullptr;

        state = VulkanCommandBufferState::NotAllocated;
        VK_CHECK(vkAllocateCommandBuffers(context->device.GetLogical(), &allocateInfo, &handle));

        state = VulkanCommandBufferState::Ready;
    }

    void VulkanCommandBuffer::Free(const VulkanContext* context, VkCommandPool pool)
    {
        vkFreeCommandBuffers(context->device.GetLogical(), pool, 1, &handle);

        handle = nullptr;
        state  = VulkanCommandBufferState::NotAllocated;
    }

    void VulkanCommandBuffer::Begin(const bool isSingleUse, const bool isRenderPassContinue, const bool isSimultaneousUse)
    {
        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags                    = 0;

        if (isSingleUse) beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (isRenderPassContinue) beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        if (isSimultaneousUse) beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VK_CHECK(vkBeginCommandBuffer(handle, &beginInfo));
        state = VulkanCommandBufferState::Recording;
    }

    void VulkanCommandBuffer::End()
    {
        VK_CHECK(vkEndCommandBuffer(handle));
        state = VulkanCommandBufferState::RecordingEnded;
    }

    void VulkanCommandBuffer::UpdateSubmitted() { state = VulkanCommandBufferState::Submitted; }

    void VulkanCommandBuffer::Reset() { state = VulkanCommandBufferState::Ready; }

    void VulkanCommandBuffer::AllocateAndBeginSingleUse(const VulkanContext* context, VkCommandPool pool)
    {
        Allocate(context, pool, true);
        Begin(true, false, false);
    }

    void VulkanCommandBuffer::EndSingleUse(const VulkanContext* context, VkCommandPool pool, VkQueue queue)
    {
        End();

        VkSubmitInfo submitInfo       = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &handle;
        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, 0));

        auto result = vkQueueWaitIdle(queue);
        if (!VulkanUtils::IsSuccess(result))
        {
            INSTANCE_ERROR_LOG("VULKAN_COMMAND_BUFFER", "vkQueueWaitIdle failed with following error: {}.",
                               VulkanUtils::ResultString(result));
        }

        // VK_CHECK(vkQueueWaitIdle(queue));

        Free(context, pool);
    }
}  // namespace C3D
