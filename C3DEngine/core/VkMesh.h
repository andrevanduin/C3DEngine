
#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "VkTypes.h"

struct VertexInputDescription
{
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;

	static VertexInputDescription GetVertexDescription();
};

struct Mesh
{
	std::vector<Vertex> vertices;
	AllocatedBuffer vertexBuffer;

	bool LoadFromObj(const char* fileName);
};

struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 renderMatrix;
};