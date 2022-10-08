
#pragma once
#include "core/application.h"

int main(int argc, char** argv)
{
	// Create our application (game) by calling the user supplied method
	const auto app = C3D::CreateApplication();
	app->Run();
	// Cleanup the main application which was created by the user in the CreateApplication
	delete app;
	// Cleanup our memory
	Memory.Shutdown();
	return 0;
}