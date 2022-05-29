
#pragma once
#include "event_context.h"

namespace C3D
{
	typedef bool (*pStaticFunc)(u16 code, void* sender, EventContext context);

	class IEventCallback
	{
	public:
		IEventCallback() = default;
		virtual ~IEventCallback() = default;

		virtual bool Invoke(u16 code, void* sender, EventContext context) = 0;

		virtual bool Equals(IEventCallback* other) = 0;
		virtual bool Equals(const IEventCallback* other) = 0;
	};

	class StaticEventCallback final : public IEventCallback
	{
	public:
		explicit StaticEventCallback(const pStaticFunc func) : function(func) {}

		~StaticEventCallback() override = default;

		bool Invoke(const u16 code, void* sender, const EventContext context) override
		{
			return function(code, sender, context);
		}

		bool Equals(IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<StaticEventCallback*>(other);
			if (otherEventCallback == nullptr) return false;
			return otherEventCallback->function == function;
		}

		bool Equals(const IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<const StaticEventCallback*>(other);
			if (otherEventCallback == nullptr) return false;
			return otherEventCallback->function == function;
		}

		pStaticFunc function;
	};

	template<typename T>
	class EventCallback final : public IEventCallback
	{
	public:
		EventCallback(T* instance, bool (T::* function)(u16 code, void* sender, EventContext context)) : instance(instance), function(function) {}

		~EventCallback() override = default;

		bool Invoke(u16 code, void* sender, EventContext context) override
		{
			return (instance->*function)(code, sender, context);
		}

		bool Equals(IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<EventCallback*>(other);
			if (otherEventCallback == nullptr) return false;

			return otherEventCallback->instance == instance && otherEventCallback->function == function;
		}

		bool Equals(const IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<const EventCallback*>(other);
			if (otherEventCallback == nullptr) return false;

			return otherEventCallback->instance == instance && otherEventCallback->function == function;
		}

		T* instance;
		bool (T::* function)(u16 code, void* sender, EventContext context);
	};
}

