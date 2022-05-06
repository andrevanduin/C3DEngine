
#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

namespace C3D
{
	class Logger
	{
	public:
		static void Init();

		static void SetPrefix(const std::string& prefix);

		template <class... Args>
		static void Debug(const std::string& format, Args&& ... args)
		{
			s_debugLogger->debug("[" + s_corePrefix + "] - " + format, std::forward<Args>(args)...);
		}

		template <class... Args>
		static void Info(const std::string& format, Args&& ... args)
		{
			s_coreLogger->info("[" + s_corePrefix + "] - " + format, std::forward<Args>(args)...);
		}

		template <class... Args>
		static void Warn(const std::string& format, Args&& ... args)
		{
			s_coreLogger->warn("[" + s_corePrefix + "] - " + format, std::forward<Args>(args)...);
		}

		template <class... Args>
		static void Error(const std::string& format, Args&& ... args)
		{
			s_coreLogger->error("[" + s_corePrefix + "] - " + format, std::forward<Args>(args)...);
		}

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_coreLogger; }

	private:
		static std::string s_corePrefix;

		static std::shared_ptr<spdlog::logger> s_coreLogger;
		static std::shared_ptr<spdlog::logger> s_debugLogger;
	};
}

