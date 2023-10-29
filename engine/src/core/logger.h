
#pragma once

#include <spdlog/spdlog.h>

#include "asserts.h"
#include "defines.h"

namespace C3D
{
    class UIConsole;

    class C3D_API Logger
    {
#define ASSERT_INIT C3D_ASSERT_MSG(GetInitialized(), "Logger was used before it was initialized!")

    public:
        static void Init(UIConsole* console = nullptr);

        template <class... Args>
        static void Debug(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const auto str = fmt::vformat(format, fmt::make_format_args(args...));
            GetCoreLogger()->debug(str);
        }

        template <class... Args>
        static void Trace(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const auto str = fmt::vformat(format, fmt::make_format_args(args...));
            GetCoreLogger()->trace(str);
        }

        template <class... Args>
        static void Info(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const auto str = fmt::vformat(format, fmt::make_format_args(args...));
            GetCoreLogger()->info(str);
        }

        template <class... Args>
        static void Warn(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const auto str = fmt::vformat(format, fmt::make_format_args(args...));
            GetCoreLogger()->warn(str);
        }

        template <class... Args>
        static void Error(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const auto str = fmt::vformat(format, fmt::make_format_args(args...));
            GetCoreLogger()->error(str);
        }

        template <class... Args>
        static void Fatal(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const auto str = fmt::vformat(format, fmt::make_format_args(args...));
            GetCoreLogger()->critical(str);
            C3D_ASSERT_MSG(false, "Fatal Exception occured");
        }

    private:
        static bool& GetInitialized();
        static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    };

#ifdef C3D_LOG_DEBUG
#define INSTANCE_DEBUG_LOG(instance, format, ...)                                                                   \
    {                                                                                                               \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(instance, __func__, format)); \
        C3D::Logger::Debug(formatStr.data(), __VA_ARGS__);                                                          \
    }

#define DEBUG_LOG(format, ...)                                                                                           \
    {                                                                                                                    \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(INSTANCE_NAME, __func__, format)); \
        C3D::Logger::Debug(formatStr.data(), __VA_ARGS__);                                                               \
    }
#else
#define INSTANCE_TRACE(instance, format, ...)
#define DEBUG_LOG(format, ...)
#endif

#ifdef C3D_LOG_TRACE
#define INSTANCE_TRACE(instance, format, ...)                                                                       \
    {                                                                                                               \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(instance, __func__, format)); \
        C3D::Logger::Trace(formatStr.data(), __VA_ARGS__);                                                          \
    }

#define TRACE(format, ...)                                                                                               \
    {                                                                                                                    \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(INSTANCE_NAME, __func__, format)); \
        C3D::Logger::Trace(formatStr.data(), __VA_ARGS__);                                                               \
    }
#else
#define INSTANCE_TRACE(instance, format, ...)
#define TRACE(format, ...)
#endif

#ifdef C3D_LOG_ERROR
#define INSTANCE_ERROR_LOG(instance, format, ...)                                                                   \
    {                                                                                                               \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(instance, __func__, format)); \
        C3D::Logger::Error(formatStr.data(), __VA_ARGS__);                                                          \
    }

#define ERROR_LOG(format, ...)                                                                                           \
    {                                                                                                                    \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(INSTANCE_NAME, __func__, format)); \
        C3D::Logger::Error(formatStr.data(), ##__VA_ARGS__);                                                             \
    }
#else
#define INSTANCE_ERROR_LOG(instance, format, ...)
#define ERROR_LOG(format, ...)
#endif

#ifdef C3D_LOG_WARN
#define INSTANCE_WARN_LOG(instance, format, ...)                                                                    \
    {                                                                                                               \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(instance, __func__, format)); \
        C3D::Logger::Warn(formatStr.data(), __VA_ARGS__);                                                           \
    }

#define WARN_LOG(format, ...)                                                                                            \
    {                                                                                                                    \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(INSTANCE_NAME, __func__, format)); \
        C3D::Logger::Warn(formatStr.data(), __VA_ARGS__);                                                                \
    }
#else
#define INSTANCE_WARN_LOG(instance, format, ...)
#define WARN_LOG(format, ...)
#endif

#ifdef C3D_LOG_INFO
#define INSTANCE_INFO_LOG(instance, format, ...)                                                                    \
    {                                                                                                               \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(instance, __func__, format)); \
        C3D::Logger::Info(formatStr.data(), __VA_ARGS__);                                                           \
    }

#define INFO_LOG(format, ...)                                                                                            \
    {                                                                                                                    \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(INSTANCE_NAME, __func__, format)); \
        C3D::Logger::Info(formatStr.data(), __VA_ARGS__);                                                                \
    }
#else
#define INSTANCE_INFO_LOG(instance, format, ...)
#define INFO_LOG(format, ...)
#endif

#define INSTANCE_FATAL_LOG(instance, format, ...)                                                                   \
    {                                                                                                               \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(instance, __func__, format)); \
        C3D::Logger::Fatal(formatStr.data(), __VA_ARGS__);                                                          \
    }

#define FATAL_LOG(format, ...)                                                                                           \
    {                                                                                                                    \
        const auto formatStr = fmt::vformat("[{}] - {}() - {}", fmt::make_format_args(INSTANCE_NAME, __func__, format)); \
        C3D::Logger::Fatal(formatStr.data(), __VA_ARGS__);                                                               \
    }
}  // namespace C3D
