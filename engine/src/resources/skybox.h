
#pragma once
#include "core/defines.h"
#include "texture.h"
#include "geometry.h"

namespace C3D
{
	class Engine;

	class C3D_API Skybox
	{
	public:
		Skybox();

		bool Create(const Engine* engine, const char* cubeMapName);

		void Destroy();

		TextureMap cubeMap;
		Geometry* g;

		u32 instanceId;
		/* @brief Synced to the renderer's current frame number
		 * when the material has been applied for that frame. */
		u64 frameNumber;

	private:
		const Engine* m_engine;
	};
}