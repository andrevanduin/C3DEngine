
#include <entry.h>
#include "game.h"

const C3D::ApplicationConfig CONFIG = { "Test Game", 640, 360, 1280, 720, { MebiBytes(1024) } };

C3D::Application* C3D::CreateApplication()
{
	return new TestEnv(CONFIG);
}
