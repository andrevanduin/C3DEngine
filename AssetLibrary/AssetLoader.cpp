
#include "AssetLoader.h"

#include <fstream>
#include <iostream>

bool Assets::SaveBinary(const std::string& path, const AssetFile& file)
{
	std::ofstream outFile;
	outFile.open(path, std::ios::binary | std::ios::out);

	if (!outFile.is_open())
	{
		std::cout << "Failed to open file: " << path << std::endl;
		return false;
	}

	// Write the type
	constexpr std::streamsize typeSize = sizeof(file.type) / sizeof(char);
	outFile.write(file.type, typeSize);

	// Write the version number
	outFile.write(reinterpret_cast<const char*>(&file.version), sizeof(uint32_t));

	// Write the lengths of the json and the binary blob
	const auto jsonLength = static_cast<uint32_t>(file.json.size());
	const auto blobLength = static_cast<uint32_t>(file.binaryBlob.size());

	outFile.write(reinterpret_cast<const char*>(&jsonLength), sizeof(uint32_t));
	outFile.write(reinterpret_cast<const char*>(&blobLength), sizeof(uint32_t));

	// Write the actual json and binary blob
	outFile.write(file.json.data(), jsonLength);
	outFile.write(file.binaryBlob.data(), blobLength);

	outFile.close();

	return true;
}

bool Assets::LoadBinary(const std::string& path, AssetFile& outputFile)
{
	std::ifstream inFile;
	inFile.open(path, std::ios::binary);

	if (!inFile.is_open())
	{
		std::cout << "Failed to open file: " << path << std::endl;
		return false;
	}

	inFile.seekg(0);

	inFile.read(outputFile.type, 4);
	inFile.read(reinterpret_cast<char*>(&outputFile.version), sizeof(uint32_t));

	uint32_t jsonLength = 0;
	inFile.read(reinterpret_cast<char*>(&jsonLength), sizeof(uint32_t));

	uint32_t blobLength = 0;
	inFile.read(reinterpret_cast<char*>(&blobLength), sizeof(uint32_t));

	outputFile.json.resize(jsonLength);
	inFile.read(outputFile.json.data(), jsonLength);

	outputFile.binaryBlob.resize(blobLength);
	inFile.read(outputFile.binaryBlob.data(), blobLength);

	return true;
}

Assets::CompressionMode Assets::ParseCompression(const std::string& compression)
{
	if (compression == "Lz4")
	{
		return CompressionMode::Lz4;
	}
	return CompressionMode::None;
}
