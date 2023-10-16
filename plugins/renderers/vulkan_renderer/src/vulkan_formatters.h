
#pragma once
#include <fmt/format.h>
#include <vulkan/vulkan.h>

template <>
struct fmt::formatter<VkAttachmentLoadOp>
{
    template <typename ParseContext>
    static auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const VkAttachmentLoadOp attachmentLoadOp, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", static_cast<i64>(attachmentLoadOp));
    }
};

template <>
struct fmt::formatter<VkResult>
{
    template <typename ParseContext>
    static auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const VkResult vkResult, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", static_cast<i64>(vkResult));
    }
};