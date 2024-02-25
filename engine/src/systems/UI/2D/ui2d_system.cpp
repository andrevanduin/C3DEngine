#include "ui2d_system.h"

#include "UI/2D/flags.h"
#include "core/ecs/entity_view.h"
#include "core/frame_data.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    UI2DSystem::UI2DSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager) {}

    bool UI2DSystem::OnInit(const UI2DSystemConfig& config)
    {
        INFO_LOG("Initializing.");

        if (config.maxControlCount == 0)
        {
            ERROR_LOG("Maximum amount of UI2D controls must be > 0.");
            return false;
        }

        if (config.allocatorSize == 0)
        {
            ERROR_LOG("Allocator size must be > 0.");
        }

        // Allocate enough space for our control allocator
        u64 usableMemory = MebiBytes(config.allocatorSize);
        u64 neededMemory = DynamicAllocator::GetMemoryRequirements(usableMemory);

        // Create our own dynamic allocator
        const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);
        m_allocator.Create(memoryBlock, neededMemory, usableMemory);

        // Use our own dynamic allocator for the component pools so we always allocate from the same block
        m_componentPools.SetAllocator(&m_allocator);

        // Reserve enough space for a component pool for every UI2D component that we have
        m_componentPools.Reserve(UI_2D::COMPONENT_COUNT);

        return true;
    }

    EntityID UI2DSystem::Register()
    {
        EntityID id;

        if (!m_freeIndices.Empty())
        {
            // There are free indices available so let's use those instead
            auto index = m_freeIndices.PopBack();
            id         = m_entities[index].Reuse(index);

            INFO_LOG("Registered entity with reused ID: {}", ToString(id));
        }
        else
        {
            // No free indices so we append an entity to the end
            id = EntityID(m_entities.Size());
            m_entities.EmplaceBack(id);

            INFO_LOG("Registered entity with new ID: {}", ToString(id));
        }

        return id;
    }

    bool UI2DSystem::Deactivate(EntityID id)
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

    bool UI2DSystem::OnUpdate(const FrameData& frameData) { return true; }

    bool UI2DSystem::OnRender(const FrameData& frameData)
    {
        using namespace UI_2D;

        for (auto entityId : EntityView<TransformComponent>(m_entities))
        {
            auto& transformComponent = GetComponent<TransformComponent>(entityId);
            // TODO: Rendering
        }

        return true;
    }

    void UI2DSystem::OnShutdown() { INFO_LOG("Shutting down UI2D System."); }
}  // namespace C3D
