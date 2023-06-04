
#pragma once
#include "event_context.h"

namespace C3D
{
	using StaticEventFunc = bool (*)(u16 code, void* sender, const EventContext& context);
	template <class T>
	using EventFunc = bool (T::*)(u16 code, void* sender, const EventContext& context);

	class IEventCallback
	{
	public:
		IEventCallback() = default;

		IEventCallback(const IEventCallback&) = delete;
		IEventCallback(IEventCallback&&) = delete;

		IEventCallback& operator=(const IEventCallback&) = delete;
		IEventCallback& operator=(IEventCallback&&) = delete;

		virtual ~IEventCallback() = default;

		virtual bool Invoke(u16 code, void* sender, const EventContext& context) = 0;

		virtual bool Equals(IEventCallback* other) = 0;
		virtual bool Equals(const IEventCallback* other) = 0;
	};

	class StaticEventCallback final : public IEventCallback
	{
	public:
		explicit StaticEventCallback(const StaticEventFunc func)
			: m_function(func)
		{}

		bool Invoke(const u16 code, void* sender, const EventContext& context) override
		{
			return m_function(code, sender, context);
		}

		bool Equals(IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<StaticEventCallback*>(other);
			if (otherEventCallback == nullptr) return false;
			return otherEventCallback->m_function == m_function;
		}

		bool Equals(const IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<const StaticEventCallback*>(other);
			if (otherEventCallback == nullptr) return false;
			return otherEventCallback->m_function == m_function;
		}

	private:
		StaticEventFunc m_function;
	};

	template<typename T>
	class EventCallback final : public IEventCallback
	{
	public:
		EventCallback(T* instance, const EventFunc<T> function)
			: m_instance(instance), m_function(function)
		{}

		bool Invoke(u16 code, void* sender, const EventContext& context) override
		{
			return (m_instance->*m_function)(code, sender, context);
		}

		bool Equals(IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<EventCallback*>(other);
			if (otherEventCallback == nullptr) return false;

			return otherEventCallback->m_instance == m_instance && otherEventCallback->m_function == m_function;
		}

		bool Equals(const IEventCallback* other) override
		{
			const auto* otherEventCallback = dynamic_cast<const EventCallback*>(other);
			if (otherEventCallback == nullptr) return false;

			return otherEventCallback->m_instance == m_instance && otherEventCallback->m_function == m_function;
		}

	private:

		T* m_instance;
		EventFunc<T> m_function;
	};
}

