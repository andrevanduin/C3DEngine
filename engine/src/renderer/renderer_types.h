
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/uuid.h"
#include "renderer/geometry.h"

namespace C3D
{
    class RendererPlugin;
    class SystemManager;
    class Skybox;

    /** @brief The Renderer Plugin type. */
    enum class RendererPluginType
    {
        Unknown,
        Vulkan,
        OpenGl,
        DirectX,
    };

    enum RendererViewMode : u8
    {
        Default  = 0,
        Lighting = 1,
        Normals  = 2,
    };

    enum RendererConfigFlagBits : u8
    {
        /** @brief Sync frame rate to monitor refresh rate. */
        FlagVSyncEnabled = 0x1,
        /** @brief Configure renderer to try to save power wherever possible (useful when on battery power for example). */
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

    /** @brief The winding order of the vertices, used to determine what the front-face of a triangle is. */
    enum class RendererWinding : u8
    {
        /** @brief The default counter-clockwise direction. */
        CounterClockwise,
        /** @brief The clockwise direction. */
        Clockwise,
    };

    /** @brief The type of projection matrix used. */
    enum class RendererProjectionMatrixType : u8
    {
        /** @brief A perspective matrix is being used. */
        Perspective,
        /** @brief An orthographic matrix that is zero-based on the top left. */
        Orthographic,
        /** @brief An orthographic matrix that is centered around width/height instead of zero-based.
         * @note This will use the fov as sort of a "zoom".
         */
        OrthographicCentered,
    };

    /** @brief The stage that a Shader is used for. */
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

    struct GeometryRenderData
    {
        GeometryRenderData() {}

        explicit GeometryRenderData(const Geometry* geometry) : geometry(geometry) {}

        GeometryRenderData(const mat4& model, const Geometry* geometry, const UUID uuid, bool windingInverted = false)
            : model(model), geometry(geometry), uuid(uuid), windingInverted(windingInverted)
        {}

        UUID uuid;
        mat4 model;
        const Geometry* geometry = nullptr;
        bool windingInverted     = false;
    };

    struct UIProperties
    {
        vec4 diffuseColor;
    };

    struct UIRenderData
    {
        GeometryRenderData geometryData;
        UIProperties properties;

        u16 depth = 0;
        u32 instanceId = INVALID_ID;

        u64* pFrameNumber = nullptr;
        u8* pDrawIndex    = nullptr;
    };
}  // namespace C3D
