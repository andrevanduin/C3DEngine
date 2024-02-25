
#pragma once
#include "containers/dynamic_array.h"
#include "entity.h"

namespace C3D
{
    template <typename... ComponentTypes>
    class EntityView
    {
    public:
        EntityView(const DynamicArray<Entity>& entities) : m_pEntities(&entities)
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
            Iterator(const DynamicArray<Entity>* pEntities, EntityIndex index, ComponentMask mask, bool all)
                : m_pEntities(pEntities), m_index(index), m_mask(mask), m_all(all)
            {}

            bool IsValid() const
            {
                auto& entity = (*m_pEntities)[m_index];
                return entity.IsValid() && (m_all || m_mask == (m_mask & entity.GetMask()));
            }

            EntityID operator*() const
            {
                auto& entity = (*m_pEntities)[m_index];
                return entity.GetId();
            }

            bool operator==(const Iterator& other) const { return m_index == other.m_index; }

            bool operator!=(const Iterator& other) const { return m_index != other.m_index; }

            Iterator& operator++()
            {
                do
                {
                    m_index++;
                } while (m_index < m_pEntities->Size() && IsValid());
                return *this;
            }

        private:
            EntityIndex m_index;
            ComponentMask m_mask;
            bool m_all = false;

            const DynamicArray<Entity>* m_pEntities;
        };

        const Iterator begin() const
        {
            EntityIndex first = 0;
            while (first < m_pEntities->Size() &&
                   (m_mask != (m_mask & (*m_pEntities)[first].GetMask()) || !(*m_pEntities)[first].IsValid()))
            {
                first++;
            }
            return Iterator(m_pEntities, first, m_mask, m_all);
        }

        const Iterator end() const { return Iterator(m_pEntities, m_pEntities->Size(), m_mask, m_all); }

    private:
        ComponentMask m_mask;
        bool m_all = false;

        const DynamicArray<Entity>* m_pEntities;
    };
}  // namespace C3D