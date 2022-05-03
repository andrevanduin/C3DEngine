
#include "VkInitializers.h"

VkCommandPoolCreateInfo VkInit::CommandPoolCreateInfo(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = flags;
	info.queueFamilyIndex = queueFamilyIndex;
	return info;
}

VkCommandBufferAllocateInfo VkInit::CommandBufferAllocateInfo(VkCommandPool pool, const uint32_t count, const VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.commandPool = pool;
	info.level = level;
	info.commandBufferCount = count;
	return info;
}

VkCommandBufferBeginInfo VkInit::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;

	info.pInheritanceInfo = nullptr;
	info.flags = flags;
	return info;
}

VkPipelineShaderStageCreateInfo VkInit::PipelineShaderStageCreateInfo(const VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
	VkPipelineShaderStageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.pNext = nullptr;
	info.stage = stage;
	info.module = shaderModule;
	info.pName = "main"; // TODO: Change this later when we want shaders with a different entry point

	return info;
}

VkPipelineVertexInputStateCreateInfo VkInit::VertexInputStateCreateInfo()
{
	VkPipelineVertexInputStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info.pNext = nullptr;
	info.vertexBindingDescriptionCount = 0;
	info.vertexAttributeDescriptionCount = 0;
	return info;
}

VkPipelineInputAssemblyStateCreateInfo VkInit::InputAssemblyCreateInfo(const VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.pNext = nullptr;
	info.topology = topology;
	info.primitiveRestartEnable = VK_FALSE;
	return info;
}

VkPipelineRasterizationStateCreateInfo VkInit::RasterizationStateCreateInfo(const VkPolygonMode polygonMode)
{
	VkPipelineRasterizationStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info.pNext = nullptr;
	info.depthClampEnable = VK_FALSE;
	info.rasterizerDiscardEnable = VK_FALSE;

	info.polygonMode = polygonMode;
	info.lineWidth = 1.0f;

	info.cullMode = VK_CULL_MODE_NONE;
	info.frontFace = VK_FRONT_FACE_CLOCKWISE;

	info.depthBiasEnable = VK_FALSE;
	info.depthBiasConstantFactor = 0.0f;
	info.depthBiasClamp = 0.0f;
	info.depthBiasSlopeFactor = 0.0f;
	return info;
}

VkPipelineMultisampleStateCreateInfo VkInit::MultisamplingStateCreateInfo()
{
	VkPipelineMultisampleStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.sampleShadingEnable = VK_FALSE;
	info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // 1 sample per pixel aka no multisampling
	info.minSampleShading = 1.0f;
	info.pSampleMask = nullptr;
	info.alphaToCoverageEnable = VK_FALSE;
	info.alphaToOneEnable = VK_FALSE;
	return info;
}

VkPipelineColorBlendAttachmentState VkInit::ColorBlendAttachmentState()
{
	VkPipelineColorBlendAttachmentState state = {};
	state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	state.blendEnable = VK_FALSE;
	return state;
}

VkPipelineLayoutCreateInfo VkInit::PipelineLayoutCreateInfo()
{
	VkPipelineLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;

	info.flags = 0;
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;
	return info;
}

VkFenceCreateInfo VkInit::FenceCreateInfo(const VkFenceCreateFlags flags)
{
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = flags;
	return info;
}

VkSemaphoreCreateInfo VkInit::SemaphoreCreateInfo(const VkSemaphoreCreateFlags flags)
{
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = flags;
	return info;
}

VkSubmitInfo VkInit::SubmitInfo(VkCommandBuffer* cmd)
{
	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.pNext = nullptr;

	info.waitSemaphoreCount = 0;
	info.pWaitSemaphores = nullptr;
	info.pWaitDstStageMask = nullptr;
	info.commandBufferCount = 1;
	info.pCommandBuffers = cmd;
	info.signalSemaphoreCount = 0;
	info.pSignalSemaphores = nullptr;

	return info;
}

VkPresentInfoKHR VkInit::PresentInfo()
{
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;

	info.swapchainCount = 0;
	info.pSwapchains = nullptr;
	info.pWaitSemaphores = nullptr;
	info.waitSemaphoreCount = 0;
	info.pImageIndices = nullptr;

	return info;
}

VkRenderPassBeginInfo VkInit::RenderPassBeginInfo(VkRenderPass renderPass, const VkExtent2D windowExtent, VkFramebuffer framebuffer)
{
	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.pNext = nullptr;

	info.renderPass = renderPass;
	info.renderArea.offset.x = 0;
	info.renderArea.offset.y = 0;
	info.renderArea.extent = windowExtent;
	info.clearValueCount = 1;
	info.pClearValues = nullptr;
	info.framebuffer = framebuffer;

	return info;
}

VkImageCreateInfo VkInit::ImageCreateInfo(const VkFormat format, const VkImageUsageFlags usageFlags, const VkExtent3D extent)
{
	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.imageType = VK_IMAGE_TYPE_2D;

	info.format = format;
	info.extent = extent;

	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;

	return info;
}

VkImageViewCreateInfo VkInit::ImageViewCreateInfo(const VkFormat format, VkImage image, const VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;

	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.image = image;
	info.format = format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = aspectFlags;

	return info;
}

VkPipelineDepthStencilStateCreateInfo VkInit::DepthStencilCreateInfo(const bool depthTest, const bool depthWrite, const VkCompareOp compareOp)
{
	VkPipelineDepthStencilStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
	info.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
	info.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
	info.depthBoundsTestEnable = VK_FALSE;
	info.minDepthBounds = 0.0f;
	info.maxDepthBounds = 1.0f;
	info.stencilTestEnable = VK_FALSE;

	return info;
}

VkDescriptorSetLayoutBinding VkInit::DescriptorSetLayoutBinding(const VkDescriptorType type, const VkShaderStageFlags stageFlags, const uint32_t binding)
{
	VkDescriptorSetLayoutBinding setBind = {};
	setBind.binding = binding;
	setBind.descriptorCount = 1;
	setBind.descriptorType = type;
	setBind.pImmutableSamplers = nullptr;
	setBind.stageFlags = stageFlags;

	return setBind;
}

VkWriteDescriptorSet VkInit::WriteDescriptorBuffer(const VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, const uint32_t binding)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;

	write.dstBinding = binding;
	write.dstSet = dstSet;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = bufferInfo;

	return write;
}

VkSamplerCreateInfo VkInit::SamplerCreateInfo(const VkFilter filters, const VkSamplerAddressMode samplerAddressMode)
{
	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;

	info.magFilter = filters;
	info.minFilter = filters;
	info.addressModeU = samplerAddressMode;
	info.addressModeV = samplerAddressMode;
	info.addressModeW = samplerAddressMode;

	return info;
}

VkWriteDescriptorSet VkInit::WriteDescriptorImage(const VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, const uint32_t binding)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;

	write.dstBinding = binding;
	write.dstSet = dstSet;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = imageInfo;

	return write;
}
