
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "render_view.h"

struct SDL_Window;

namespace C3D
{
    class RendererPlugin;
    class SystemManager;

    enum class RendererPluginType
    {
        Unknown,
        Vulkan,
        OpenGl,
        DirectX,
    };

    enum RendererViewMode : i32
    {
        Default = 0,
        Lighting = 1,
        Normals = 2,
    };

    struct RenderPacket
    {
        DynamicArray<RenderViewPacket, LinearAllocator> views;
    };

    enum RendererConfigFlagBits : u8
    {
        /* Sync frame rate to monitor refresh rate. */
        FlagVSyncEnabled = 0x1,
        /* Configure renderer to try to save power wherever possible (useful when on battery power for example). */
        FlagPowerSavingEnabled = 0x2,
    };

    typedef u8 RendererConfigFlags;

    struct RendererPluginConfig
    {
        const char* applicationName;
        u32 applicationVersion;

        RendererConfigFlags flags;

        SDL_Window* pWindow;
        const SystemManager* pSystemsManager;
    };

    enum class ShaderStage
    {
        Vertex,
        Geometry,
        Fragment,
        Compute
    };
}  // namespace C3D
