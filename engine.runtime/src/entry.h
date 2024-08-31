
#pragma once
#include "engine.h"
#include "memory/global_memory_system.h"
#include "metrics/metrics.h"

int main(int argc, char** argv)
{
    // Initialize our logger which we should do first to ensure we can log errors everywhere
    C3D::Logger::Init();

    // Initialize the platform layer
    C3D::Platform::Init();

    // Initialize our metrics to track our memory usage and other stats
    Metrics.Init();

    // Initialize our global allocator that we will normally always use
    C3D::GlobalMemorySystem::Init({ MebiBytes(1024) });

    // Create the application by calling the method provided by the user
    const auto application = C3D::CreateApplication();

    // Create our instance of the engine and supply it with the user's application
    const auto engine = Memory.New<C3D::Engine>(C3D::MemoryType::Engine, application);

    // Initialize our engine
    if (engine->Init())
    {
        // Init our application
        InitApplication(engine);

        // Run our engine's game loop
        engine->Run();
    }

    // Cleanup our engine
    Memory.Delete(engine);

    // Call the user's cleanup method
    C3D::DestroyApplication();

    // Shutdown the platform layer
    C3D::Platform::Shutdown();

    // Cleanup our global memory system
    C3D::GlobalMemorySystem::Destroy();
    return 0;
}
