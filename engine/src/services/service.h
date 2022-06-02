
#pragma once
#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
	class Service
	{
	public:
		explicit Service(const std::string& name);
	protected:
		LoggerInstance m_logger;
	};
}
