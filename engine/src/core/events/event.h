
#pragma once
#include <vector>

#include "event_callback.h"
#include "event_context.h"

#include "core/defines.h"

namespace C3D
{
	constexpr auto MAX_MESSAGE_CODES = 16384;

	class EventSystem
	{
		struct EventCodeEntry
		{
			std::vector<IEventCallback*> events;
		};

	public:
		bool Init();
		void Shutdown();

		C3D_API bool Register(u16 code, IEventCallback* onEvent);
		C3D_API bool UnRegister(u16 code, const IEventCallback* onEvent);

		C3D_API bool Fire(u16 code, void* sender, EventContext data);

	private:
		EventCodeEntry m_registered[MAX_MESSAGE_CODES];
	};
}
