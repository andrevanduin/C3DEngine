
#include "logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <VkBootstrap.h>

namespace C3D
{
	void Logger::Init()
	{
		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		consoleSink->set_pattern("%^ [%T] %v%$");
		consoleSink->set_level(spdlog::level::debug);

		auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("console.log", true);
		consoleSink->set_pattern("%^ [%T] %v%$");
		consoleSink->set_level(spdlog::level::trace);

		GetCoreLogger() = std::make_shared<spdlog::logger>(spdlog::logger("core", { consoleSink, fileSink }));
		GetCoreLogger()->set_level(spdlog::level::trace);

		GetInitialized() = true;
	}

	VkBool32 Logger::VkDebugLog(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		const VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
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
			Error("[{}] {}", type, pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

	bool& Logger::GetInitialized()
	{
		static bool initialized = false;
		return initialized;
	}

	std::shared_ptr<spdlog::logger>& Logger::GetCoreLogger()
	{
		static std::shared_ptr<spdlog::logger> coreLogger;
		return coreLogger;
	}
}
