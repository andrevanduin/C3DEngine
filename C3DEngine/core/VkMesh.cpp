
#include "VkMesh.h"

#include <tiny_obj_loader.h>
#include <iostream>

VertexInputDescription Vertex::GetVertexDescription()
{
	VertexInputDescription description;

	VkVertexInputBindingDescription binding = {};
	binding.binding = 0;
	binding.stride = sizeof(Vertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.bindings.push_back(binding);

	const VkVertexInputAttributeDescription positionAttribute =
	{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };

	const VkVertexInputAttributeDescription normalAttribute =
	{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) };

	const VkVertexInputAttributeDescription colorAttribute =
	{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) };

	const VkVertexInputAttributeDescription uvAttribute =
	{ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) };

	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(normalAttribute);
	description.attributes.push_back(colorAttribute);
	description.attributes.push_back(uvAttribute);

	return description;
}

bool Mesh::LoadFromObj(const char* fileName)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName, nullptr);

	if (!warn.empty())
	{
		std::cout << "[OBJ Loader WARN] " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cerr << "[OBJ Loader ERROR]" << err << std::endl;
		return false;
	}

	for (auto& [name, mesh, lines, points] : shapes)
	{
		size_t indexOffset = 0;
		for (size_t f = 0; f < mesh.num_face_vertices.size(); f++)
		{
			int fv = 3;

			for (size_t v = 0; v < fv; v++)
			{
				auto [vertex_index, normal_index, texcoord_index] = mesh.indices[indexOffset + v];

				// Positions
				tinyobj::real_t vx = attrib.vertices[3 * vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * vertex_index + 2];

				// Normals
				tinyobj::real_t nx = attrib.normals[3 * normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * normal_index + 2];

				tinyobj::real_t ux = attrib.texcoords[2 * texcoord_index + 0];
				tinyobj::real_t uy = attrib.texcoords[2 * texcoord_index + 1];

				Vertex vert;
				vert.position.x = vx;
				vert.position.y = vy;
				vert.position.z = vz;

				vert.normal.x = nx;
				vert.normal.y = ny;
				vert.normal.z = nz;

				vert.uv.x = ux;
				vert.uv.y = 1 - uy;

				vert.color = vert.normal;

				vertices.push_back(vert);
			}

			indexOffset += fv;
		}
	}

	return true;
}
