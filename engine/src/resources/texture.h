
#pragma once
#include "core/c3d_string.h"
#include "core/defines.h"

namespace C3D
{
	constexpr auto TEXTURE_NAME_MAX_LENGTH = 256;

	enum class TextureUse
	{
		Unknown = 0x0,
		Diffuse = 0x1,
		Specular = 0x2,
		Normal = 0x3,
	};

	enum class TextureFilter
	{
		ModeNearest = 0x0,
		ModeLinear  = 0x1
	};

	enum class TextureRepeat
	{
		Repeat = 0x1,
		MirroredRepeat = 0x2,
		ClampToEdge = 0x3,
		ClampToBorder = 0x4
	};

	struct Texture
	{
		Texture()
			: id(0), width(0), height(0), channelCount(0), hasTransparency(false), isWritable(false), name(), generation(0), internalData(nullptr)
		{}

		Texture(const char* textureName, const u32 width, const u32 height, const u8 channelCount, const bool hasTransparency, const bool isWritable)
			: id(INVALID_ID), width(width), height(height), channelCount(channelCount), hasTransparency(hasTransparency), isWritable(isWritable), name(),
			  generation(INVALID_ID), internalData(nullptr)
		{
			StringNCopy(name, textureName, TEXTURE_NAME_MAX_LENGTH);
		}

		u32 id;
		u32 width, height;

		u8 channelCount;
		bool hasTransparency;
		bool isWritable;

		char name[TEXTURE_NAME_MAX_LENGTH];

		u32 generation;
		void* internalData;
	};

	struct TextureMap
	{
		TextureMap()
			: texture(nullptr), use(), minifyFilter(), magnifyFilter(), repeatU(), repeatV(), repeatW(), internalData(nullptr)
		{}

		TextureMap(const TextureUse use, const TextureFilter minifyFilter, const TextureFilter magnifyFilter, const TextureRepeat repeatU, const TextureRepeat repeatV, const TextureRepeat repeatW)
			: texture(nullptr), use(use), minifyFilter(minifyFilter), magnifyFilter(magnifyFilter), repeatU(repeatU), repeatV(repeatV), repeatW(repeatW), internalData(nullptr)
		{}

		TextureMap(const TextureUse use, const TextureFilter minifyFilter, const TextureFilter magnifyFilter, const TextureRepeat repeat)
			: texture(nullptr), use(use), minifyFilter(minifyFilter), magnifyFilter(magnifyFilter), repeatU(repeat), repeatV(repeat), repeatW(repeat), internalData(nullptr)
		{}

		TextureMap(const TextureUse use, const TextureFilter filter, const TextureRepeat repeat)
			: texture(nullptr), use(use), minifyFilter(filter), magnifyFilter(filter), repeatU(repeat), repeatV(repeat), repeatW(repeat), internalData(nullptr)
		{
			
		}

		// Pointer to the corresponding texture
		Texture* texture;
		// Use of the texture
		TextureUse use;
		// Texture filtering mode for minification
		TextureFilter minifyFilter;
		// Texture filtering mode for magnification
		TextureFilter magnifyFilter;
		// Texture repeat mode on the U axis
		TextureRepeat repeatU;
		// Texture repeat mode on the V axis
		TextureRepeat repeatV;
		// Texture repeat mode on the W axis
		TextureRepeat repeatW;
		// A pointer to internal, render API-specific data. Typically the internal sampler.
		void* internalData;
	};
}

