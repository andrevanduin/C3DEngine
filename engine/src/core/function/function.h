
#pragma once
#include "core/defines.h"

namespace C3D
{
	template <typename... Args>
	class Function;

	template <typename Result, typename... Args>
	class Function<Result (Args...)>
	{
	public:
		Function() = default;

		Function(const Function&) = delete;
		Function& operator= (const Function&) = delete;

		Function(Function&&) = delete;
		Function& operator= (Function&&) = delete;

		virtual ~Function() = default;
		virtual Result operator() (Args&&...) const = 0;
	};

	template <typename, u64>
	class StackFunction;

	template<u64 stackSize, typename Result, typename... Args>
	class StackFunction<Result (Args...), stackSize> final : public Function<Result (Args...)>
	{
	public:
		StackFunction()
			: Function<Result (Args...)>(), m_functorHolderPtr(nullptr), m_stack{0}
		{}

		template <typename Functor>
		StackFunction(Functor func)
			: Function<Result(Args...)>(), m_stack{0}
		{
			static_assert(sizeof FunctorHolder<Functor, Result, Args...> <= sizeof m_stack, "Functor is too large to fit on defined stack (increase stackSize)");
			m_functorHolderPtr = reinterpret_cast<IFunctorHolder<Result, Args...>*>(std::addressof(m_stack));
			new (m_functorHolderPtr) FunctorHolder<Functor, Result, Args...>(func);
		}

		StackFunction(const StackFunction& other)
			: m_stack{0}
		{
			if (other.m_functorHolderPtr != nullptr)
			{
				m_functorHolderPtr = reinterpret_cast<IFunctorHolder<Result, Args...>*>(std::addressof(m_stack));
				other.m_functorHolderPtr->CopyInto(m_functorHolderPtr);
			}
		}

		StackFunction(StackFunction&&) noexcept = delete;
		StackFunction& operator= (StackFunction&&) = delete;

		StackFunction& operator=(const StackFunction& other)
		{
			if (this == &other) return *this;

			if (m_functorHolderPtr != nullptr)
			{
				m_functorHolderPtr->~IFunctorHolder<Result, Args...>();
				m_functorHolderPtr = nullptr;
			}

			if (other.m_functorHolderPtr != nullptr)
			{
				m_functorHolderPtr = reinterpret_cast<IFunctorHolder<Result, Args...>*>(std::addressof(m_stack));
				other.m_functorHolderPtr->CopyInto(m_functorHolderPtr);
			}

			return *this;
		}

		~StackFunction() override
		{
			if (m_functorHolderPtr != nullptr)
			{
				m_functorHolderPtr->~IFunctorHolder<Result, Args...>();
			}
		}

		Result operator() (Args&&... args) const override
		{
			return (*m_functorHolderPtr) (std::forward<Args>(args)...);
		}

		explicit operator bool() const noexcept
		{
			return m_functorHolderPtr == nullptr;
		}

	//private:
		template <typename ReturnType, typename... Arguments>
		struct IFunctorHolder
		{
			IFunctorHolder() = default;

			IFunctorHolder(const IFunctorHolder&) = delete;
			IFunctorHolder(IFunctorHolder&&) = delete;

			IFunctorHolder& operator=(const IFunctorHolder&) = delete;
			IFunctorHolder& operator=(IFunctorHolder&&) = delete;

			virtual ~IFunctorHolder() = default;
			virtual ReturnType operator() (Arguments&&...) = 0;
			virtual void CopyInto(void*) const = 0;
		};

		template <typename Functor, typename ReturnType, typename... Arguments>
		struct FunctorHolder final : IFunctorHolder<Result, Arguments...>
		{
			explicit FunctorHolder(Functor func)
				: IFunctorHolder<Result, Arguments...>(), functor(func)
			{}

			ReturnType operator() (Arguments&&... args) override
			{
				return functor(std::forward<Arguments>(args)...);
			}

			void CopyInto(void* destination) const override
			{
				new(destination) FunctorHolder(functor);
;			}

			Functor functor;
		};

		IFunctorHolder<Result, Args...>* m_functorHolderPtr;
		byte m_stack[stackSize];
	};

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