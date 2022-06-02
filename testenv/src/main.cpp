
#include <entry.h>
#include <core/memory.h>

#include "game.h"

const C3D::ApplicationConfig CONFIG = { "Test Game", 640, 360, 1280, 720 };

C3D::Application* C3D::CreateApplication()
{
	return new TestEnv(CONFIG);
}
