
#pragma once

#pragma warning(push, 0)
#undef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include "asserts.h"
#include "containers/cstring.h"
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
            const std::string str = std::vformat(format, std::make_format_args(args...));
            GetCoreLogger()->debug(str);
        }

        template <class... Args>
        static void Trace(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const std::string str = std::vformat(format, std::make_format_args(args...));
            GetCoreLogger()->trace(str);
        }

        template <class... Args>
        static void Info(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const std::string str = std::vformat(format, std::make_format_args(args...));
            GetCoreLogger()->info(str);
        }

        template <class... Args>
        static void Warn(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const std::string str = std::vformat(format, std::make_format_args(args...));
            GetCoreLogger()->warn(str);
        }

        template <class... Args>
        static void Error(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const std::string str = std::vformat(format, std::make_format_args(args...));
            GetCoreLogger()->error(str);
        }

        template <class... Args>
        static void Fatal(const char* format, Args&&... args)
        {
            ASSERT_INIT;
            const std::string str = std::vformat(format, std::make_format_args(args...));
            GetCoreLogger()->critical(str);
            C3D_ASSERT_MSG(false, "Fatal Exception occured");
        }

    private:
        static bool& GetInitialized();
        static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    };

    template <u64 PrefixSize>
    class LoggerInstance
    {
    public:
        explicit LoggerInstance(CString<PrefixSize> prefix) : prefix(std::move(prefix)) {}

        template <class... Args>
        void Debug(const char* format, Args&&... args) const
        {
            const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(prefix, format));
            Logger::Debug(formatStr.data(), std::forward<Args>(args)...);
        }

        template <class... Args>
        void Trace(const char* format, Args&&... args) const
        {
            const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(prefix, format));
            Logger::Trace(formatStr.data(), std::forward<Args>(args)...);
        }

        template <class... Args>
        void Info(const char* format, Args&&... args) const
        {
            const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(prefix, format));
            Logger::Info(formatStr.data(), std::forward<Args>(args)...);
        }

        template <class... Args>
        void Warn(const char* format, Args&&... args) const
        {
            const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(prefix, format));
            Logger::Warn(formatStr.data(), std::forward<Args>(args)...);
        }

        template <class... Args>
        void Error(const char* format, Args&&... args) const
        {
            const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(prefix, format));
            Logger::Error(formatStr.data(), std::forward<Args>(args)...);
        }

        template <class... Args>
        void Fatal(const char* format, Args&&... args) const
        {
            const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(prefix, format));
            Logger::Fatal(formatStr.data(), std::forward<Args>(args)...);
        }

        CString<PrefixSize> prefix;
    };
}  // namespace C3D
