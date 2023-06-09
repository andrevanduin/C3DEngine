
#pragma once
#include "core/defines.h"
#include "core/logger.h"
#include "core/events/event_context.h"

#include "systems/system.h"
#include "containers/dynamic_array.h"
#include "core/function/function.h"

namespace C3D
{
	constexpr auto MAX_MESSAGE_CODES = 4096;

	using EventCallbackFunc = StackFunction<bool(u16, void*, const EventContext&), 16>;
	using EventCallbackId = u16;
#define INVALID_CALLBACK std::numeric_limits<u16>::max()

	class EventSystem final : public BaseSystem
	{
		struct EventCallback
		{
			EventCallback(const EventCallbackId id, EventCallbackFunc func)
				: id(id), func(std::move(func))
			{}

			EventCallbackId id;
			EventCallbackFunc func;
		};

		struct EventCodeEntry
		{
			DynamicArray<EventCallback> events;
		};

	public:
		explicit EventSystem(const Engine* engine);

		void Shutdown() override;

		C3D_API EventCallbackId Register(u16 code, const EventCallbackFunc& callback);
		C3D_API bool Unregister(u16 code, EventCallbackId& callbackId);
		C3D_API bool UnregisterAll(u16 code);

		C3D_API bool Fire(u16 code, void* sender, EventContext data) const;

	private:
		EventCodeEntry m_registered[MAX_MESSAGE_CODES];
	};
}
