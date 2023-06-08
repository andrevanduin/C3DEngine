
#pragma once
#include <functional>

#include "containers/array.h"
#include "containers/cstring.h"
#include "core/callable/callable.h"

namespace C3D
{
	using CVarName = CString<128>;

	template <typename T>
	using CVarCallable = ICallable<const T&>;
	template <typename T>
	using CVarStaticCallable = StaticCallable<bool(*)(const T&), const T&>;
	template <typename Instance, typename T>
	using CVarInstanceCallable = InstanceCallable<Instance, bool(Instance::*)(const T&), const T&>;
	template <typename Instance, typename T>
	using CVarCInstanceCallable = InstanceCallable<Instance, bool(Instance::*)(const T&) const, const T&>;

	class ICVar
	{
	public:
		explicit ICVar(const CVarName& name) : name(name) {}

		ICVar(const ICVar&) = delete;
		ICVar(ICVar&&) = delete;

		ICVar& operator=(const ICVar&) = delete;
		ICVar& operator=(ICVar&&) = delete;

		virtual ~ICVar() = default;

		CVarName name;

		[[nodiscard]] virtual CString<256> ToString() const = 0;
	};

	template <typename T>
	using CVarOnChangedCallback = std::function<void(const T& value)>;

	template<typename T>
	class CVar final : public ICVar
	{
	public:
		CVar(const CVarName& name, const T& value) : ICVar(name), m_value(value), m_onChangeCallbacks(nullptr) {}

		[[nodiscard]] CString<256> ToString() const override
		{
			return CString<256>::FromFormat("{} {} = {}", TypeToString(m_value), name, m_value);
		}

		void SetValue(const T& value)
		{
			m_value = value;
			for (auto& c : m_onChangeCallbacks)
			{
				// For all valid callbacks we inform that the value has been changed
				if (c) c(m_value);
			}
		}

		void AddOnChangedCallback(CVarCallable<T>* callback)
		{
			for (auto& c : m_onChangeCallbacks)
			{
				if (!c) c = callback;
			}
		}

	private:
		T m_value;
		Array<CVarCallable<T>*, 4> m_onChangeCallbacks;
	};

	template <>
	CString<256> CVar<u8>::ToString() const;

	template <>
	CString<256> CVar<u16>::ToString() const;

	template <>
	CString<256> CVar<u32>::ToString() const;

	template <>
	CString<256> CVar<u64>::ToString() const;

	template <>
	CString<256> CVar<i8>::ToString() const;

	template <>
	CString<256> CVar<i16>::ToString() const;

	template <>
	CString<256> CVar<i32>::ToString() const;

	template <>
	CString<256> CVar<i64>::ToString() const;

	template <>
	CString<256> CVar<f32>::ToString() const;

	template <>
	CString<256> CVar<f64>::ToString() const;

	template <>
	CString<256> CVar<bool>::ToString() const;
}
