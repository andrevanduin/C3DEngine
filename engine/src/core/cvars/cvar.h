
#pragma once
#include <functional>

#include "containers/array.h"
#include "containers/cstring.h"
#include "core/function/function.h"

namespace C3D
{
	using CVarName = CString<128>;
	template <typename T>
	using CVarOnChangedCallback = StackFunction<void(const T&), 16>;

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

	template<typename T>
	class CVar final : public ICVar
	{
	public:
		CVar(const CVarName& name, const T& value) : ICVar(name), m_value(value) {}

		[[nodiscard]] CString<256> ToString() const override
		{
			CString<256> str;
			str.FromFormat("{} {} = {}", TypeToString(m_value), name, m_value);
			return str;
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

		void AddOnChangedCallback(const CVarOnChangedCallback<T>& callback)
		{
			for (auto& c : m_onChangeCallbacks)
			{
				if (!c) c = callback;
			}
		}

	private:
		T m_value;
		Array<CVarOnChangedCallback<T>, 4> m_onChangeCallbacks;
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
