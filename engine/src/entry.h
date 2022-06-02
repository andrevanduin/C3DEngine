
#pragma once
#include "core/application.h"

int main(int argc, char** argv)
{
	// Create our application (game) by calling the user supplied method
	const auto app = C3D::CreateApplication();
	app->Run();
	// Cleanup the allocation that the user did
	delete app;
	return 0;
}