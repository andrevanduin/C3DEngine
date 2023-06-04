
#pragma once
#include "core/defines.h"
#include "core/logger.h"
#include "core/events/event_callback.h"
#include "core/events/event_context.h"

#include "systems/system.h"
#include "containers/dynamic_array.h"

namespace C3D
{
	constexpr auto MAX_MESSAGE_CODES = 4096;

	class EventSystem final : public BaseSystem
	{
		struct EventCodeEntry
		{
			DynamicArray<IEventCallback*> events;
		};

	public:
		explicit EventSystem(const Engine* engine);

		void Shutdown() override;

		template <class T>
		C3D_API bool Register(u16 code, T* classPtr, const EventFunc<T> function)
		{
			const auto onEvent = Memory.New<EventCallback<T>>(MemoryType::EventSystem, classPtr, function);

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

		template <class T>
		C3D_API bool UnRegister(const u16 code, T* classPtr, const EventFunc<T> function)
		{
			auto& events = m_registered[code].events;
			if (events.Empty())
			{
				m_logger.Warn("Tried to UnRegister Event for a code that has no events");
				return false;
			}

			const auto searchCallback = EventCallback<T>(classPtr, function);
			const auto it = std::ranges::find_if(events, [&](IEventCallback* e) { return e->Equals(&searchCallback); });
			if (it != events.end())
			{
				Memory.Delete(MemoryType::EventSystem, *it);

				events.Erase(it);
				return true;
			}

			m_logger.Warn("Tried to UnRegister Event that did not exist");
			return false;
		}

		C3D_API bool Register(u16 code, StaticEventFunc function);
		C3D_API bool UnRegister(u16 code, StaticEventFunc function);


		C3D_API bool Fire(u16 code, void* sender, EventContext data) const;

	private:
		EventCodeEntry m_registered[MAX_MESSAGE_CODES];
	};
}
