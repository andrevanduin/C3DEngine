
#pragma once
#include <vector>

#include "event_callback.h"
#include "event_context.h"
#include "containers/dynamic_array.h"

#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
	constexpr auto MAX_MESSAGE_CODES = 4096;

	class EventSystem
	{
		struct EventCodeEntry
		{
			DynamicArray<IEventCallback*> events;
		};

	public:
		EventSystem();

		[[nodiscard]] bool Init() const;
		void Shutdown();

		C3D_API bool Register(u16 code, IEventCallback* onEvent);
		C3D_API bool UnRegister(u16 code, IEventCallback* onEvent);

		C3D_API bool Fire(u16 code, void* sender, EventContext data) const;

	private:
		LoggerInstance<16> m_logger;

		EventCodeEntry m_registered[MAX_MESSAGE_CODES];
	};
}
