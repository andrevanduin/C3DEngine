
#pragma once
#include <format>
#include <vulkan/vulkan.h>

template <>
struct std::formatter<VkAttachmentLoadOp>
{
	template <typename ParseContext>
	static auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const VkAttachmentLoadOp attachmentLoadOp, FormatContext& ctx)
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(static_cast<i64>(attachmentLoadOp)));
	}
};

template <>
struct std::formatter<VkResult>
{
	template <typename ParseContext>
	static auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const VkResult vkResult, FormatContext& ctx)
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(static_cast<i64>(vkResult)));
	}
};