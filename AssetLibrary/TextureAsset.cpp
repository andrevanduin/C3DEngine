
#include "TextureAsset.h"
#include <lz4.h>
#include <nlohmann/json.hpp>

Assets::TextureFormat Assets::ParseFormat(const std::string& format)
{
	if (format == "Rgba8")
	{
		return TextureFormat::Rgba8;
	}

	return TextureFormat::Unknown;

}

Assets::TextureInfo Assets::ReadTextureInfo(AssetFile* file)
{
	TextureInfo info;

	auto textureMetadata = nlohmann::json::parse(file->json);

	// Parse the Format, Compression, TextureSize and Original File name from the metadata
	info.textureFormat = ParseFormat(textureMetadata["format"]);
	info.compressionMode = ParseCompression(textureMetadata["compression"]);
	info.textureSize = textureMetadata["bufferSize"];
	info.originalFile = textureMetadata["originalFile"];

	for (auto& [key, value] : textureMetadata["pages"].items())
	{
		PageInfo page
		{
			value["width"],
			value["height"],
			value["compressedSize"],
			value["originalSize"]
		};
		info.pages.push_back(page);
	}

	return info;
}

void Assets::UnpackTexture(const TextureInfo* info, const char* sourceBuffer, const size_t sourceSize, char* destination)
{
	if (info->compressionMode == CompressionMode::Lz4)
	{
		for (auto& page : info->pages)
		{
			LZ4_decompress_safe(sourceBuffer, destination, page.compressedSize, page.originalSize);
			sourceBuffer += page.compressedSize;
			destination += page.originalSize;
		}
	}
	else
	{
		memcpy(destination, sourceBuffer, sourceSize);
	}
}

void Assets::UnpackTexturePage(const TextureInfo* info, const int pageIndex, const char* sourceBuffer, char* destination)
{
	auto source = sourceBuffer;
	for (int i = 0; i < pageIndex; i++)
	{
		source += info->pages[i].compressedSize;
	}

	if (info->compressionMode == CompressionMode::Lz4)
	{
		if (info->pages[pageIndex].compressedSize != info->pages[pageIndex].originalSize)
		{
			LZ4_decompress_safe(source, destination, info->pages[pageIndex].compressedSize, info->pages[pageIndex].originalSize);
		}
		else
		{
			memcpy(destination, source, info->pages[pageIndex].originalSize);
		}
	}
	else
	{
		memcpy(destination, source, info->pages[pageIndex].originalSize);
	}
}

Assets::AssetFile Assets::PackTexture(TextureInfo* info, void* pixelData)
{
	AssetFile file;
	file.type[0] = 'T';
	file.type[1] = 'E';
	file.type[2] = 'X';
	file.type[3] = 'I';
	file.version = 1;

	auto pixels = static_cast<char*>(pixelData);
	std::vector<char> pageBuffer;

	for (auto& page : info->pages)
	{
		pageBuffer.resize(page.originalSize);

		const int compressStaging = LZ4_compressBound(page.originalSize);
		pageBuffer.resize(compressStaging);

		int compressedSize = LZ4_compress_default(pixels, pageBuffer.data(), page.originalSize, compressStaging);

		const auto compressionRate = static_cast<float>(compressedSize) / static_cast<float>(info->textureSize);
		// The compressed size is more than 80% of the original size so it's not worth it to use compression
		if (compressionRate > 0.8f)
		{
			compressedSize = page.originalSize;
			pageBuffer.resize(compressedSize);

			memcpy(pageBuffer.data(), pixels, compressedSize);
		}
		else
		{
			pageBuffer.resize(compressedSize);
		}

		page.compressedSize = compressedSize;

		file.binaryBlob.insert(file.binaryBlob.end(), pageBuffer.begin(), pageBuffer.end());

		// Advance the pixel pointer to the next page
		pixels += page.originalSize;
	}

	nlohmann::json textureMetadata;
	textureMetadata["format"] = "Rgba8";
	textureMetadata["bufferSize"] = info->textureSize;
	textureMetadata["originalFile"] = info->originalFile;
	textureMetadata["compression"] = "Lz4";

	std::vector<nlohmann::json> pageJson;
	for (auto& p : info->pages)
	{
		nlohmann::json page;
		page["compressedSize"] = p.compressedSize;
		page["originalSize"] = p.originalSize;
		page["width"] = p.width;
		page["height"] = p.height;
		pageJson.push_back(page);
	}
	textureMetadata["pages"] = pageJson;

	// Dump the metadata into a json string
	file.json = textureMetadata.dump();
	return file;
}
