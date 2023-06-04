
#pragma once
#include "core/logger.h"

namespace C3D
{
	class Engine;

	class ISystem
	{
	public:
		explicit ISystem(const Engine* engine) : m_initialized(false), m_engine(engine) {}

		ISystem(const ISystem&) = delete;
		ISystem(ISystem&&) = delete;

		ISystem& operator=(const ISystem&) = delete;
		ISystem& operator=(ISystem&&) = delete;

		virtual ~ISystem() = default;

		virtual void Shutdown() = 0;
		virtual void Update(f64 deltaTime) = 0;

	protected:
		bool m_initialized;

		const Engine* m_engine;
	};

	class BaseSystem : public ISystem
	{
	public:
		explicit BaseSystem(const Engine* engine, const CString<32>& name) : ISystem(engine), m_logger(name) {}
		
		virtual bool Init()
		{
			m_logger.Info("Init()");
			m_initialized = true;
			return true;
		}

		void Shutdown() override
		{
			m_logger.Info("Shutdown()");
			m_initialized = false;
		}

		void Update(const f64 deltaTime) override {}

	protected:
		LoggerInstance<32> m_logger;
	};

	template <typename T>
	class SystemWithConfig : public ISystem
	{
	public:
		explicit SystemWithConfig(const Engine* engine, const CString<32>& name) : ISystem(engine), m_logger(name), m_config{} {}

		virtual bool Init(const T& config = {})
		{
			m_logger.Info("Init()");
			m_initialized = true;
			m_config = config;
			return true;
		}

		void Shutdown() override
		{
			m_logger.Info("Shutdown()");
			m_initialized = false;
		}

		void Update(const f64 deltaTime) override {}

	protected:
		LoggerInstance<32> m_logger;
		T m_config;
	};
}
