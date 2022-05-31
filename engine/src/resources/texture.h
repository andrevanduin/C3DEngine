
#pragma once
#include "core/defines.h"

namespace C3D
{
	constexpr auto TEXTURE_NAME_MAX_LENGTH = 256;

	enum class TextureUse
	{
		Unknown = 0x0,
		Diffuse = 0x1,
	};

	struct Texture
	{
		u32 id;
		u32 width, height;

		u8 channelCount;
		bool hasTransparency;

		char name[TEXTURE_NAME_MAX_LENGTH];

		u32 generation;
		void* internalData;
	};

	struct TextureMap
	{
		Texture* texture;
		TextureUse use;
	};
}

