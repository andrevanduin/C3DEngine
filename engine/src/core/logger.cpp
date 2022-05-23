
#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace C3D
{
	std::shared_ptr<spdlog::logger> Logger::s_coreLogger;
	std::shared_ptr<spdlog::logger> Logger::s_debugLogger;
	std::stack<std::string> Logger::m_prefixes = std::stack<std::string>();

	void Logger::Init()
	{
		spdlog::set_pattern("%^ [%T] %v%$");
		s_coreLogger = spdlog::stdout_color_mt("C3D");
		s_coreLogger->set_level(spdlog::level::trace);

		s_debugLogger = spdlog::stdout_color_mt("C3DDebug");
		s_debugLogger->set_level(spdlog::level::trace);
		s_debugLogger->set_pattern("%^ [source %s] [function %!] [line %#] [%T] %v%$");

		m_prefixes.push("CORE");
	}

	void Logger::PushPrefix(const std::string& prefix)
	{
		if (!m_prefixes.empty() && m_prefixes.top() != prefix)
		{
			m_prefixes.push(prefix);
		}
	}

	void Logger::PopPrefix()
	{
		if (m_prefixes.size() > 1) m_prefixes.pop();
	}

	/*
	VkBool32 Logger::VkDebugLog(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		const VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		PushPrefix("VKDEBUG");

		auto type = vkb::to_string_message_type(messageType);
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			Trace("[{}] {}", type, pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			Info("[{}] {}", type, pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			Warn("[{}] {}", type, pCallbackData->pMessage);
		}
		else
		{
			Error("[VkDebug: {}] {}", type, pCallbackData->pMessage);
		}

		PopPrefix();
		return VK_FALSE;
	}*/
}
