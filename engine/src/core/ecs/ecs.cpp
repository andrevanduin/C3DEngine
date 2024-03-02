
#include "ecs.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "ECS";

    bool ECS::Create(const SystemManager* pSystemsManager, u64 memorySize, u64 componentTypeCount, u64 maxComponents)
    {
        m_pSystemsManager = pSystemsManager;

        if (memorySize < MebiBytes(8))
        {
            ERROR_LOG("An ECS requires at least 8MebiBytes of memory.");
            return false;
        }

        if (componentTypeCount > MAX_COMPONENTS_TYPES)
        {
            ERROR_LOG("Tried creating an ECS with: {} components types. Which is greater than the max of: {}.", componentTypeCount,
                      MAX_COMPONENTS_TYPES);
            return false;
        }

        if (maxComponents == 0)
        {
            ERROR_LOG("To create an ECS you need to have maxComponents > 0.", maxComponents);
            return false;
        }

        m_maxComponents = maxComponents;

        // Allocate enough space for our control allocator
        u64 neededMemory = DynamicAllocator::GetMemoryRequirements(memorySize);

        // Create our own dynamic allocator
        m_memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);
        m_allocator.Create(m_memoryBlock, neededMemory, memorySize);

        // Use our own dynamic allocator for the component pools so we always allocate from the same block
        m_componentPools.SetAllocator(&m_allocator);

        // Create a component pool for every component type that we have
        m_componentPools.Resize(componentTypeCount);

        return true;
    }

    void ECS::Destroy()
    {
        // Destroy all our pools
        for (auto& pool : m_componentPools)
        {
            pool.Destroy();
        }
        // Destroy our arrays
        m_componentPools.Destroy();
        m_entities.Destroy();
        m_freeIndices.Destroy();

        // Destroy our internally used allocator
        m_allocator.Destroy();

        // Free our internal memory block
        Memory.Free(m_memoryBlock);
    }

    EntityID ECS::Register()
    {
        EntityID id;

        if (!m_freeIndices.Empty())
        {
            // There are free indices available so let's use those instead
            auto index = m_freeIndices.PopBack();
            id         = m_entities[index].Reuse(index);

            INFO_LOG("Registered entity with reused ID: {}.", ToString(id));
        }
        else
        {
            // No free indices so we append an entity to the end
            id = EntityID(m_entities.Size());
            m_entities.EmplaceBack(id);

            INFO_LOG("Registered entity with new ID: {}.", ToString(id));
        }

        return id;
    }

    bool ECS::Deactivate(EntityID id)
    {
        auto index = id.GetIndex();
        if (id.IsValid())
        {
            ERROR_LOG("Provided id is invalid.");
            return false;
        }

        if (index >= m_entities.Size())
        {
            ERROR_LOG("Provided id is invalid since it is outside the entity range.");
            return false;
        }

        auto& entity = m_entities[index];
        if (entity.IsValid())
        {
            ERROR_LOG("The entity at the provided id is invalid.");
            return false;
        }

        // Entity is found and valid. Let's deactivate
        entity.Deactivate();

        // Mark this index as free
        m_freeIndices.PushBack(index);

        // Sort the free indices array from high indices to low to ensure that when we reuse
        // we start with the lowest indices first to ensure we minimize fragmentation
        std::sort(m_freeIndices.begin(), m_freeIndices.end(), std::greater<u32>());

        INFO_LOG("Deactivated entity with id: '{}'.", ToString(id));

        return true;
    }
}  // namespace C3D