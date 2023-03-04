
#pragma once
#include <stack>

#pragma warning(push, 0)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include <vulkan/vulkan_core.h>

#include "defines.h"
#include "asserts.h"

#include "containers/cstring.h"

namespace C3D
{
	class C3D_API Logger
	{
		#define ASSERT_INIT C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!")

	public:
		static void Init();

		template <class... Args>
		static void Debug(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = fmt::vformat(format, fmt::make_format_args(args...));
			s_coreLogger->debug(str);
		}

		template <class... Args>
		static void Trace(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = fmt::vformat(format, fmt::make_format_args(args...));
			s_coreLogger->trace(str);
		}

		template <class... Args>
		static void Info(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = fmt::vformat(format, fmt::make_format_args(args...));
			s_coreLogger->info(str);
		}

		template <class... Args>
		static void Warn(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = fmt::vformat(format, fmt::make_format_args(args...));
			s_coreLogger->warn(str);
		}

		template <class... Args>
		static void Error(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = fmt::vformat(format, fmt::make_format_args(args...));
			s_coreLogger->error(str);
		}

		template <class... Args>
		static void Fatal(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = fmt::vformat(format, fmt::make_format_args(args...));
			s_coreLogger->critical(str);
		}

		static VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_coreLogger; }
	private:
		static bool m_initialized;

		static std::shared_ptr<spdlog::logger> s_coreLogger;
	};

	class LoggerInstance
	{
	public:
		explicit LoggerInstance(std::string prefix) : m_prefix(std::move(prefix)) {}

		template <class... Args>
		void Debug(const char* format, Args&& ... args) const
		{
			const auto formatStr = fmt::vformat("[{}] - {}", fmt::make_format_args(m_prefix, format));
			Logger::Debug(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Trace(const char* format, Args&& ... args) const
		{
			const auto formatStr = fmt::vformat("[{}] - {}", fmt::make_format_args(m_prefix, format));
			Logger::Trace(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Info(const char* format, Args&& ... args) const
		{
			const auto formatStr = fmt::vformat("[{}] - {}", fmt::make_format_args(m_prefix, format));
			Logger::Info(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Warn(const char* format, Args&& ... args) const
		{
			const auto formatStr = fmt::vformat("[{}] - {}", fmt::make_format_args(m_prefix, format));
			Logger::Warn(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Error(const char* format, Args&& ... args) const
		{
			const auto formatStr = fmt::vformat("[{}] - {}", fmt::make_format_args(m_prefix, format));
			Logger::Error(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Fatal(const char* format, Args&& ... args) const
		{
			const auto formatStr = fmt::vformat("[{}] - {}", fmt::make_format_args(m_prefix, format));
			Logger::Fatal(formatStr.data(), std::forward<Args>(args)...);
		}

	private:
		std::string m_prefix;
	};
}
