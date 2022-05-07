
#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <stack>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace C3D
{
	class Logger
	{
	public:
		static void Init();

		static void PushPrefix(const std::string& prefix);
		static void PopPrefix();

		template <class... Args>
		static void Debug(const std::string& format, Args&& ... args)
		{
			s_debugLogger->debug("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
		}

		template <class... Args>
		static void Trace(const std::string& format, Args&& ... args)
		{
			s_coreLogger->trace("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
		}

		template <class... Args>
		static void Info(const std::string& format, Args&& ... args)
		{
			s_coreLogger->info("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
		}

		template <class... Args>
		static void Warn(const std::string& format, Args&& ... args)
		{
			s_coreLogger->warn("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
		}

		template <class... Args>
		static void Error(const std::string& format, Args&& ... args)
		{
			s_coreLogger->error("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
		}

		static VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_coreLogger; }

	private:
		static std::stack<std::string> m_prefixes;

		static std::shared_ptr<spdlog::logger> s_coreLogger;
		static std::shared_ptr<spdlog::logger> s_debugLogger;
	};
}

