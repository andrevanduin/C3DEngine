
#include "Gltf.h"

#include <iostream>
#include <PrefabAsset.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tiny_gltf.h>

using namespace Assets;
using namespace tinygltf;

bool GltfConverter::LoadFromAscii(const std::filesystem::directory_entry& directory, const std::filesystem::path& outputPath)
{
	TinyGLTF loader;
	std::string err, warn;
	m_model = new Model();

	const auto result = loader.LoadASCIIFromFile(m_model, &err, &warn, directory.path().string());
	if (!warn.empty())
	{
		std::cout << "Warning: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cout << "Error: " << err << std::endl;
	}

	if (!result)
	{
		std::cout << "Failed to parse GLTF" << std::endl;
		return false;
	}

	m_input = directory.path();
	m_output = outputPath.parent_path() / (m_input.stem().string() + "_GLTF");

	create_directory(m_output);

	ExtractMeshes();
	ExtractMaterials();
	ExtractNodes();

	delete m_model;
	return true;
}

void GltfConverter::ExtractMeshes()
{
	for (auto meshIndex = 0; meshIndex < m_model->meshes.size(); meshIndex++)
	{
		auto& mesh = m_model->meshes[meshIndex];

		std::vector<VertexF32> vertices;
		std::vector<uint32_t> indices;

		for (auto primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); primitiveIndex++)
		{
			auto& primitive = mesh.primitives[primitiveIndex];

			vertices.clear();
			indices.clear();

			auto meshName = CalculateMeshName(meshIndex, primitiveIndex);

			ExtractIndices(primitive, indices);
			ExtractVertices(primitive, vertices);

			MeshInfo meshInfo;
			meshInfo.vertexFormat = VertexFormat::F32;
			meshInfo.vertexBufferSize = vertices.size() * sizeof(VertexF32);
			meshInfo.indexBufferSize = indices.size() * sizeof(uint32_t);
			meshInfo.indexSize = sizeof(uint32_t);
			meshInfo.originalFile = m_input.string();

			meshInfo.bounds = CalculateBounds(vertices);

			auto file = PackMesh(&meshInfo, reinterpret_cast<char*>(vertices.data()), reinterpret_cast<char*>(indices.data()));
			auto path = m_output / (meshName + ".mesh");

			SaveBinary(path.string(), file);
		}
	}

}

void GltfConverter::ExtractMaterials()
{
	int materialIndex = 0;
	for (auto& material : m_model->materials)
	{
		auto materialName = CalculateMaterialName(materialIndex);
		materialIndex++;

		auto& pbr = material.pbrMetallicRoughness;

		MaterialInfo materialInfo;
		materialInfo.baseEffect = "defaultPBR";

		if (pbr.baseColorTexture.index < 0) pbr.baseColorTexture.index = 0;
		MaterialSetTexture(pbr.baseColorTexture.index, materialInfo, "baseColor");

		if (pbr.metallicRoughnessTexture.index >= 0)
		{
			MaterialSetTexture(pbr.metallicRoughnessTexture.index, materialInfo, "metallicRoughness");
		}

		if (material.normalTexture.index >= 0)
		{
			MaterialSetTexture(material.normalTexture.index, materialInfo, "normals");
		}

		if (material.occlusionTexture.index >= 0)
		{
			MaterialSetTexture(material.occlusionTexture.index, materialInfo, "occlusion");
		}

		if (material.emissiveTexture.index >= 0)
		{
			MaterialSetTexture(material.emissiveTexture.index, materialInfo, "emissive");
		}

		auto materialPath = m_output / (materialName + ".mat");
		if (material.alphaMode == "BLEND")
		{
			materialInfo.transparency = TransparencyMode::Transparent;
		}
		else
		{
			materialInfo.transparency = TransparencyMode::Opaque;
		}

		auto file = PackMaterial(&materialInfo);
		SaveBinary(materialPath.string(), file);
	}
}

void GltfConverter::ExtractNodes()
{
	PrefabInfo info{};
	std::vector<uint64_t> meshNodes;

	int i = 0;
	for (const auto& node : m_model->nodes)
	{
		info.nodeNames[i] = node.name;

		std::array<float, 16> matrix{};
		if (!node.matrix.empty())
		{
			for (int n = 0; n < 16; n++) matrix[n] = node.matrix[n];

			glm::mat4 mat;
			memcpy(&mat, &matrix, sizeof(glm::mat4));
			memcpy(matrix.data(), &mat, sizeof(glm::mat4));
		}
		else
		{
			glm::mat4 translation{ 1.0f };
			if (!node.translation.empty())
			{
				translation = translate(glm::vec3{ node.translation[0], node.translation[1], node.translation[2] });
			}

			glm::mat4 rotation{ 1.0f };
			if (!node.rotation.empty())
			{
				glm::quat rot(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
				rotation = glm::mat4{ rot };
			}

			glm::mat4 scale{ 1.0f };
			if (!node.scale.empty())
			{
				scale = glm::scale(glm::vec3{ node.scale[0], node.scale[1], node.scale[2] });
			}

			glm::mat4 transformMatrix = (translation * rotation * scale);
			memcpy(matrix.data(), &transformMatrix, sizeof(glm::mat4));
		}

		info.nodeMatrices[i] = info.matrices.size();
		info.matrices.push_back(matrix);

		if (node.mesh >= 0)
		{
			auto mesh = m_model->meshes[node.mesh];
			if (mesh.primitives.size() > 1)
			{
				meshNodes.push_back(i);
			}
			else
			{
				const auto primitive = mesh.primitives[0];
				auto meshName = CalculateMeshName(node.mesh, 0);

				auto meshPath = m_output / (meshName + ".mesh");

				const int material = primitive.material;
				auto materialName = CalculateMaterialName(material);

				auto materialPath = m_output / (materialName + ".mat");

				PrefabInfo::NodeMesh nodeMesh;
				nodeMesh.meshPath = m_state.ConvertToExportRelative(meshPath).string();
				nodeMesh.materialPath = m_state.ConvertToExportRelative(materialPath).string();

				info.nodeMeshes[i] = nodeMesh;
			}
		}
		i++;
	}

	for (int i = 0; i < m_model->nodes.size(); i++)
	{
		for (auto c : m_model->nodes[i].children)
		{
			info.nodeParents[c] = i;
		}
	}

	auto flip = glm::mat4{ 1.0f };
	flip[1][1] = -1;

	auto rotation = glm::mat4{ 1.0f };
	rotation = rotate(glm::radians(-180.0f), glm::vec3{ 1, 0, 0 });

	for (int i = 0; i < m_model->nodes.size(); i++)
	{
		if (auto it = info.nodeParents.find(i); it == info.nodeParents.end())
		{
			auto matrix = info.matrices[info.nodeMatrices[i]];
			glm::mat4 mat;

			memcpy(&mat, &matrix, sizeof(glm::mat4));

			mat = rotation * (flip * mat);

			memcpy(&matrix, &mat, sizeof(glm::mat4));

			info.matrices[info.nodeMatrices[i]] = matrix;
		}
	}

	int nodeIndex = m_model->nodes.size();
	for (int i = 0; i < meshNodes.size(); i++)
	{
		auto& node = m_model->nodes[i];
		if (node.mesh < 0) break;

		auto mesh = m_model->meshes[node.mesh];
		for (int primIndex = 0; i < mesh.primitives.size(); primIndex++)
		{
			auto primitive = mesh.primitives[primIndex];
			int newNode = nodeIndex++;

			info.nodeNames[newNode] = info.nodeNames[i] + "_PRIM_" + std::to_string(primIndex);

			int material = primitive.material;
			auto mat = m_model->materials[material];

			auto matName = CalculateMaterialName(material);
			auto meshName = CalculateMeshName(node.mesh, primIndex);

			auto materialPath = m_output / (matName + ".mat");
			auto meshPath = m_output / (meshName + ".mesh");

			PrefabInfo::NodeMesh nodeMesh;
			nodeMesh.meshPath = m_state.ConvertToExportRelative(meshPath).string();
			nodeMesh.materialPath = m_state.ConvertToExportRelative(materialPath).string();

			info.nodeMeshes[newNode] = nodeMesh;
		}
	}

	auto file = PackPrefab(info);
	auto scenePath = m_output.parent_path() / m_input.stem();
	scenePath.replace_extension(".pfb");

	SaveBinary(scenePath.string(), file);
}

void GltfConverter::ExtractIndices(const Primitive& primitive, std::vector<uint32_t>& indices)
{
	const int indexAccessor = primitive.indices;
	const int componentType = m_model->accessors[indexAccessor].componentType;

	std::vector<uint8_t> unpackedIndices;
	UnpackBuffer(m_model->accessors[indexAccessor], unpackedIndices);

	for (int i = 0; i < m_model->accessors[indexAccessor].count; i++)
	{
		uint32_t index;
		switch (componentType)
		{
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			{
				const auto bfr = reinterpret_cast<uint16_t*>(unpackedIndices.data());
				index = *(bfr + i);
			}
			break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			{
				const auto bfr = reinterpret_cast<int16_t*>(unpackedIndices.data());
				index = *(bfr + i);
			}
			break;
			default:
				assert(false);
		}

		indices.push_back(index);
	}

	for (int i = 0; i < indices.size() / 3; i++)
	{
		// Flip the triangle
		std::swap(indices[i * 3 + 1], indices[i * 3 + 2]);
	}
}

void GltfConverter::ExtractVertices(Primitive& primitive, std::vector<VertexF32>& vertices)
{
	const auto& posAccessor = m_model->accessors[primitive.attributes["POSITION"]];

	vertices.resize(posAccessor.count);

	std::vector<uint8_t> posData;
	UnpackBuffer(posAccessor, posData);

	for (int i = 0; i < vertices.size(); i++)
	{
		if (posAccessor.type == TINYGLTF_TYPE_VEC3)
		{
			if (posAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				const auto dtf = reinterpret_cast<float*>(posData.data());

				vertices[i].position[0] = *(dtf + (i * 3) + 0);
				vertices[i].position[1] = *(dtf + (i * 3) + 1);
				vertices[i].position[2] = *(dtf + (i * 3) + 2);
			}
			else
			{
				assert(false);
			}
		}
		else
		{
			assert(false);
		}
	}

	const auto& normalAccessor = m_model->accessors[primitive.attributes["NORMAL"]];

	std::vector<uint8_t> normalData;
	UnpackBuffer(normalAccessor, normalData);

	for (int i = 0; i < vertices.size(); i++)
	{
		if (normalAccessor.type == TINYGLTF_TYPE_VEC3)
		{
			if (normalAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				const auto dtf = reinterpret_cast<float*>(normalData.data());

				vertices[i].normal[0] = *(dtf + (i * 3) + 0);
				vertices[i].normal[1] = *(dtf + (i * 3) + 1);
				vertices[i].normal[2] = *(dtf + (i * 3) + 2);

				vertices[i].color[0] = *(dtf + (i * 3) + 0);
				vertices[i].color[1] = *(dtf + (i * 3) + 1);
				vertices[i].color[2] = *(dtf + (i * 3) + 2);
			}
			else
			{
				assert(false);
			}
		}
		else
		{
			assert(false);
		}
	}

	const auto& uvAccessor = m_model->accessors[primitive.attributes["TEXCOORD_0"]];

	std::vector<uint8_t> uvData;
	UnpackBuffer(uvAccessor, uvData);

	for (int i = 0; i < vertices.size(); i++) {
		if (uvAccessor.type == TINYGLTF_TYPE_VEC2)
		{
			if (uvAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				const auto dtf = reinterpret_cast<float*>(uvData.data());

				vertices[i].uv[0] = *(dtf + (i * 2) + 0);
				vertices[i].uv[1] = *(dtf + (i * 2) + 1);
			}
			else
			{
				assert(false);
			}
		}
		else
		{
			assert(false);
		}
	}
}

void GltfConverter::UnpackBuffer(const Accessor& accessor, std::vector<uint8_t>& outputBuffer) const
{
	const int bufferId = accessor.bufferView;
	size_t elementSize = GetComponentSizeInBytes(accessor.componentType);

	const auto& bufferView = m_model->bufferViews[bufferId];
	const auto& bufferData = m_model->buffers[bufferView.buffer];

	const uint8_t* dataPtr = bufferData.data.data() + accessor.byteOffset + bufferView.byteOffset;

	const int components = GetNumComponentsInType(accessor.type);

	elementSize *= components;

	size_t stride = bufferView.byteStride;
	if (stride == 0) stride = elementSize;

	outputBuffer.resize(accessor.count, elementSize);

	for (int i = 0; i < accessor.count; i++)
	{
		const uint8_t* dataIndex = dataPtr + stride * i;
		uint8_t* targetPtr = outputBuffer.data() + elementSize * i;

		memcpy(targetPtr, dataIndex, elementSize);
	}
}

void GltfConverter::MaterialSetTexture(const int baseColorIndex, MaterialInfo& materialInfo, const std::string& textureType) const
{
	const auto baseColor = m_model->textures[baseColorIndex];
	const auto baseImage = m_model->images[baseColor.source];

	auto baseColorPath = m_output.parent_path() / baseImage.uri;
	baseColorPath.replace_extension(".tx");
	baseColorPath = m_state.ConvertToExportRelative(baseColorPath);

	materialInfo.textures[textureType] = baseColorPath.string();
}

std::string GltfConverter::CalculateMeshName(const int meshIndex, const int primitiveIndex) const
{
	auto name = "MESH_" + std::to_string(meshIndex) + "_" + m_model->meshes[meshIndex].name;
	if (m_model->meshes[meshIndex].primitives.size() > 1)
	{
		name += "_PRIM_" + std::to_string(primitiveIndex);
	}
	return name;
}

std::string GltfConverter::CalculateMaterialName(const int materialIndex) const
{
	return "MAT_" + std::to_string(materialIndex) + "_" + m_model->materials[materialIndex].name;
}
