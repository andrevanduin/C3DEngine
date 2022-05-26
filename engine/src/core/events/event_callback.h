
#pragma once
#include "event_context.h"

namespace C3D
{
	class IEventCallback
	{
	public:
		virtual bool Invoke(u16 code, void* sender, void* listener, EventContext context) = 0;
		virtual bool operator== (IEventCallback* other) = 0;
	};

	template<typename T>
	class EventCallback : public IEventCallback
	{
	public:
		EventCallback(T* instance, bool (T::* function)(u16 code, void* sender, void* listener, EventContext context))
			: instance(instance), function(function) {}

		bool Invoke(u16 code, void* sender, void* listener, EventContext context) override
		{
			return (instance->*function)(code, sender, listener, context);
		}

		bool operator== (IEventCallback* other) override
		{
			EventCallback* otherEventCallback = dynamic_cast<EventCallback*>(other);
			if (otherEventCallback == nullptr) return false;

			return this->function == otherEventCallback->function &&
				this->instance == otherEventCallback->instance;
		}
	private:
		T* instance;
		bool (T::* function)(u16 code, void* sender, void* listener, EventContext context);
	};
}

