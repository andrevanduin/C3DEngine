//
#pragma once
#include <vk_types.h>

struct GpuCameraData
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection;
};

struct GpuSceneData
{
	glm::vec4 fogColor;
	glm::vec4 fogDistance;
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection;
	glm::vec4 sunlightColor;
};

struct GpuObjectData
{
	glm::mat4 modelMatrix;
};

struct FrameData
{
	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	AllocatedBuffer cameraBuffer, objectBuffer;
	VkDescriptorSet globalDescriptor, objectDescriptor;
};

struct UploadContext
{
	VkFence uploadFence;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
};