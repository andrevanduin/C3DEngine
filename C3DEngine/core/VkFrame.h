
#pragma once
#include "VkPushBuffer.h"
#include "VkTypes.h"
#include "VkDeletionQueue.h"

namespace C3D
{
	class DescriptorAllocator;

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
		glm::vec4 originRad;
		glm::vec4 extents;
	};

	struct FrameData
	{
		VkSemaphore presentSemaphore, renderSemaphore;
		VkFence renderFence;

		DeletionQueue deletionQueue;

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		PushBuffer dynamicData;

		AllocatedBufferUntyped debugOutputBuffer;

		DescriptorAllocator* dynamicDescriptorAllocator;

		std::vector<uint32_t> debugDataOffsets;
		std::vector<std::string> debugDataNames;
	};

	struct UploadContext
	{
		VkFence uploadFence;
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
	};
}

