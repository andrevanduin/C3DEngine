
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "defines.h"

namespace C3D
{
    class Entity
    {
    public:
        Entity() {}
        Entity(u32 index) : m_index(index) {}

        EntityIndex GetIndex() const { return m_index; }
        EntityVersion GetVersion() const { return m_version; }

        bool IsValid() const { return m_index != INVALID_ID; }

        void Invalidate()
        {
            // Set index to INVALID_ID so we know this entity is no longer valid
            m_index = INVALID_ID;
        }

        void Reuse(u32 index)
        {
            m_index = index;
            // Increment the version to ensure that this reused entity id is unique
            m_version++;
        }

        static Entity Invalid() { return Entity(INVALID_ID); }

    private:
        EntityIndex m_index     = INVALID_ID;
        EntityVersion m_version = 0;
    };
}  // namespace C3D

template <>
struct fmt::formatter<C3D::Entity>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const C3D::Entity& entity, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "Entity(Index = {}, Version = {})", entity.GetIndex(), entity.GetVersion());
    }
};