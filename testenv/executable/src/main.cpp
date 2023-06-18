
#include <core/plugin/plugin.h>
#include <entry.h>
#include <math/frustum.h>
#include <resources/mesh.h>

#include "core/dynamic_library/game_library.h"

namespace C3D
{
    class Camera;
}

C3D::FileWatchId applicationLibraryFile;

C3D::Plugin rendererPlugin;
C3D::GameLibrary applicationLib;

C3D::Application* application;
C3D::ApplicationState* applicationState;

C3D::CString<32> libPath("TestEnvLib");
C3D::CString<32> loadedLibPath("TestEnvLib_loaded");

bool TryCopyLib()
{
    C3D::CString source(libPath);
    C3D::CString dest(loadedLibPath);
    source += C3D::Platform::GetDynamicLibraryExtension();
    dest += C3D::Platform::GetDynamicLibraryExtension();

    auto errorCode = C3D::CopyFileStatus::Locked;
    while (errorCode == C3D::CopyFileStatus::Locked)
    {
        errorCode = C3D::Platform::CopyFile(source.Data(), dest.Data(), true);
        if (errorCode == C3D::CopyFileStatus::Locked)
        {
            C3D::Platform::SleepMs(50);
        }
    }

    if (errorCode != C3D::CopyFileStatus::Success)
    {
        C3D::Logger::Error("Failed to copy Game library");
        return false;
    }

    C3D::Logger::Info("Copied Game library {} -> {}", source, dest);
    return true;
}

bool OnWatchedFileChanged(const u16, void* sender, const C3D::EventContext& context)
{
    const auto fileId = context.data.u32[0];
    if (fileId != applicationLibraryFile) return false;

    C3D::Logger::Info("Game Library was updated. Trying hot-reload");

    application->OnLibraryUnload();

    if (!applicationLib.Unload())
    {
        throw std::runtime_error("OnWatchedFileChanged() - Failed to unload Application library");
    }

    C3D::Platform::SleepMs(100);

    if (!TryCopyLib())
    {
        throw std::runtime_error("OnWatchedFileChanged() - Failed to copy Application library");
    }

    if (!applicationLib.Load(loadedLibPath.Data()))
    {
        throw std::runtime_error("OnWatchedFileChanged() - Failed to load Application library");
    }

    // On a reload we can just use our already existing state
    application = applicationLib.Create(applicationState);

    const auto engine = static_cast<C3D::Engine*>(sender);
    engine->OnApplicationLibraryReload(application);

    // Let other handlers potentially also handle this
    return false;
}

// Create our actual application
C3D::Application* C3D::CreateApplication()
{
    TryCopyLib();

    if (!rendererPlugin.Load("C3DVulkanRenderer"))
    {
        throw std::exception("Failed to load Vulkan Renderer plugin");
    }

    if (!applicationLib.Load(loadedLibPath.Data()))
    {
        throw std::exception("Failed to load TestEnv library");
    }

    // On the first start we need to create our state
    applicationState = applicationLib.CreateState();
    applicationState->rendererPlugin = rendererPlugin.Create<RendererPlugin>();

    application = applicationLib.Create(applicationState);
    return application;
}

// Method that gets called when the engine is fully initialized
void C3D::InitApplication(Engine* engine)
{
    engine->GetSystem<EventSystem>(EventSystemType)
        .Register(EventCodeWatchedFileChanged, [engine](const u16 code, void*, const EventContext& context) {
            return OnWatchedFileChanged(code, engine, context);
        });
    applicationLibraryFile = engine->GetSystem<Platform>(PlatformSystemType).WatchFile("TestEnvLib.dll");

    application->OnLibraryLoad();
}

void C3D::DestroyApplication()
{
    Memory.Delete(C3D::MemoryType::Game, applicationState);
    Memory.Delete(C3D::MemoryType::Game, application);

    applicationLib.Unload();
    rendererPlugin.Unload();
}