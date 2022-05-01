
#pragma once
#include <vk_types.h>

#include "vk_mesh.h"

struct Material
{
	VkDescriptorSet textureSet{ nullptr };
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject
{
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};