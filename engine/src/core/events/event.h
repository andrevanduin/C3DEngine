
#pragma once
#include <vector>

#include "event_callback.h"
#include "event_context.h"

#include "core/defines.h"
#include "core/logger.h"

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
		EventSystem();

		[[nodiscard]] bool Init() const;
		void Shutdown();

		C3D_API bool Register(u16 code, IEventCallback* onEvent);
		C3D_API bool UnRegister(u16 code, IEventCallback* onEvent);

		C3D_API bool Fire(u16 code, void* sender, EventContext data);

	private:
		LoggerInstance m_logger;

		EventCodeEntry m_registered[MAX_MESSAGE_CODES];
	};
}
