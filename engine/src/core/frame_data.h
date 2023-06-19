
#pragma once
#include "core/defines.h"

namespace C3D
{
    class LinearAllocator;

    struct ApplicationFrameData
    {
    };

    struct FrameData
    {
        /** @brief The time in seconds since the last frame. */
        f64 deltaTime;
        /** @brief The total amount of time in seconds that the application has been running. */
        f64 totalTime;
        /** @brief A pointer to the engine's frame allocator. */
        LinearAllocator* frameAllocator;
        /** @brief A pointer to application specific frame data. Optional and up to the application to use. */
        ApplicationFrameData* applicationFrameData;
    };
}  // namespace C3D