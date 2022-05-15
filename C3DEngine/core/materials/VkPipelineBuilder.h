
#pragma once
#include "../VkTypes.h"
#include "../VkMesh.h"

#include <vector>

namespace C3D
{
	class VkPipelineBuilder
	{
	public:
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VertexInputDescription vertexDescription;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo multiSampling;
		VkPipelineDepthStencilStateCreateInfo depthStencil;

		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineLayout layout;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;

		VkPipeline Build(VkDevice device, VkRenderPass pass);

		void ClearVertexInput();

		void SetShaders(const struct ShaderEffect* effect);
	};

	class VkComputePipelineBuilder
	{
	public:
		VkPipelineShaderStageCreateInfo shaderStage;
		VkPipelineLayout layout;

		VkPipeline Build(VkDevice device) const;
	};
}
