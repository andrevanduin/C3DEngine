
#include "event.h"

#include <algorithm>

#include "core/logger.h"
#include "core/memory.h"

namespace C3D
{
	bool EventSystem::m_initialized = false;
	EventSystem::EventSystemState EventSystem::m_state;

	bool EventSystem::Init()
	{
		Logger::PrefixInfo("EVENT", "Init()");

		if (m_initialized) return false;

		Memory::Zero(&m_state, sizeof m_state);

		m_initialized = true;
		return true;
	}

	void EventSystem::Shutdown()
	{
		Logger::PrefixInfo("EVENT", "Shutting Down");
		for (auto& [events] : m_state.registered)
		{
			if (!events.empty()) events.clear();
		}
		m_initialized = false;
	}

	bool EventSystem::Register(u16 code, void* listener, IEventCallback* onEvent)
	{
		if (!m_initialized) return false;

		for (const auto& event : m_state.registered[code].events)
		{
			if (event.listener == listener)
			{
				Logger::Warn("This listener has already been Registered for {}", code);
				return false;
			}
		}

		const RegisteredEvent event{ listener, onEvent };
		m_state.registered[code].events.push_back(event);

		return true;
	}

	bool EventSystem::UnRegister(const u16 code, const void* listener, const IEventCallback* onEvent)
	{
		if (!m_initialized) return false;

		if (m_state.registered[code].events.empty())
		{
			Logger::Warn("Tried to UnRegister Event for a code that has no events");
			return false;
		}

		const auto it = std::find_if(
			m_state.registered[code].events.begin(), 
			m_state.registered[code].events.end(), 
			[&](const RegisteredEvent& e)
			{
				return e.listener == listener && e.callback == onEvent;
			}
		);

		if (it != m_state.registered[code].events.end())
		{
			m_state.registered[code].events.erase(it);
			return true;
		}

		Logger::Warn("Tried to UnRegister Event that did not exist");
		return false;
	}

	bool EventSystem::Fire(const u16 code, void* sender, const EventContext data)
	{
		if (!m_initialized) return false;

		if (m_state.registered[code].events.empty())
		{
			return false;
		}

		return std::any_of(m_state.registered[code].events.begin(), m_state.registered[code].events.end(), 
		[&](const RegisteredEvent& e)
		{
			return e.callback->Invoke(code, sender, e.listener, data);
		});
	}
}
