
#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace C3D
{
	std::string Logger::s_corePrefix = "CORE";

	std::shared_ptr<spdlog::logger> Logger::s_coreLogger;
	std::shared_ptr<spdlog::logger> Logger::s_debugLogger;

	void Logger::Init()
	{
		spdlog::set_pattern("%^ [%T] %v%$");
		s_coreLogger = spdlog::stdout_color_mt("C3D");
		s_coreLogger->set_level(spdlog::level::trace);

		s_debugLogger = spdlog::stdout_color_mt("C3DDebug");
		s_debugLogger->set_level(spdlog::level::trace);
		s_debugLogger->set_pattern("%^ [source %s] [function %!] [line %#] [%T] %v%$");
	}

	void Logger::SetPrefix(const std::string& prefix)
	{
		s_corePrefix = prefix;
	}
}