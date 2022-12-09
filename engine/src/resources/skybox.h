
#pragma once
#include "core/defines.h"
#include "texture.h"
#include "geometry.h"

namespace C3D
{
	class C3D_API Skybox
	{
	public:
		Skybox();

		bool Create(const char* cubeMapName);

		void Destroy();

		TextureMap cubeMap;
		Geometry* g;

		u32 instanceId;
		/* @brief Synced to the renderer's current frame number
		 * when the material has been applied for that frame. */
		u64 frameNumber;
	};
}