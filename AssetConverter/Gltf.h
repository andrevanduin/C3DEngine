
#pragma once
#include <filesystem>
#include <MeshAsset.h>
#include <MaterialAsset.h>

#include "ConverterState.h"

namespace tinygltf
{
	class Model;
	struct Accessor;
	struct Primitive;
}

class GltfConverter
{
public:
	bool LoadFromAscii(const std::filesystem::directory_entry& directory, const std::filesystem::path& outputPath);
private:
	void ExtractMeshes();
	void ExtractMaterials();
	void ExtractNodes();

	void ExtractIndices(const tinygltf::Primitive& primitive, std::vector<uint32_t>& indices);
	void ExtractVertices(tinygltf::Primitive& primitive, std::vector<Assets::VertexF32>& vertices);

	void UnpackBuffer(const tinygltf::Accessor& accessor, std::vector<uint8_t>& outputBuffer) const;

	void MaterialSetTexture(int baseColorIndex, Assets::MaterialInfo& materialInfo, const std::string& textureType) const;

	[[nodiscard]] std::string CalculateMeshName(int meshIndex, int primitiveIndex) const;
	[[nodiscard]] std::string CalculateMaterialName(int materialIndex) const;

	tinygltf::Model* m_model{};
	std::filesystem::path m_input;
	std::filesystem::path m_output;

	ConverterState m_state;
};
