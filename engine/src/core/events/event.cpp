
#include "event.h"

#include <algorithm>

#include "core/logger.h"

namespace C3D
{
	EventSystem::EventSystem() : m_logger("EVENT"), m_registered{} {}

	bool EventSystem::Init() const
	{
		m_logger.Info("Init()");
		return true;
	}

	void EventSystem::Shutdown()
	{
		m_logger.Info("Shutting Down");
		for (auto& [events] : m_registered)
		{
			if (!events.empty()) events.clear();
		}
	}

	bool EventSystem::Register(u16 code, IEventCallback* onEvent)
	{
		auto& events = m_registered[code].events;
		for (const auto& event : events)
		{
			if (event->Equals(onEvent))
			{
				m_logger.Warn("This listener has already been Registered for {}", code);
				return false;
			}
		}

		events.push_back(onEvent);
		return true;
	}

	bool EventSystem::UnRegister(const u16 code, const IEventCallback* onEvent)
	{
		auto& events = m_registered[code].events;
		if (events.empty())
		{
			m_logger.Warn("Tried to UnRegister Event for a code that has no events");
			return false;
		}

		const auto it = std::find_if(events.begin(), events.end(), [&](IEventCallback* e) { return e->Equals(onEvent); });
		if (it != events.end())
		{
			events.erase(it);
			return true;
		}

		m_logger.Warn("Tried to UnRegister Event that did not exist");
		return false;
	}

	bool EventSystem::Fire(const u16 code, void* sender, const EventContext data)
	{
		auto& events = m_registered[code].events;
		if (events.empty())
		{
			return false;
		}
		return std::any_of(events.begin(), events.end(), [&](IEventCallback* e){ return e->Invoke(code, sender, data); });
	}
}
