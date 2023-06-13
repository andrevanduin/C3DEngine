
#include <entry.h>
#include <core/plugin/plugin.h>

#include <math/frustum.h>
#include <resources/mesh.h>

#include "core/dynamic_library/game_library.h"


namespace C3D
{
	class Camera;
}

static C3D::Plugin rendererPlugin;
static C3D::GameLibrary applicationLib;
static C3D::ApplicationState* applicationState;

// Create our actual application
C3D::Application* C3D::CreateApplication()
{
	if (!rendererPlugin.Load("C3DVulkanRenderer"))
	{
		throw std::exception("Failed to load Vulkan Renderer plugin");
	}

	if (!applicationLib.Load("TestEnvLib"))
	{
		throw std::exception("Failed to load TestEnv library");
	}

	applicationState = applicationLib.CreateState();
	applicationState->rendererPlugin = rendererPlugin.Create<RendererPlugin>();

	return applicationLib.Create(applicationState);
}

// Method that gets called when our game library has updated
bool C3D::OnGameLibraryUpdated()
{
	return true;
}