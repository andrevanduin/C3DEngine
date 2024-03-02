
#pragma once
#include "component_pool.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "defines.h"
#include "entity.h"

namespace C3D
{
    class SystemManager;

    class C3D_API ECS
    {
    public:
        bool Create(const SystemManager* pSystemsManager, u64 memorySize, u64 componentTypeCount, u64 maxComponents);
        void Destroy();

        template <typename ComponentType>
        bool AddComponentPool(const String& name)
        {
            auto componentId = ComponentType::GetId();
            if (componentId > m_componentPools.Size())
            {
                INSTANCE_ERROR_LOG("ECS", "ComponentId falls outside of range of Component Pools.");
                return false;
            }

            m_componentPools[componentId].Create<ComponentType>(name, m_maxComponents, &m_allocator);
            return true;
        }

        EntityID Register();

        bool Deactivate(EntityID id);

        template <typename ComponentType>
        ComponentType& AddComponent(EntityID id) const
        {
            auto index       = id.GetIndex();
            auto componentId = ComponentType::GetId();
            auto pComponent  = m_componentPools[componentId].Allocate<ComponentType>(index);

            m_entities[index].AddComponent(componentId);
            return *pComponent;
        }

        template <typename ComponentType>
        void RemoveComponent(EntityID id) const
        {
            auto index       = id.GetIndex();
            auto componentId = ComponentType::GetId();

            m_entities[index].RemoveComponent(componentId);
        }

        template <typename ComponentType>
        ComponentType& GetComponent(EntityID id)
        {
            auto index       = id.GetIndex();
            auto componentId = ComponentType::GetId();

            return m_componentPools[componentId].Get<ComponentType>(index);
        }

        template <typename ComponentType>
        bool HasComponent(EntityID id) const
        {
            auto index       = id.GetIndex();
            auto componentId = ComponentType::GetId();

            return m_entities[index].HasComponent(componentId);
        }

    private:
        u64 m_maxComponents = 0;

        DynamicArray<ComponentPool<DynamicAllocator>> m_componentPools;
        mutable DynamicArray<Entity> m_entities;
        DynamicArray<u32> m_freeIndices;

        DynamicAllocator m_allocator;
        void* m_memoryBlock = nullptr;

        const SystemManager* m_pSystemsManager;

        template <typename... ComponentTypes>
        friend class EntityView;
    };
}  // namespace C3D