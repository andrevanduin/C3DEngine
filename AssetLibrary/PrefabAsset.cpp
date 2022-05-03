
#include "PrefabAsset.h"

#include <nlohmann/json.hpp>

Assets::PrefabInfo Assets::ReadPrefabInfo(AssetFile* file)
{
	PrefabInfo info;
	auto metadata = nlohmann::json::parse(file->json);

	for (auto& [key, value] : metadata["nodeMatrices"].items())
	{
		info.nodeMatrices[value[0]] = value[1];
	}

	for (auto& [key, value] : metadata["nodeNames"].items())
	{
		info.nodeNames[value[0]] = value[1];
	}

	for (auto& [key, value] : metadata["nodeParents"].items())
	{
		info.nodeParents[value[0]] = value[1];
 	}

	std::unordered_map<uint64_t, nlohmann::json> meshNodes = metadata["nodeMeshes"];
	for (auto& [key, value] : meshNodes)
	{
		PrefabInfo::NodeMesh node;

		node.meshPath = value["meshPath"];
		node.materialPath = value["materialPath"];

		info.nodeMeshes[key] = node;
	}

	const size_t nMatrices = file->binaryBlob.size() / (sizeof(float) * 16);
	info.matrices.resize(nMatrices);

	memcpy(info.matrices.data(), file->binaryBlob.data(), file->binaryBlob.size());
	return info;
}

Assets::AssetFile Assets::PackPrefab(const PrefabInfo& info)
{
	nlohmann::json metadata;
	metadata["nodeMatrices"] = info.nodeMatrices;
	metadata["nodeNames"] = info.nodeNames;
	metadata["nodeParents"] = info.nodeParents;

	std::unordered_map<uint64_t, nlohmann::json> meshIndex;
	for (auto [key, value] : info.nodeMeshes)
	{
		nlohmann::json meshNode;
		meshNode["meshPath"] = value.meshPath;
		meshNode["materialPath"] = value.materialPath;
		meshIndex[key] = meshNode;
	}

	metadata["nodeMeshes"] = meshIndex;

	AssetFile file;
	file.type[0] = 'P';
	file.type[1] = 'R';
	file.type[2] = 'F';
	file.type[3] = 'B';
	file.version = 1;

	file.binaryBlob.resize(info.matrices.size() * sizeof(float) * 16);
	memcpy(file.binaryBlob.data(), info.matrices.data(), info.matrices.size() * sizeof(float) * 16);

	file.json = metadata.dump();
	return file;
}
