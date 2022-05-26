
#include "logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <VkBootstrap.h>

namespace C3D
{
	bool Logger::m_initialized = false;

	std::shared_ptr<spdlog::logger> Logger::s_coreLogger;
	std::stack<std::string> Logger::m_prefixes = std::stack<std::string>();

	void Logger::Init()
	{
		spdlog::set_pattern("%^ [%T] %v%$");
		s_coreLogger = spdlog::stdout_color_mt("C3D");
		s_coreLogger->set_level(spdlog::level::trace);

		m_prefixes.push("CORE");
		m_initialized = true;
	}

	void Logger::PushPrefix(const std::string& prefix)
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized");

		if (!m_prefixes.empty() && m_prefixes.top() != prefix)
		{
			m_prefixes.push(prefix);
		}
	}

	void Logger::PopPrefix()
	{
		C3D_ASSERT_MSG(m_initialized, "Logger was used before it was initialized");

		if (m_prefixes.size() > 1) m_prefixes.pop();
	}

	VkBool32 Logger::VkDebugLog(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		const VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		PushPrefix("VK_DEBUG");

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
	}
}
