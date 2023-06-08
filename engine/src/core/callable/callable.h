
#pragma once

namespace C3D
{
	template <class ... Args>
	class ICallable
	{
	public:
		ICallable() = default;

		ICallable(const ICallable&) = delete;
		ICallable(ICallable&&) = delete;

		ICallable& operator=(const ICallable&) = delete;
		ICallable& operator=(ICallable&&) = delete;

		virtual ~ICallable() = default;

		virtual bool Invoke(Args&& ... args) const = 0;
	};

	template <typename FuncPtr, class... Args>
	class StaticCallable final : public ICallable<Args...>
	{
	public:
		explicit StaticCallable(FuncPtr func) : m_func(func) {}

		bool Invoke(Args&& ... args) const override
		{
			return m_func(std::forward<Args>(args)...);
		}
	private:
		FuncPtr m_func;
	};

	template <typename Instance, typename FuncPtr, class... Args>
	class InstanceCallable final : public ICallable<Args...>
	{
	public:
		InstanceCallable(Instance* instance, FuncPtr func)
			: m_instance(instance), m_func(func)
		{}

		bool Invoke(Args&&... args) const override
		{
			return (m_instance->*m_func)(std::forward<Args>(args)...);
		}

	private:
		Instance* m_instance;
		FuncPtr m_func;
	};

#define REGISTER_CALLABLE(name, ...)																	\
	using name##Callable = ICallable<__VA_ARGS__>;														\
	using name##StaticCallable = StaticCallable<bool(*)(__VA_ARGS__), __VA_ARGS__>;						\
	template <typename T>																				\
	using name##InstanceCallable = InstanceCallable<T, bool(T::*)(__VA_ARGS__), __VA_ARGS__>;			\
	template <typename T>																				\
	using name##CInstanceCallable = InstanceCallable<T, bool(T::*)(__VA_ARGS__) const, __VA_ARGS__>;	\
																										\
	static name##Callable* Make##name##Callable(bool(*func)(__VA_ARGS__))								\
	{																									\
		return Memory.New<name##StaticCallable>(MemoryType::Callable, func);							\
	}																									\
																										\
	template <typename T>																				\
	static name##Callable* Make##name##Callable(T* instance, bool(T::*func)(__VA_ARGS__))				\
	{																									\
		return Memory.New<name##InstanceCallable<T>>(MemoryType::Callable, instance, func);				\
	}																									\
																										\
	template <typename T>																				\
	static name##Callable* Make##name##Callable(T* instance, bool(T::* func)(__VA_ARGS__) const)		\
	{																									\
		return Memory.New<name##CInstanceCallable<T>>(MemoryType::Callable, instance, func);			\
	}
																
}