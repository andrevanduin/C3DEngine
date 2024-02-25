
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "memory/global_memory_system.h"

namespace C3D
{
    template <class Allocator = DynamicAllocator>
    class ComponentPool
    {
        static_assert(std::is_base_of_v<BaseAllocator<Allocator>, Allocator>, "Allocator must derive from BaseAllocator");

    public:
        ComponentPool(Allocator* allocator = BaseAllocator<Allocator>::GetDefault()) : m_allocator(allocator) {}

        template <typename Type>
        bool Create(const String& name, u64 maxComponents)
        {
            INSTANCE_INFO_LOG("COMPONENT_POOL", "Creating: '{}' with room for: '{}' components", name, maxComponents);

            m_name          = name;
            m_maxComponents = maxComponents;
            m_components    = Allocator(m_allocator)->AllocateBlock(MemoryType::ECS, maxComponents * sizeof(Type));
        }

        void Destroy()
        {
            INSTANCE_INFO_LOG("COMPONENT_POOL", "Destroying: '{}'.", m_name);
            m_allocator->Free(m_components);
        }

        template <typename Type>
        Type* Allocate(EntityIndex index)
        {
            Type* pComponent = new (m_components + (index * sizeof(Type))) Type();
            return pComponent;
        }

        template <typename Type>
        Type& Get(EntityIndex index)
        {
            return *reinterpret_cast<Type*>(m_components + (index * sizeof(Type)));
        }

    private:
        String m_name;
        u64 m_maxComponents;

        char* m_components;
        const Allocator* m_allocator = nullptr;
    };
}  // namespace C3D