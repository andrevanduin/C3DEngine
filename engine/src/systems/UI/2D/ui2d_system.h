
#pragma once
#include "UI/2D/components.h"
#include "UI/2D/control.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/ecs/component_pool.h"
#include "core/ecs/entity.h"
#include "memory/allocators/dynamic_allocator.h"
#include "systems/system.h"

namespace C3D
{
    struct UI2DSystemConfig
    {
        u32 maxControlCount = 64;
        u64 allocatorSize   = MebiBytes(8);
    };

    class C3D_API UI2DSystem final : public SystemWithConfig<UI2DSystemConfig>
    {
    public:
        explicit UI2DSystem(const SystemManager* pSystemsManager);

        bool OnInit(const UI2DSystemConfig& config) override;

        bool OnUpdate(const FrameData& frameData) override;
        bool OnRender(const FrameData& frameData);

        void OnShutdown() override;

        EntityID Register();
        bool Deactivate(EntityID id);

        template <typename ComponentType>
        ComponentType& AddComponent(EntityID id)
        {
            auto index       = id.GetIndex();
            auto componentId = ComponentType::GetId();
            auto pComponent  = m_componentPools[componentId].Allocate<ComponentType>(index);

            m_entities[index].AddComponent(componentId);
            return *pComponent;
        }

        template <typename ComponentType>
        void RemoveComponent(EntityID id)
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

    private:
        DynamicArray<ComponentPool<DynamicAllocator>> m_componentPools;
        DynamicArray<Entity> m_entities;
        DynamicArray<u32> m_freeIndices;

        DynamicAllocator m_allocator;
    };
}  // namespace C3D
