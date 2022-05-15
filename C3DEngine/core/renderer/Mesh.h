
#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../VkTypes.h"
#include "../materials/Material.h"

namespace C3D
{
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

		void PackNormal(glm::vec3 n);
		void PackColor(glm::vec3 c);
	};

	struct RenderBounds
	{
		glm::vec3 origin;
		glm::vec3 extents;

		float radius;
		bool valid;
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		AllocatedBuffer<Vertex> vertexBuffer;
		AllocatedBuffer<uint32_t> indexBuffer;

		RenderBounds bounds;

		bool LoadFromMeshAsset(const std::string& fileName);
		//bool LoadFromObj(const char* fileName);
	};

	struct MeshPushConstants
	{
		glm::vec4 data;
		glm::mat4 renderMatrix;
	};

	struct MeshObject {
		Mesh* mesh{ nullptr };

		Material* material{ nullptr };
		uint32_t customSortKey = 0;
		glm::mat4 transformMatrix;

		RenderBounds bounds;

		uint32_t bDrawForwardPass : 1;
		uint32_t bDrawShadowPass : 1;
	};
}

