
#pragma once
#include "core/logger.h"

namespace C3D
{
	template <u64 NameSize, typename T>
	class System
	{
	public:
		explicit System(const CString<NameSize>& name) : m_logger(name), m_config{} {}
		System(const System&) = delete;
		System(System&&) = delete;

		System& operator=(const System&) = delete;
		System& operator=(System&&) = delete;

		virtual ~System() = default;

		virtual bool Init(const T& config) { return false; }
		virtual void Shutdown() {}

	protected:
		LoggerInstance<NameSize> m_logger;
		T m_config;
	};
}
