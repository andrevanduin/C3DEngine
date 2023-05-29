
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
	 *	Default is -1 which checks the entire string
	 *
	 * @return a bool indicating if the string match case-insensitively
	 */
	bool IEquals(const char* a, const char* b, i32 length = -1);
}