
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "ecs.h"

namespace C3D
{
    class EntityID
    {
    public:
        EntityID() {}
        EntityID(u32 index) : m_index(index) {}

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

    private:
        EntityIndex m_index     = INVALID_ID;
        EntityVersion m_version = 0;
    };

    static String ToString(EntityID id) { return String::FromFormat("EntityID(Index = {}, Version = {})", id.GetIndex(), id.GetVersion()); }
}  // namespace C3D