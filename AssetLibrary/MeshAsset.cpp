
#include "MeshAsset.h"

#include <nlohmann/json.hpp>
#include <lz4.h>

Assets::VertexFormat Assets::ParseFormat(const std::string& format)
{
	if (format == "F32") return VertexFormat::F32;
	if (format == "P32N8C8V16") return VertexFormat::P32N8C8V16;

	return VertexFormat::Unknown;
}

Assets::MeshInfo Assets::ReadMeshInfo(AssetFile* file)
{
	MeshInfo info;

	auto metaData = nlohmann::json::parse(file->json);

	info.vertexBufferSize = metaData["vertexBufferSize"];
	info.indexBufferSize = metaData["indexBufferSize"];
	info.indexSize = static_cast<uint8_t>(metaData["indexSize"]);
	info.originalFile = metaData["originalFile"];
	info.compressionMode = ParseCompression(metaData["compression"]);

	std::vector<float> boundsData;
	boundsData.reserve(7);
	boundsData = metaData["bounds"].get<std::vector<float>>();

	info.bounds.origin[0] = boundsData[0];
	info.bounds.origin[1] = boundsData[1];
	info.bounds.origin[2] = boundsData[2];

	info.bounds.radius = boundsData[3];

	info.bounds.extents[0] = boundsData[4];
	info.bounds.extents[1] = boundsData[5];
	info.bounds.extents[2] = boundsData[6];

	info.vertexFormat = ParseFormat(metaData["vertexFormat"]);
	return info;
}

void Assets::UnpackMesh(const MeshInfo* info, const char* sourceBuffer, const size_t sourceSize, char* vertexBuffer, char* indexBuffer)
{
	// TODO: Directly stream the decompressed data into the correct buffers instead of using a temporary buffer
	std::vector<char> decompressedBuffer;
	decompressedBuffer.resize(info->vertexBufferSize + info->indexBufferSize);

	LZ4_decompress_safe(sourceBuffer, decompressedBuffer.data(), static_cast<int>(sourceSize), static_cast<int>(decompressedBuffer.size()));

	// Copy vertex buffer
	memcpy(vertexBuffer, decompressedBuffer.data(), info->vertexBufferSize);

	// Copy index buffer
	memcpy(indexBuffer, decompressedBuffer.data() + info->vertexBufferSize, info->indexBufferSize);
}

Assets::AssetFile Assets::PackMesh(MeshInfo* info, const char* vertexData, const char* indexData)
{
	AssetFile file;
	file.type[0] = 'M';
	file.type[1] = 'E';
	file.type[2] = 'S';
	file.type[3] = 'H';
	file.version = 1;

	nlohmann::json metadata;
	if (info->vertexFormat == VertexFormat::P32N8C8V16) 
	{
		metadata["vertexFormat"] = "P32N8C8V16";
	}
	else if (info->vertexFormat == VertexFormat::F32)
	{
		metadata["vertexFormat"] = "F32";
	}

	metadata["vertexBufferSize"] = info->vertexBufferSize;
	metadata["indexBufferSize"] = info->indexBufferSize;
	metadata["indexSize"] = info->indexSize;
	metadata["originalFile"] = info->originalFile;

	std::vector<float> boundsData;
	boundsData.resize(7);

	boundsData[0] = info->bounds.origin[0];
	boundsData[1] = info->bounds.origin[1];
	boundsData[2] = info->bounds.origin[2];

	boundsData[3] = info->bounds.radius;

	boundsData[4] = info->bounds.extents[0];
	boundsData[5] = info->bounds.extents[1];
	boundsData[6] = info->bounds.extents[2];

	metadata["bounds"] = boundsData;

	const size_t fullSize = info->vertexBufferSize + info->indexBufferSize;

	std::vector<char> mergedBuffer;
	mergedBuffer.resize(fullSize);

	// Copy vertex buffer
	memcpy(mergedBuffer.data(), vertexData, info->vertexBufferSize);

	// Copy index buffer
	memcpy(mergedBuffer.data() + info->vertexBufferSize, indexData, info->indexBufferSize);

	// Compress buffer and copy it into the file struct
	const size_t compressStaging = LZ4_compressBound(static_cast<int>(fullSize));
	file.binaryBlob.resize(compressStaging);

	const int compressedSize = LZ4_compress_default(mergedBuffer.data(), file.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging));
	file.binaryBlob.resize(compressedSize);

	metadata["compression"] = "Lz4";

	file.json = metadata.dump();
	return file;
}

Assets::MeshBounds Assets::CalculateBounds(const std::vector<VertexF32>& vertices)
{
	MeshBounds bounds{};

	float min[3] = { std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max() };
	float max[3] = { std::numeric_limits<float>::min(),std::numeric_limits<float>::min(),std::numeric_limits<float>::min() };

	for (const auto& vertex : vertices)
	{
		min[0] = std::min(min[0], vertex.position[0]);
		min[1] = std::min(min[1], vertex.position[1]);
		min[2] = std::min(min[2], vertex.position[2]);

		max[0] = std::max(max[0], vertex.position[0]);
		max[1] = std::max(max[1], vertex.position[1]);
		max[2] = std::max(max[2], vertex.position[2]);
	}

	for (auto i = 0; i < 3; i++)
	{
		bounds.extents[i] = (max[i] - min[i]) / 2.0f;
		bounds.origin[i] = bounds.extents[i] + min[i];
	}

	float r2 = 0;
	for (const auto& vertex : vertices)
	{
		float offset[3]{};
		offset[0] = vertex.position[0] - bounds.origin[0];
		offset[1] = vertex.position[1] - bounds.origin[1];
		offset[2] = vertex.position[2] - bounds.origin[2];

		// Pythagoras to get the distance^2
		float d2 = offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
		// Store the largest r^2
		r2 = std::max(r2, d2);
	}

	// The radius is sqrt of the largest r^2
	bounds.radius = std::sqrt(r2);
	return bounds;
}


