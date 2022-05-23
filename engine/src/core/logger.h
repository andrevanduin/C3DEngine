
#pragma once
#include <stack>

#include <spdlog/spdlog.h>

#include "defines.h"

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
		static void Trace(const string& format, Args&& ... args);

		template <class... Args>
		static void Info(const string& format, Args&& ... args);

		template <class... Args>
		static void Warn(const string& format, Args&& ... args);

		template <class... Args>
		static void Error(const string& format, Args&& ... args);

		/*
		static VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);*/

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_coreLogger; }
	private:
		static std::stack<string> m_prefixes;

		static std::shared_ptr<spdlog::logger> s_coreLogger;
		static std::shared_ptr<spdlog::logger> s_debugLogger;
	};

	template <class... Args>
	void Logger::Error(const string& format, Args&&... args)
	{
		s_coreLogger->error("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Debug(const string& format, Args&&... args)
	{
		s_debugLogger->debug("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Trace(const string& format, Args&&... args)
	{
		s_coreLogger->trace("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Info(const string& format, Args&&... args)
	{
		s_coreLogger->info("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}

	template <class... Args>
	void Logger::Warn(const string& format, Args&&... args)
	{
		s_coreLogger->warn("[" + m_prefixes.top() + "] - " + format, std::forward<Args>(args)...);
	}
}
