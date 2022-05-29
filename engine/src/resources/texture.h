
#pragma once
#include "core/defines.h"

namespace C3D
{
	struct Texture
	{
		u32 id;
		u32 width, height;

		u8 channelCount;
		bool hasTransparency;

		u32 generation;
		void* internalData;
	};
}

