
#pragma once
#include <tuple>
#include <utility>

namespace C3D
{
	struct ICallable
	{
		virtual ~ICallable() = default;

		virtual bool Call() = 0;

	};

	template <typename TCallable, typename... Args>
	struct StaticCallable final : ICallable
	{
		TCallable func;
		std::tuple<Args...> args;

		explicit StaticCallable(TCallable callable, Args&&... args)
			: func(callable), args(std::forward<Args>(args)...)
		{}

		bool Call() override
		{
			return std::apply(func, args);
		}
	};

	template <class ClassType, typename Ret, typename... Args>
	struct MemberCallable final : ICallable
	{
		using TFunc = Ret(ClassType::*)(Args...);

		TFunc func;
		ClassType* instance;
		std::tuple<Args...> args;

		MemberCallable(const TFunc func, ClassType* instance, Args&&... args)
			: func(func), instance(instance), args(std::forward<Args>(args)...)
		{}

		bool Call() override
		{
			return std::apply(instance->*func, args);
		}
	};
}