
#pragma once
#include <vector>
#include <string>

namespace Assets
{
	struct AssetFile
	{
		char type[4];
		int version;
		std::string json;
		std::vector<char> binaryBlob;
	};

	enum class CompressionMode : uint32_t
	{
		None, Lz4
	};

	bool SaveBinary(const std::string& path, const AssetFile& file);

	bool LoadBinary(const std::string& path, AssetFile& outputFile);

	CompressionMode ParseCompression(const std::string& compression);
}