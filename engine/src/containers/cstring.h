
// ReSharper disable CppNonExplicitConvertingConstructor
// ReSharper disable CppInconsistentNaming
#pragma once
#include <format>

#include "core/defines.h"

namespace C3D
{
	template <u64 CCapacity>
	class __declspec(dllexport) CString
	{
	public:
		CString() : m_data{} {}

		constexpr CString(const char* str, const u64 size)
			: m_data{}
		{
			if (size >= CCapacity)
			{
				throw std::invalid_argument("Provided size is larger then available capacity");
			}
			Create(str, size);
		}

		constexpr CString(const char* str)
			: m_data{}
		{
			const auto size = std::strlen(str);
			if (size >= CCapacity)
			{
				throw std::invalid_argument("Provided size is larger then available capacity");
			}
			Create(str, size);
		}

		explicit CString(const u64 value)
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

		void Append(const char c)
		{
			const auto thisSize = Size();

			if (thisSize + 1 >= CCapacity)
			{
				throw std::invalid_argument("this.Size() + other.Size() >= Capacity");
			}

			m_data[thisSize] = c;
			m_data[thisSize + 1] = '\0';
		}

		void Append(const char* other)
		{
			const auto thisSize = Size();
			const auto otherSize = std::strlen(other);

			if (thisSize + otherSize >= CCapacity)
			{
				throw std::invalid_argument("this.Size() + other.Size() >= Capacity");
			}

			// Copy over the characters from the other string
			std::memcpy(m_data + thisSize, other, otherSize);
			// Ensure that our newly appended string is null-terminated
			m_data[thisSize + otherSize] = '\0';
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
			auto endIt = std::vformat_to(m_data, format, std::make_format_args(args...));
			// Get the size of the newly created string
			auto size = endIt - m_data;
			// Ensure that the string is null terminated
			m_data[size] = '\0';
		}

		/* @brief Removes all starting whitespace characters from the string. */
		void TrimLeft()
		{
			u64 size = Size();
			u64 newStart = 0;
			
			// Find the first non-space character
			while (std::isspace(m_data[newStart]))
			{
				++newStart;
			}
			// If the first character is a non-space character we do nothing
			if (newStart == 0) return;
			// Decrement the size by however many characters we have removed
			size -= newStart;
			// Copy over the remaining characters
			std::memcpy(m_data, m_data + newStart, size);
			// Add a null termination character
			m_data[size] = '\0';
		}

		/* @brief Removes all the trailing whitespace characters from the string. */
		void TrimRight()
		{
			u64 size = Size();
			u64 newSize = size;
			// Find the first no-space character at the end
			while (std::isspace(m_data[newSize - 1]))
			{
				--newSize;
			}
			// If the last character is a non-space character we do nothing
			if (newSize == size) return;
			// We save off our new size
			size = newSize;
			// Set the null termination character to end our string
			m_data[size] = '\0';
		}

		/* @brief Remove all the starting and trailing whitespace characters from the string. */
		void Trim()
		{
			TrimLeft();
			TrimRight();
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

		/* @brief Check if CString matches case-sensitive. */
		template <u64 OtherCapacity>
		[[nodiscard]] bool Equals(const CString<OtherCapacity>& other) const
		{
			return std::strcmp(m_data, other.m_data) == 0;
		}

		/* @brief Check if const char pointer matches case-insensitive. */
		[[nodiscard]] bool IEquals(const char* other) const
		{
			return _stricmp(m_data, other) == 0;
		}

		/* @brief Check if CString matches case-insensitive. */
		template <u64 OtherCapacity>
		[[nodiscard]] bool IEquals(const CString<OtherCapacity>& other) const
		{
			return std::strcmp(m_data, other.m_data) == 0;
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

		const char& operator[](const u64 index) const
		{
			return m_data[index];
		}

		CString& operator+=(const char c)
		{
			Append(c);
			return *this;
		} 

		CString& operator+=(const char* other)
		{
			Append(other);
			return *this;
		}

		CString& operator+=(const CString& other)
		{
			Append(other);
			return *this;
		}

		template <u64 OtherCapacity>
		CString& operator+=(const CString<OtherCapacity>& other)
		{
			Append(other);
			return *this;
		}

		bool operator==(const char* other) const
		{
			return Equals(other);
		}

		template <u64 OtherCapacity>
		bool operator==(const CString<OtherCapacity>& other)
		{
			return Equals(other);
		}

		/* @brief Returns an iterator pointing to the start of the character array. */
		[[nodiscard]] char* begin() noexcept { return m_data; }

		/* @brief Returns a const_iterator pointing to the start of the character array. */
		[[nodiscard]] const char* begin() const noexcept { return m_data; }

		/* @brief Returns a const_iterator pointing to the start of the character array. */
		[[nodiscard]] const char* cbegin() const noexcept { return m_data; }

		/* @brief Returns an iterator pointing to the element right after the last character in the character array. */
		[[nodiscard]] char* end() noexcept { return m_data + Size(); }

		/* @brief Returns a const_iterator pointing to the element right after the last character in the character array. */
		[[nodiscard]] const char* end() const noexcept { return m_data + Size(); }

		/* @brief Returns a const_iterator pointing to the element right after the last character in the character array. */
		[[nodiscard]] const char* cend() const noexcept { return m_data + Size(); }

	private:
		void Create(const char* str, const u64 size)
		{
			std::memcpy(m_data, str, size);
			// We end our string with a '\0' character
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

template <u64 CCapacity>
struct std::formatter<C3D::CString<CCapacity>>
{
	template<typename ParseContext>
	static auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const C3D::CString<CCapacity>& str, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(str.Data()));
	}
};

template <u64 CCapacity>
struct std::hash<C3D::CString<CCapacity>>
{
	size_t operator() (const C3D::CString<CCapacity>& key) const noexcept
	{
		size_t hash = 0;
		for (const auto c : key)
		{
			hash ^= static_cast<size_t>(c);
			hash *= std::_FNV_prime;
		}
		return hash;
	}
};
