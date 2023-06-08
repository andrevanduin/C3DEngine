
#include "cvar.h"

namespace C3D
{
	template <>
	CString<256> CVar<u8>::ToString() const
	{
		CString<256> str;
		str.FromFormat("u8 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<u16>::ToString() const
	{
		CString<256> str;
		str.FromFormat("u16 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<u32>::ToString() const
	{
		CString<256> str;
		str.FromFormat("u32 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<u64>::ToString() const
	{
		CString<256> str;
		str.FromFormat("u64 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<i8>::ToString() const
	{
		CString<256> str;
		str.FromFormat("i8 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<i16>::ToString() const
	{
		CString<256> str;
		str.FromFormat("i16 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<i32>::ToString() const
	{
		CString<256> str;
		str.FromFormat("i32 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<i64>::ToString() const
	{
		CString<256> str;
		str.FromFormat("i64 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<f32>::ToString() const
	{
		CString<256> str;
		str.FromFormat("f32 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<f64>::ToString() const
	{
		CString<256> str;
		str.FromFormat("f64 {} = {}", name, m_value);
		return str;
	}

	template <>
	CString<256> CVar<bool>::ToString() const
	{
		CString<256> str;
		str.FromFormat("bool {} = {}", name, m_value);
		return str;
	}
}