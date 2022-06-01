
#pragma once
#include <stack>

#pragma warning(push, 0)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include <vulkan/vulkan_core.h>

#include "defines.h"
#include "asserts.h"

namespace C3D
{
	class C3D_API Logger
	{
	public:
		static void Init();

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

		template <class... Args>
		static void Fatal(const string& format, Args&& ... args);

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
		void Debug(const string& format, Args&& ... args) const;

		template <class... Args>
		void Trace(const string& format, Args&& ... args) const;

		template <class... Args>
		void Info(const string& format, Args&& ... args) const;

		template <class... Args>
		void Warn(const string& format, Args&& ... args) const;

		template <class... Args>
		void Error(const string& format, Args&& ... args) const;

		template <class... Args>
		void Fatal(const string& format, Args&& ... args) const;

	private:
		std::string m_prefix;
	};

	template <class ... Args>
	void Logger::Debug(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->debug(format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void LoggerInstance::Debug(const string& format, Args&&... args) const
	{
		return Logger::Debug("[{}] - " + format, m_prefix, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Trace(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->trace(format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void LoggerInstance::Trace(const string& format, Args&&... args) const
	{
		return Logger::Trace("[{}] - " + format, m_prefix, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void Logger::Info(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->info(format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void LoggerInstance::Info(const string& format, Args&&... args) const
	{
		return Logger::Info("[{}] - " + format, m_prefix, std::forward<Args>(args)...);
	}

	template <class... Args>
	void Logger::Warn(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->warn(format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void LoggerInstance::Warn(const string& format, Args&&... args) const
	{
		return Logger::Warn("[{}] - " + format, m_prefix, std::forward<Args>(args)...);
	}

	template <class... Args>
	void Logger::Error(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");
		s_coreLogger->error(format, std::forward<Args>(args)...);
	}

	template <class ... Args>
	void LoggerInstance::Error(const string& format, Args&&... args) const
	{
		return Logger::Error("[{}] - " + format, m_prefix, std::forward<Args>(args)...);
	}


	template <class ... Args>
	void Logger::Fatal(const string& format, Args&&... args)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized!");

		s_coreLogger->critical(format, std::forward<Args>(args)...);
		abort();
	}

	template <class ... Args>
	void LoggerInstance::Fatal(const string& format, Args&&... args) const
	{
		return Logger::Fatal("[{}] - " + format, m_prefix, std::forward<Args>(args)...);
	}
}
