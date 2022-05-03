
#pragma once
#include <unordered_map>
#include "AssetLoader.h"

namespace Assets
{
	struct PrefabInfo
	{
		std::unordered_map<uint64_t, int> nodeMatrices;
		std::unordered_map<uint64_t, std::string> nodeNames;

		std::unordered_map<uint64_t, uint64_t> nodeParents;

		struct NodeMesh
		{
			std::string materialPath;
			std::string meshPath;
		};

		std::unordered_map<uint64_t, NodeMesh> nodeMeshes;
		std::vector<std::array<float, 16>> matrices;
	};

	PrefabInfo ReadPrefabInfo(AssetFile* file);
	AssetFile PackPrefab(const PrefabInfo& info);
}