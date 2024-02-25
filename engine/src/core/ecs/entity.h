
#pragma once
#include "ecs.h"
#include "entity_id.h"

namespace C3D
{
    class Entity
    {
    public:
        Entity(EntityID id) : m_id(id) {}

        EntityID Reuse(u32 index)
        {
            m_id.Reuse(index);
            m_mask.reset();
            return m_id;
        }

        bool IsValid() const { return m_id.IsValid(); }

        void Deactivate() { m_id.Invalidate(); }

        void AddComponent(u32 componentId) { m_mask.set(componentId, true); }

        void RemoveComponent(u32 componentId) { m_mask.set(componentId, false); }

        EntityID GetId() const { return m_id; }
        ComponentMask GetMask() const { return m_mask; }

    private:
        EntityID m_id;
        ComponentMask m_mask;
    };
}  // namespace C3D