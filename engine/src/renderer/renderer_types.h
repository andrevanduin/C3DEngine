
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
    struct TextureMap;

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
        Cascades = 3,
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
        None,
        Vertex,
        Geometry,
        Fragment,
        Compute
    };
    
    struct ShaderStageConfig
    {
        ShaderStage stage = ShaderStage::None;
        String name;
        String fileName;
        String source;
    };

    enum PrimitiveTopologyType : u16
    {
        PRIMITIVE_TOPOLOGY_TYPE_NONE           = 0x0,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST  = 0x1,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP = 0x2,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN   = 0x4,
        PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST      = 0x8,
        PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP     = 0x10,
        PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST     = 0x20,
        PRIMITIVE_TOPOLOGY_TYPE_MAX            = 0x40
    };
    using PrimitiveTopologyTypeBits = u16;

    struct GeometryRenderData
    {
        GeometryRenderData() {}

        GeometryRenderData(const UUID uuid, const Geometry* geometry, bool windingInverted = false)
            : uuid(uuid),
              vertexCount(geometry->vertexCount),
              vertexSize(geometry->vertexSize),
              vertexBufferOffset(geometry->vertexBufferOffset),
              indexCount(geometry->indexCount),
              indexSize(geometry->indexSize),
              indexBufferOffset(geometry->indexBufferOffset),
              material(geometry->material),
              windingInverted(windingInverted)
        {}

        GeometryRenderData(const Geometry* geometry, bool windingInverted = false)
            : vertexCount(geometry->vertexCount),
              vertexSize(geometry->vertexSize),
              vertexBufferOffset(geometry->vertexBufferOffset),
              indexCount(geometry->indexCount),
              indexSize(geometry->indexSize),
              indexBufferOffset(geometry->indexBufferOffset),
              material(geometry->material),
              windingInverted(windingInverted)
        {}

        GeometryRenderData(const UUID uuid, const mat4& model, u32 vertexCount, u32 vertexSize, u64 vertexBufferOffset, u32 indexCount,
                           u32 indexSize, u64 indexBufferOffset, Material* material = nullptr, bool windingInverted = false)
            : uuid(uuid),
              model(model),
              vertexCount(vertexCount),
              vertexSize(vertexSize),
              vertexBufferOffset(vertexBufferOffset),
              indexCount(indexCount),
              indexSize(indexSize),
              indexBufferOffset(indexBufferOffset),
              material(material),
              windingInverted(windingInverted)
        {}

        GeometryRenderData(const UUID uuid, const mat4& model, const Geometry* geometry, bool windingInverted = false)
            : uuid(uuid),
              model(model),
              vertexCount(geometry->vertexCount),
              vertexSize(geometry->vertexSize),
              vertexBufferOffset(geometry->vertexBufferOffset),
              indexCount(geometry->indexCount),
              indexSize(geometry->indexSize),
              indexBufferOffset(geometry->indexBufferOffset),
              material(geometry->material),
              windingInverted(windingInverted)
        {}

        UUID uuid;
        mat4 model;

        /** @brief The amount of vertices. */
        u32 vertexCount = 0;
        /** @brief The size of each vertex. */
        u32 vertexSize = 0;
        /** @brief The offset from the start of the vertex buffer where we need to start drawing. */
        u64 vertexBufferOffset = 0;

        /** @brief The amount of indices. */
        u32 indexCount = 0;
        /** @brief The size of each index. */
        u32 indexSize = 0;
        /** @brief The offset from the start of the index buffer where we need to start drawing. */
        u64 indexBufferOffset = 0;

        bool windingInverted = false;
        // TODO: Replace this with a material handle
        Material* material = nullptr;
    };

    struct UIProperties
    {
        vec4 diffuseColor;
    };

    struct UIRenderData
    {
        GeometryRenderData geometryData;
        UIProperties properties;

        u16 depth      = 0;
        u32 instanceId = INVALID_ID;

        u64* pFrameNumber = nullptr;
        u8* pDrawIndex    = nullptr;
        /** @brief Optional override for the used atlas. Will use default if this is left as nullptr. */
        TextureMap* atlas = nullptr;
    };

    enum class StencilOperation
    {
        /** @brief Keeps the current value. */
        Keep,
        /** @brief Sets the stencil buffer value to 0. */
        Zero,
        /** @brief Sets the stencil buffer value to _ref_, as specified in the function. */
        Replace,
        /** @brief Increments the current stencil buffer value. Clamps to the maximum representable unsigned value. */
        IncrementAndClamp,
        /** @brief Decrements the current stencil buffer value. Clamps to 0. */
        DecrementAndClamp,
        /** @brief Bitwise inverts the current stencil buffer value. */
        Invert,
        /** @brief Increments the current stencil buffer value. Wraps stencil buffer value to zero when incrementing the maximum
           representable unsigned value. */
        IncrementAndWrap,
        /** @brief Decrements the current stencil buffer value. Wraps stencil buffer value to the maximum representable unsigned value when
           decrementing a stencil buffer value of zero. */
        DecrementAndWrap,
    };

    enum class CompareOperation
    {
        /** @brief Specifies that the comparison always evaluates false. */
        Never,
        /** @brief Specifies that the comparison evaluates reference < test. */
        Less = 1,
        /** @brief Specifies that the comparison evaluates reference = test. */
        Equal = 2,
        /** @brief Specifies that the comparison evaluates reference <= test. */
        LessOrEqual = 3,
        /** @brief Specifies that the comparison evaluates reference > test. */
        Greater = 4,
        /** @brief Specifies that the comparison evaluates reference != test.*/
        NotEqual = 5,
        /** @brief Specifies that the comparison evaluates reference >= test. */
        GreaterOrEqual = 6,
        /** @brief Specifies that the comparison is always true. */
        Always = 7
    };
}  // namespace C3D
