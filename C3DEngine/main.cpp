
#include "core/VkEngine.h"

int main(int argc, char* argv[])
{
	C3D::VulkanEngine engine;

	engine.Init();
	engine.Run();
	engine.Cleanup();

	return 0;
}
