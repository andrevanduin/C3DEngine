
#pragma once
#include "containers/cstring.h"
#include "core/defines.h"

/**
 * @brief All the platform stuff that is generic for all platforms
 *
 */
namespace C3D
{
    using DynamicLibraryPrefix    = CString<8>;
    using DynamicLibraryExtension = CString<8>;
    using FileWatchId             = u32;

    enum class CopyFileStatus : u8
    {
        Success,
        NotFound,
        Locked,
        NoPermissions,
        Failed,
        Unknown,
    };

    struct PlatformSystemConfig
    {
        /** @brief The name of the application. */
        const char* applicationName;
        /** @brief The initial x and y position of the main window. */
        i32 x = 0, y = 0;
        /** @brief The initial width and height of the main window. */
        i32 width = 0, height = 0;
        /** @brief Boolean to specify if the engine should create a window. */
        bool makeWindow = true;
    };
}  // namespace C3D