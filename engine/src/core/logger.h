
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
		#define ASSERT_INIT C3D_ASSERT_MSG(GetInitialized(), "Logger was used before it was initialized!")

	public:
		static void Init();

		template <class... Args>
		static void Debug(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = std::vformat(format, std::make_format_args(args...));
			GetCoreLogger()->debug(str);
		}

		template <class... Args>
		static void Trace(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = std::vformat(format, std::make_format_args(args...));
			GetCoreLogger()->trace(str);
		}

		template <class... Args>
		static void Info(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = std::vformat(format, std::make_format_args(args...));
			GetCoreLogger()->info(str);
		}

		template <class... Args>
		static void Warn(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = std::vformat(format, std::make_format_args(args...));
			GetCoreLogger()->warn(str);
		}

		template <class... Args>
		static void Error(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = std::vformat(format, std::make_format_args(args...));
			GetCoreLogger()->error(str);
		}

		template <class... Args>
		static void Fatal(const char* format, Args&& ... args)
		{
			ASSERT_INIT;
			const std::string str = std::vformat(format, std::make_format_args(args...));
			GetCoreLogger()->critical(str);
		}

		static VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	private:
		static bool& GetInitialized();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger();
	};

	template <u64 PrefixSize>
	class LoggerInstance
	{
	public:
		explicit LoggerInstance(CString<PrefixSize> prefix) : m_prefix(std::move(prefix)) {}

		template <class... Args>
		void Debug(const char* format, Args&& ... args) const
		{
			const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(m_prefix, format));
			Logger::Debug(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Trace(const char* format, Args&& ... args) const
		{
			const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(m_prefix, format));
			Logger::Trace(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Info(const char* format, Args&& ... args) const
		{
			const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(m_prefix, format));
			Logger::Info(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Warn(const char* format, Args&& ... args) const
		{
			const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(m_prefix, format));
			Logger::Warn(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Error(const char* format, Args&& ... args) const
		{
			const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(m_prefix, format));
			Logger::Error(formatStr.data(), std::forward<Args>(args)...);
		}

		template <class... Args>
		void Fatal(const char* format, Args&& ... args) const
		{
			const auto formatStr = std::vformat("[{}] - {}", std::make_format_args(m_prefix, format));
			Logger::Fatal(formatStr.data(), std::forward<Args>(args)...);
		}

	private:
		CString<PrefixSize> m_prefix;
	};
}
