
#include <entry.h>
#include <vulkan_renderer_plugin.h>

#include "game.h"

C3D::Engine* C3D::CreateApplication()
{
	const auto vulkanRendererPlugin = Memory.New<VulkanRendererPlugin>(MemoryType::RenderSystem);

	ApplicationConfig config = {};
	config.name = "TestEnv";
	config.width = 1280;
	config.height = 720;
	config.x = 2560 / 2 - config.width / 2;
	config.y = 1440 / 2 - config.height / 2;
	config.rendererPlugin = vulkanRendererPlugin;

	return new TestEnv(config);
}
