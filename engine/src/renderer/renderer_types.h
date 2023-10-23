
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"

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
        Default  = 0,
        Lighting = 1,
        Normals  = 2,
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

        const SystemManager* pSystemsManager;
    };

    /** @brief The windinig order of the vertices, used to determine what the front-face of a triangle is. */
    enum class RendererWinding : u8
    {
        /** @brief The default counter-clockwise direction. */
        CounterClockwise,
        /** @brief The clockwise direction. */
        Clockwise,
    };

    enum class ShaderStage
    {
        Vertex,
        Geometry,
        Fragment,
        Compute
    };

    enum PrimitiveTopologyType : u16
    {
        PRIMITIVE_TOPOLOGY_TYPE_NONE           = 0,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST  = 1,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP = 2,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN   = 4,
        PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST      = 8,
        PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP     = 16,
        PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST     = 32,
        PRIMITIVE_TOPOLOGY_TYPE_MAX            = 64
    };

    using PrimitiveTopologyTypeBits = u16;
}  // namespace C3D
