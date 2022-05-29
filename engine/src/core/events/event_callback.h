
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

	class StaticEventCallback final : public IEventCallback
	{
	public:
		explicit StaticEventCallback(bool (*function)(u16 code, void* sender, void* listener, EventContext context))
			: function(function) {}

		virtual ~StaticEventCallback() = default;

		bool Invoke(const u16 code, void* sender, void* listener, const EventContext context) override
		{
			return function(code, sender, listener, context);
		}

		bool operator== (IEventCallback* other) override
		{
			const StaticEventCallback* otherEventCallback = dynamic_cast<StaticEventCallback*>(other);
			if (otherEventCallback == nullptr) return false;
			return otherEventCallback->function == function;
		}

	private:
		bool (*function)(u16 code, void* sender, void* listener, EventContext context);
	};

	template<typename T>
	class EventCallback final : public IEventCallback
	{
	public:
		EventCallback(T* instance, bool (T::* function)(u16 code, void* sender, void* listener, EventContext context))
			: instance(instance), function(function) {}

		virtual ~EventCallback() = default;

		bool Invoke(u16 code, void* sender, void* listener, EventContext context) override
		{
			return (instance->*function)(code, sender, listener, context);
		}

		bool operator== (IEventCallback* other) override
		{
			const EventCallback* otherEventCallback = dynamic_cast<EventCallback*>(other);
			if (otherEventCallback == nullptr) return false;

			return this->function == otherEventCallback->function &&
				this->instance == otherEventCallback->instance;
		}
	private:
		T* instance;
		bool (T::* function)(u16 code, void* sender, void* listener, EventContext context);
	};
}

