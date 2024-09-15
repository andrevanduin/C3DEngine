
#pragma once
#include "ecs_types.h"
#include "entity.h"

namespace C3D
{
    class EntityDescription
    {
    public:
        EntityDescription(Entity id) : m_entity(id) {}

        Entity Reuse(u32 index)
        {
            m_entity.Reuse(index);
            m_mask.reset();
            return m_entity;
        }

        bool IsValid() const { return m_entity.IsValid(); }

        void Deactivate() { m_entity.Invalidate(); }

        void AddComponent(u32 componentId) { m_mask.set(componentId, true); }

        void RemoveComponent(u32 componentId) { m_mask.set(componentId, false); }

        bool HasComponent(u32 componentId) const { return m_mask.test(componentId); }

        Entity Get() const { return m_entity; }
        ComponentMask GetMask() const { return m_mask; }

    private:
        Entity m_entity;
        ComponentMask m_mask;
    };
}  // namespace C3D