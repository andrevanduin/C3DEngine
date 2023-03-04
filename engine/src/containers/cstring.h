
// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once
#include "core/defines.h"

namespace C3D
{
	template <u64 CCapacity>
	class __declspec(dllexport) CString
	{
	public:
		CString() : m_data{} {}

		constexpr CString(const char* str, const u64 size)
		{
			static_assert(size < Capacity);
			Create(str, size);
		}

		constexpr CString(const char* str)
		{
			const auto size = std::strlen(str);
			if (size >= CCapacity)
			{
				throw std::invalid_argument("Provided size is larger then available capacity");
			}
			Create(str, size);
		}

		CString(const u64 value)
			: m_data{}
		{
			const auto str = std::to_string(value);
			std::memcpy(m_data, str.c_str(), str.size());
		}

		CString& operator=(const char* str)
		{
			const auto size = std::strlen(str);
			if (size >= CCapacity)
			{
				throw std::invalid_argument("Provided size is larger then available capacity");
			}
			Create(str, size);
			return *this;
		}

		CString SubString(const u64 first, u64 count)
		{
			static_assert(first + count < CCapacity);
			return CString(m_data + first, count);
		}

		template <u64 Cap>
		void Append(const CString<Cap>& other)
		{
			const auto thisSize = Size();
			const auto otherSize = other.Size();

			if (thisSize + otherSize >= CCapacity)
			{
				throw std::invalid_argument("this.Size() + other.Size() >= Capacity");
			}

			// Copy over the characters from the other string
			std::memcpy(m_data + thisSize, other.Data(), otherSize);
			// Ensure that our newly appended string is null-terminated
			m_data[thisSize + otherSize] = '\0';
		}

		/* @brief Builds a cstring from the format and the provided arguments.
		 * Internally uses fmt::format to build out the string.
		 */
		template <typename... Args>
		void FromFormat(const char* format, Args&&... args)
		{
			auto endIt = fmt::format_to_n(m_data, CCapacity, format, std::forward<Args>(args)...);
			// Ensure that the string is null terminated
			m_data[endIt.size] = '\0';
		}

		/* @brief Clears the string. Resulting in an empty null-terminated string. */
		void Clear()
		{
			m_data[0] = '\0';
		}

		/* @brief Check if const char pointer matches case-sensitive. */
		[[nodiscard]] bool Equals(const char* other) const
		{
			return std::strcmp(m_data, other) == 0;
		}

		/* @brief Check if const char pointer matches case-insensitive. */
		[[nodiscard]] bool IEquals(const char* other) const
		{
			return _stricmp(m_data, other) == 0;
		}

		[[nodiscard]] bool Empty() const
		{
			return Size() == 0;
		}

		[[nodiscard]] static u64 Capacity()
		{
			return CCapacity;
		}

		[[nodiscard]] u64 Size() const
		{
			return std::strlen(m_data);
		}

		[[nodiscard]] char* Data()
		{
			return m_data;
		}

		[[nodiscard]] const char* Data() const
		{
			return m_data;
		}

		char& operator[](const u64 index)
		{
			return m_data[index];
		}

		CString& operator+=(const CString& other)
		{
			Append(other);
			return *this;
		}

	private:
		void Create(const char* str, const u64 size)
		{
			std::memcpy(m_data, str, size);
			m_data[size] = '\0';
		}

		char m_data[CCapacity];
	};

	template <u64 CCapacity>
	std::ostream& operator<< (std::ostream& cout, const CString<CCapacity>& str)
	{
		cout << str.Data();
		return cout;
	}
}

template<u64 CCapacity>
struct fmt::formatter<C3D::CString<CCapacity>>
{
	template<typename ParseContext>
	static constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template<typename FormatContext>
	auto format(C3D::CString<CCapacity> const& str, FormatContext& ctx)
	{
		return fmt::format_to(ctx.out(), "{}", str.Data());
	}
};