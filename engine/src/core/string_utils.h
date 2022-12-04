
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
}