
#include "Mesh.h"
#include "../Logger.h"

#include <tiny_obj_loader.h>

#include "MeshAsset.h"

namespace C3D
{
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

	glm::vec2 OctNormalWrap(const glm::vec2 v)
	{
		glm::vec2 wrap{};
		wrap.x = (1.0f - glm::abs(v.y)) * (v.x >= 0.0f ? 1.0f : -1.0f);
		wrap.y = (1.0f - glm::abs(v.x)) * (v.y >= 0.0f ? 1.0f : -1.0f);
		return wrap;
	}

	glm::vec2 OctNormalEncode(glm::vec3 n)
	{
		n /= (glm::abs(n.x) + glm::abs(n.y) + glm::abs(n.z));

		const auto wrapped = OctNormalWrap(n);

		glm::vec2 result;
		result.x = n.z >= 0.0f ? n.x : wrapped.x;
		result.y = n.z >= 0.0f ? n.y : wrapped.y;

		result.x = result.x * 0.5f + 0.5f;
		result.y = result.y * 0.5f + 0.5f;

		return result;
	}

	void Vertex::PackNormal(const glm::vec3 n)
	{
		const auto oct = OctNormalEncode(n);

		normal.x = static_cast<uint8_t>(oct.x * 255);
		normal.y = static_cast<uint8_t>(oct.y * 255);
	}

	void Vertex::PackColor(const glm::vec3 c)
	{
		color.r = static_cast<uint8_t>(c.r * 255);
		color.g = static_cast<uint8_t>(c.g * 255);
		color.b = static_cast<uint8_t>(c.b * 255);
	}

	bool Mesh::LoadFromMeshAsset(const std::string& fileName)
	{
		Assets::AssetFile file;

		if (!LoadBinary(fileName, file))
		{
			Logger::Error("Failed to load Mesh {}", fileName);
			return false;
		}

		Assets::MeshInfo meshInfo = ReadMeshInfo(&file);

		std::vector<char> vertexBuffer;
		std::vector<char> indexBuffer;

		vertexBuffer.resize(meshInfo.vertexBufferSize);
		indexBuffer.resize(meshInfo.indexBufferSize);

		UnpackMesh(&meshInfo, file.binaryBlob.data(), file.binaryBlob.size(), vertexBuffer.data(), indexBuffer.data());

		bounds.extents.x = meshInfo.bounds.extents[0];
		bounds.extents.y = meshInfo.bounds.extents[1];
		bounds.extents.z = meshInfo.bounds.extents[2];

		bounds.origin.x = meshInfo.bounds.origin[0];
		bounds.origin.y = meshInfo.bounds.origin[1];
		bounds.origin.z = meshInfo.bounds.origin[2];

		bounds.radius = meshInfo.bounds.radius;
		bounds.valid = true;

		vertices.clear();
		indices.clear();

		indices.resize(indexBuffer.size() / sizeof(uint32_t));
		for (size_t i = 0; i < indices.size(); i++)
		{
			const auto unpackedIndices = reinterpret_cast<uint32_t*>(indexBuffer.data());
			indices[i] = unpackedIndices[i];
		}

		if (meshInfo.vertexFormat == Assets::VertexFormat::F32)
		{
			auto unpackedVertices = reinterpret_cast<Assets::VertexF32*>(vertexBuffer.data());

			vertices.resize(vertexBuffer.size() / sizeof(Assets::VertexF32));

			for (size_t i = 0; i < vertices.size(); i++)
			{
				vertices[i].position.x = unpackedVertices[i].position[0];
				vertices[i].position.y = unpackedVertices[i].position[1];
				vertices[i].position.z = unpackedVertices[i].position[2];

				auto normal = glm::vec3(
					unpackedVertices[i].normal[0],
					unpackedVertices[i].normal[1],
					unpackedVertices[i].normal[2]
				);
				auto color = glm::vec3(
					unpackedVertices[i].color[0],
					unpackedVertices[i].color[1],
					unpackedVertices[i].color[2]
				);

				vertices[i].PackNormal(normal);
				vertices[i].PackColor(color);

				vertices[i].uv.x = unpackedVertices[i].uv[0];
				vertices[i].uv.y = unpackedVertices[i].uv[1];
			}
		}
		else if (meshInfo.vertexFormat == Assets::VertexFormat::P32N8C8V16)
		{
			auto unpackedVertices = reinterpret_cast<Assets::VertexP32N8C8V16*>(vertexBuffer.data());

			vertices.resize(vertexBuffer.size() / sizeof(Assets::VertexP32N8C8V16));

			for (size_t i = 0; i < vertices.size(); i++)
			{
				vertices[i].position.x = unpackedVertices[i].position[0];
				vertices[i].position.y = unpackedVertices[i].position[1];
				vertices[i].position.z = unpackedVertices[i].position[2];

				auto normal = glm::vec3(
					unpackedVertices[i].normal[0],
					unpackedVertices[i].normal[1],
					unpackedVertices[i].normal[2]
				);

				vertices[i].PackNormal(normal);

				vertices[i].color.r = unpackedVertices[i].color[0];
				vertices[i].color.g = unpackedVertices[i].color[1];
				vertices[i].color.b = unpackedVertices[i].color[2];

				vertices[i].uv.x = unpackedVertices[i].uv[0];
				vertices[i].uv.y = unpackedVertices[i].uv[1];
			}
		}

		Logger::Info("Loaded Mesh {} with Vertices={}, Triangles={}", fileName, vertices.size(), indices.size() / 3);
		return true;
	}
}

/*
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
}*/