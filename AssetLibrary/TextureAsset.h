
#pragma once
#include "AssetLoader.h"

namespace Assets
{
	enum class TextureFormat : uint32_t
	{
		Unknown = 0,
		Rgba8
	};

	struct PageInfo
	{
		uint32_t width;
		uint32_t height;
		uint32_t compressedSize;
		uint32_t originalSize;
	};

	struct TextureInfo
	{
		uint64_t textureSize;
		TextureFormat textureFormat;
		CompressionMode compressionMode;

		std::string originalFile;
		std::vector<PageInfo> pages;
	};

	TextureFormat ParseFormat(const std::string& format);

	TextureInfo ReadTextureInfo(AssetFile* file);

	void UnpackTexture(const TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination);

	void UnpackTexturePage(const TextureInfo* info, int pageIndex, const char* sourceBuffer, char* destination);

	AssetFile PackTexture(TextureInfo* info, void* pixelData);
}