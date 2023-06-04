
#pragma once
#include <fmt/core.h>

#include "containers/string.h"

namespace C3D::StringUtils
{
	/* @brief Builds a string from the format and the provided arguments.
	 * Internally uses fmt::format to build out the string.
	 */
	template <class Allocator, typename... Args>
	static BasicString<Allocator> FromFormat(Allocator* allocator, const char* format, Args&&... args)
	{
		BasicString<Allocator> buffer(allocator);
		fmt::format_to(std::back_inserter(buffer), format, std::forward<Args>(args)...);
		return buffer;
	}

	/*
	 * @brief Compares two strings case-sensitive
	 *
	 * @param left The string you want to compare
	 * @param right The string you want to compare to
	 * @param length The maximum number of characters we should compare.
	 *	Default is -1 which checks the entire string
	 *
	 * @return a bool indicating if the strings match case-sensitively
	 */
	bool Equals(const char* a, const char* b, i32 length = -1);

	/*
	 * @brief Compares two strings case-insensitive
	 *
	 * @param left The string you want to compare
	 * @param right The string you want to compare to
	 * @param length The maximum number of characters we should compare.
	 * Default is -1 which checks the entire string
	 *
	 * @return a bool indicating if the string match case-insensitively
	 */
	bool IEquals(const char* a, const char* b, i32 length = -1);

	/*
	 * @brief Splits a CString on the provided delimiter
	 *
	 * @param string The CString that you want to split
	 * @param delimiter The char that you want to split the string on
	 * @param trimEntries An optional bool to enable the split strings to be trimmed (no started and ending whitespace)
	 * @param skipEmpty An optional bool to enable skipping empty strings in the split array
	 */
	template <u64 CCapacity, u64 OutputCapacity = CCapacity>
	[[nodiscard]] DynamicArray<CString<OutputCapacity>> Split(const CString<CCapacity>& string, char delimiter, const bool trimEntries = true, const bool skipEmpty = true)
	{
		DynamicArray<CString<OutputCapacity>> elements;
		CString<OutputCapacity> current;

		const auto size = string.Size();
		for (u64 i = 0; i < size; i++)
		{
			if (string[i] == delimiter)
			{
				if (!skipEmpty || !current.Empty())
				{
					if (trimEntries) current.Trim();

					elements.PushBack(current);
					current.Clear();
				}
			}
			else
			{
				current.Append(string[i]);
			}
		}

		if (!current.Empty())
		{
			if (trimEntries) current.Trim();
			elements.PushBack(current);
		}
		return elements;
	}

	/*
	 * @brief Checks if provided string is empty or only contains whitespace
	 *
	 * @param string The string that you want to check.
	 */
	template <u64 CCapacity>
	bool IsEmptyOrWhitespaceOnly(const CString<CCapacity>& string)
	{
		// We can stop early if our string does not contain any characters
		if (string[0] == '\0') return true;
		// Otherwise we parse all chars in the string and check if they are all whitespace
		for (const auto c : string)
		{
			// If we find a non-whitespace character we stop our search
			if (!std::isspace(c)) return false;
		}
		return true;
	}
}