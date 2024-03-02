
#pragma once
#include "containers/dynamic_array.h"
#include "entity.h"

namespace C3D
{
    class ECS;

    template <typename... ComponentTypes>
    class EntityView
    {
    public:
        EntityView(const ECS& ecs) : m_ecs(&ecs)
        {
            if (sizeof...(ComponentTypes) == 0)
            {
                m_all = true;
            }
            else
            {
                // Unpack all provided component types into an array of ids
                u32 componentIds[] = { 0, ComponentTypes::GetId()... };
                // Build our mask from all the provided component ids
                for (auto id : componentIds)
                {
                    m_mask.set(id);
                }
            }
        }

        class Iterator
        {
        public:
            Iterator(const ECS* ecs, EntityIndex index, ComponentMask mask, bool all) : m_ecs(ecs), m_index(index), m_mask(mask), m_all(all)
            {}

            bool IsValid() const
            {
                auto& entity = m_ecs->m_entities[m_index];
                return entity.IsValid() && (m_all || m_mask == (m_mask & entity.GetMask()));
            }

            EntityID operator*() const
            {
                auto& entity = m_ecs->m_entities[m_index];
                return entity.GetId();
            }

            bool operator==(const Iterator& other) const { return m_index == other.m_index; }

            bool operator!=(const Iterator& other) const { return m_index != other.m_index; }

            Iterator& operator++()
            {
                do
                {
                    m_index++;
                } while (m_index < m_ecs->m_entities.Size() && !IsValid());
                return *this;
            }

        private:
            EntityIndex m_index;
            ComponentMask m_mask;
            bool m_all = false;

            const ECS* m_ecs;
        };

        const Iterator begin() const
        {
            EntityIndex first = 0;
            while (first < m_ecs->m_entities.Size() &&
                   (m_mask != (m_mask & m_ecs->m_entities[first].GetMask()) || !m_ecs->m_entities[first].IsValid()))
            {
                first++;
            }
            return Iterator(m_ecs, first, m_mask, m_all);
        }

        const Iterator end() const { return Iterator(m_ecs, m_ecs->m_entities.Size(), m_mask, m_all); }

    private:
        ComponentMask m_mask;
        bool m_all = false;

        const ECS* m_ecs;
    };
}  // namespace C3D