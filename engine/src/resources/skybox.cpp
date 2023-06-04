
#include "skybox.h"

#include "renderer/renderer_frontend.h"
#include "systems/system_manager.h"
#include "systems/geometry/geometry_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
	Skybox::Skybox()
		: g(nullptr), instanceId(INVALID_ID), frameNumber(INVALID_ID_U64), m_engine(nullptr)
	{}

	bool Skybox::Create(const Engine* engine, const char* cubeMapName)
	{
		m_engine = engine;
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
		const auto shader = Shaders.Get("Shader.Builtin.Skybox");

		Renderer.ReleaseTextureMapResources(&cubeMap);
		Renderer.ReleaseShaderInstanceResources(shader, instanceId);
	}
}
