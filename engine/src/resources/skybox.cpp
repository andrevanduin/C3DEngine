
#include "skybox.h"

#include "renderer/renderer_frontend.h"
#include "services/services.h"
#include "systems/geometry_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"

namespace C3D
{
	Skybox::Skybox()
		: g(nullptr), instanceId(INVALID_ID), frameNumber(INVALID_ID_U64)
	{}

	bool Skybox::Create(const char* cubeMapName)
	{
		cubeMap = TextureMap(TextureUse::CubeMap, TextureFilter::ModeLinear, TextureRepeat::ClampToEdge);

		// Acquire resources for our cube map
		if (!Renderer.AcquireTextureMapResources(&cubeMap))
		{
			Logger::Fatal("[SKYBOX] - Create() - Unable to acquire resources for cube map texture.");
			return false;
		}

		// Acquire our skybox texture
		cubeMap.texture = Textures.AcquireCube("skybox", true); // TODO: Replace this hardcoded name

		// Generate a config for some cube geometry
		auto skyboxCubeConfig = Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, cubeMapName, "");
		// Clear out the material name
		skyboxCubeConfig.materialName[0] = '\0';

		// Build the cube geometry based on the config
		g = Geometric.AcquireFromConfig(skyboxCubeConfig, true);

		// Get our builtin skybox shader
		const auto shader = Shaders.Get("Shader.Builtin.Skybox"); // TODO: Allow configurable shader here

		TextureMap* maps[1] = { &cubeMap };

		// Acquire our shader instance resources
		if (!Renderer.AcquireShaderInstanceResources(shader, maps, &instanceId))
		{
			Logger::Fatal("[SKYBOX] - Create() - Unable to acquire shader resources for skybox texture.");
			return false;
		}

		return true;
;	}

	void Skybox::Destroy()
	{
		Renderer.ReleaseTextureMapResources(&cubeMap);
	}
}
