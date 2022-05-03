
#pragma once
#include "AssetLoader.h"

namespace Assets
{
	struct VertexF32
	{
		float position[3];
		float normal[3];
		float color[3];
		float uv[2];
	};

	struct VertexP32N8C8V16
	{
		float position[3];
		uint8_t normal[3];
		uint8_t color[3];
		float uv[2];
	};

	enum class VertexFormat : uint32_t
	{
		Unknown = 0,
		F32,			// All values at 32bits
		P32N8C8V16		// Position at 32bits, normal and color at 8 bits and uvs at 16 bits
	};

	struct MeshBounds
	{
		float origin[3];
		float radius;
		float extents[3];
	};

	struct MeshInfo
	{
		uint64_t vertexBufferSize;
		uint64_t indexBufferSize;
		MeshBounds bounds;
		VertexFormat vertexFormat;
		char indexSize;
		CompressionMode compressionMode;
		std::string originalFile;
	};

	VertexFormat ParseFormat(const std::string& format);

	MeshInfo ReadMeshInfo(AssetFile* file);

	void UnpackMesh(const MeshInfo* info, const char* sourceBuffer, size_t sourceSize, char* vertexBuffer, char* indexBuffer);

	AssetFile PackMesh(MeshInfo* info, const char* vertexData, const char* indexData);

	MeshBounds CalculateBounds(const std::vector<VertexF32>& vertices);
}