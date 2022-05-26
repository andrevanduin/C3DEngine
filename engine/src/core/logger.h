
#pragma once
#include <stack>

#pragma warning(push, 0)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include <vulkan/vulkan_core.h>

#include "application.h"
#include "defines.h"
#include "asserts.h"

namespace C3D
{
	class C3D_API Logger
	{
	public:
		static void Init();

		static void PushPrefix(const string& prefix);
		static void PopPrefix();

		template <class... Args>
		static void Debug(const string& format, Args&& ... args);

		template <class... Args>
		static void PrefixTrace(const string& prefix, const string& format, Args&& ... args);

		template <class... Args>
		static void Trace(const string& format, Args&& ... args);

		template <class... Args>
		static void PrefixInfo(const string& prefix, const string& format, Args&& ... args);

		template <class... Args>
		static void Info(const string& format, Args&& ... args);

		template <class... Args>
		static void PrefixWarn(const string& prefix, const string& format, Args&& ... args);

		template <class... Args>
		static void Warn(const string& format, Args&& ... args);

		template <class... Args>
		static void PrefixError(const string& prefix, const string& format, Args&& ... args);

		template <class... Args>
		static void Error(const string& format, Args&& ... args);

		template <class... Args>
		static void Fatal(const string& format, Args&& ... args);

		static VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_coreLogger; }
	private:
		static bool m_initialized;

		static std::stack<string> m_prefixes;

		static std::shared_ptr<spdlog::logger> s_coreLogger;
	};

	template <class... Args>
	void Logger::Error(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->error("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class... Args>
	void Logger::PrefixError(const string& prefix, const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->error("[" + prefix + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Fatal(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->critical("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
		abort();
	}

	template <class ... Args>
	void Logger::Debug(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->debug("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::PrefixTrace(const string& prefix, const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->trace("[" + prefix + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Trace(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->trace("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Info(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->info("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::PrefixInfo(const string& prefix, const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->info("[" + prefix + "] - " + format, std::forward<Args>(args)...);
	}

	template <class... Args>
	void Logger::Warn(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->warn("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class... Args>
	void Logger::PrefixWarn(const string& prefix, const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->warn("[" + prefix + "] - " + format, std::forward<Args>(args)...);
	}
}
