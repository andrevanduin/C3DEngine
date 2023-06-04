
#pragma once
#include "containers/cstring.h"
#include "containers/string.h"

namespace C3D
{
	using pStaticCommandFunc = bool (*)(const DynamicArray<CString<128>>& args, CString<256>& output);
	template <class T>
	using pCommandFunc = bool(T::*)(const DynamicArray<CString<128>>& args, CString<256>& output);

	class ICommand
	{
	public:
		explicit ICommand(const CString<128>& name) : name(name) {}

		ICommand(const ICommand&) = delete;
		ICommand(ICommand&&) = delete;

		ICommand& operator=(const ICommand&) = delete;
		ICommand& operator=(ICommand&&) = delete;

		virtual ~ICommand() = default;

		CString<128> name;

		virtual bool Invoke(const DynamicArray<CString<128>>& args, CString<256>& output) = 0;
	};

	class StaticCommand final : public ICommand
	{
	public:
		StaticCommand(const CString<128>& commandName, const pStaticCommandFunc function)
			: ICommand(commandName), m_function(function)
		{}

		bool Invoke(const DynamicArray<CString<128>>& args, CString<256>& output) override
		{
			return m_function(args, output);
		}

	private:
		pStaticCommandFunc m_function;
	};

	template <class T>
	class InstanceCommand final : public ICommand
	{
	public:
		InstanceCommand(const CString<128>& commandName, T* instance, const pCommandFunc<T> function)
			: ICommand(commandName), m_instance(instance), m_function(function)
		{}

		bool Invoke(const DynamicArray<CString<128>>& args, CString<256>& output) override
		{
			return (m_instance->*m_function)(args, output);
		}

	private:
		T* m_instance;
		pCommandFunc<T> m_function;
	};
}