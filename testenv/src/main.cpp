
#include <entry.h>
#include "game.h"



C3D::Application* C3D::CreateApplication()
{
	const ApplicationConfig config = { "Test Game", 640, 360, 1280, 720 };
	return new TestEnv(config);
}
