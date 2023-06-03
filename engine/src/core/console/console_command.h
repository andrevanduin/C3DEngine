
#pragma once
#include "containers/cstring.h"
#include "containers/string.h"

namespace C3D
{
	class ICommand
	{
	public:
		explicit ICommand(const CString<128> name) : name(name) {}

		ICommand(const ICommand&) = delete;
		ICommand(ICommand&&) = delete;

		ICommand& operator=(const ICommand&) = delete;
		ICommand& operator=(ICommand&&) = delete;

		virtual ~ICommand() = default;

		CString<128> name;

		virtual bool Invoke(String* args) = 0;
	};

	typedef bool (*pStaticCommandFunc)(String* args);

	class StaticCommand final : public ICommand
	{
	public:
		StaticCommand(const CString<128> commandName, const pStaticCommandFunc function)
			: ICommand(commandName), m_function(function)
		{}

		bool Invoke(String* args) override
		{
			return m_function(args);
		}

	private:
		pStaticCommandFunc m_function;
	};

	template <class T>
	class InstanceCommand final : public ICommand
	{
	public:
		InstanceCommand(const CString<128> commandName, T* instance, bool (T::* function)(String* args))
			: ICommand(commandName), m_instance(instance), m_function(function)
		{}

		bool Invoke(String* args) override
		{
			return (m_instance->*m_function)(args);
		}

	private:
		T* m_instance;
		bool (T::* m_function)(String* args);
	};
}