
#pragma once
#include <vector>

#include "event_callback.h"
#include "event_context.h"

#include "core/defines.h"

namespace C3D
{
	constexpr auto MAX_MESSAGE_CODES = 16384;

	class C3D_API EventSystem
	{
		struct RegisteredEvent
		{
			void*			listener;
			IEventCallback* callback;
		};

		struct EventCodeEntry
		{
			std::vector<RegisteredEvent> events;
		};

		struct EventSystemState
		{
			EventCodeEntry registered[MAX_MESSAGE_CODES];
		};

	public:
		static bool Init();
		static void Shutdown();

		static bool Register(u16 code, void* listener, IEventCallback* onEvent);
		static bool UnRegister(u16 code, const void* listener, const IEventCallback* onEvent);

		static bool Fire(u16 code, void* sender, EventContext data);

	private:
		static bool m_initialized;
		static EventSystemState m_state;
	};
}
