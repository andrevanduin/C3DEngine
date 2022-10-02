
#pragma once
#include "core/defines.h"
#include "texture.h"
#include "geometry.h"

namespace C3D
{
	struct Skybox
	{
		TextureMap cubeMap;
		Geometry* g;

		u32 instanceId;
		/* @brief Synced to the renderer's current frame number
		 * when the material has been applied for that frame. */
		u64 frameNumber;
	};
}