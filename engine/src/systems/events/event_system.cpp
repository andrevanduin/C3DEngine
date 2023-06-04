
#include "event_system.h"

#include <algorithm>

#include "core/logger.h"

namespace C3D
{
	EventSystem::EventSystem(const Engine* engine) : BaseSystem(engine, "EVENT"), m_registered{} {}

	void EventSystem::Shutdown()
	{
		m_logger.Info("Shutting Down");
		for (auto& [events] : m_registered)
		{
			if (!events.Empty()) events.Clear();
		}
	}

	bool EventSystem::Register(u16 code, const StaticEventFunc function)
	{
		const auto onEvent = Memory.New<StaticEventCallback>(MemoryType::EventSystem, function);

		auto& events = m_registered[code].events;
		for (const auto& event : events)
		{
			if (event->Equals(onEvent))
			{
				m_logger.Warn("This listener has already been Registered for {}", code);
				Memory.Delete(MemoryType::EventSystem, onEvent);
				return false;
			}
		}

		events.PushBack(onEvent);
		return true;
	}

	bool EventSystem::UnRegister(const u16 code, const StaticEventFunc function)
	{
		auto& events = m_registered[code].events;
		if (events.Empty())
		{
			m_logger.Warn("Tried to UnRegister Event for a code that has no events");
			return false;
		}

		const auto searchCallback = StaticEventCallback(function);
		const auto it = std::ranges::find_if(events, [&](IEventCallback* e) { return e->Equals(&searchCallback); });
		if (it != events.end())
		{
			events.Erase(it);
			Memory.Delete(MemoryType::EventSystem, *it);
			return true;
		}

		m_logger.Warn("Tried to UnRegister Event that did not exist");
		return false;
	}

	bool EventSystem::Fire(const u16 code, void* sender, const EventContext data) const
	{
		const auto& events = m_registered[code].events;
		if (events.Empty())
		{
			return false;
		}
		return std::ranges::any_of(events, [&](IEventCallback* e){ return e->Invoke(code, sender, data); });
	}
}
