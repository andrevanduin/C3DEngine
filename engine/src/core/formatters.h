
#pragma once
#include <format>

template <>
struct std::formatter<std::thread::id>
{
	template<typename ParseContext>
	static auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format(const std::thread::id& id, FormatContext& ctx)
	{
		const auto hash = std::hash<std::thread::id>{}(id);
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(hash));
	}
};