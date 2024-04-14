
#pragma once
#include "component_pool.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "defines.h"
#include "entity_description.h"

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

        Entity Register();

        bool Deactivate(Entity entity);

        template <typename ComponentType>
        ComponentType& AddComponent(Entity entity) const
        {
            auto index       = entity.GetIndex();
            auto componentId = ComponentType::GetId();
            auto pComponent  = m_componentPools[componentId].Allocate<ComponentType>(index);

            m_entities[index].AddComponent(componentId);
            return *pComponent;
        }

        template <typename ComponentType>
        void RemoveComponent(Entity entity) const
        {
            auto index       = entity.GetIndex();
            auto componentId = ComponentType::GetId();

            m_entities[index].RemoveComponent(componentId);
        }

        template <typename ComponentType>
        ComponentType& GetComponent(Entity entity)
        {
            auto index       = entity.GetIndex();
            auto componentId = ComponentType::GetId();

            return m_componentPools[componentId].Get<ComponentType>(index);
        }

        template <typename ComponentType>
        const ComponentType& GetComponent(Entity entity) const
        {
            auto index       = entity.GetIndex();
            auto componentId = ComponentType::GetId();

            return m_componentPools[componentId].Get<ComponentType>(index);
        }

        /**
         * @brief Gets the requested component. If the component does not yet exist on this entity it gets added first.
         *
         * @param entity The entity that you want to get/add the component from/to
         * @return A reference to the component on the provided entity
         */
        template <typename ComponentType>
        ComponentType& GetOrAddComponent(Entity entity)
        {
            auto index       = entity.GetIndex();
            auto componentId = ComponentType::GetId();

            if (m_entities[index].HasComponent(componentId))
            {
                return m_componentPools[componentId].Get<ComponentType>(index);
            }

            auto pComponent = m_componentPools[componentId].Allocate<ComponentType>(index);

            m_entities[index].AddComponent(componentId);
            return *pComponent;
        }

        template <typename ComponentType>
        bool HasComponent(Entity entity) const
        {
            auto index       = entity.GetIndex();
            auto componentId = ComponentType::GetId();

            return m_entities[index].HasComponent(componentId);
        }

    private:
        u64 m_maxComponents = 0;

        DynamicArray<ComponentPool<DynamicAllocator>> m_componentPools;
        mutable DynamicArray<EntityDescription> m_entities;
        DynamicArray<u32> m_freeIndices;

        DynamicAllocator m_allocator;
        void* m_memoryBlock = nullptr;

        const SystemManager* m_pSystemsManager;

        template <typename... ComponentTypes>
        friend class EntityView;
    };
}  // namespace C3D