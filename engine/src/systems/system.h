
#pragma once
#include "core/logger.h"

namespace C3D
{
	template<typename T>
	class System
	{
	public:
		explicit System(const std::string& name) : m_logger(name), m_config{} {}
		virtual ~System() = default;

		virtual bool Init(const T& config) = 0;
		virtual void Shutdown() = 0;

	protected:
		LoggerInstance m_logger;
		T m_config;
	};
}
