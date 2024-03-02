
#pragma once
#include <type_traits>

#include "core/defines.h"
#include "memory/allocators/linear_allocator.h"
#include "system.h"

#define Input m_pSystemsManager->GetSystem<C3D::InputSystem>(C3D::SystemType::InputSystemType)
#define Event m_pSystemsManager->GetSystem<C3D::EventSystem>(C3D::SystemType::EventSystemType)
#define Renderer m_pSystemsManager->GetSystem<C3D::RenderSystem>(C3D::SystemType::RenderSystemType)
#define Textures m_pSystemsManager->GetSystem<C3D::TextureSystem>(C3D::SystemType::TextureSystemType)
#define Materials m_pSystemsManager->GetSystem<C3D::MaterialSystem>(C3D::SystemType::MaterialSystemType)
#define Geometric m_pSystemsManager->GetSystem<C3D::GeometrySystem>(C3D::SystemType::GeometrySystemType)
#define Resources m_pSystemsManager->GetSystem<C3D::ResourceSystem>(C3D::SystemType::ResourceSystemType)
#define Shaders m_pSystemsManager->GetSystem<C3D::ShaderSystem>(C3D::SystemType::ShaderSystemType)
#define Lights m_pSystemsManager->GetSystem<C3D::LightSystem>(C3D::SystemType::LightSystemType)
#define Cam m_pSystemsManager->GetSystem<C3D::CameraSystem>(C3D::SystemType::CameraSystemType)
#define Jobs m_pSystemsManager->GetSystem<C3D::JobSystem>(C3D::SystemType::JobSystemType)
#define Fonts m_pSystemsManager->GetSystem<C3D::FontSystem>(C3D::SystemType::FontSystemType)
#define CVars m_pSystemsManager->GetSystem<C3D::CVarSystem>(C3D::SystemType::CVarSystemType)
#define OS m_pSystemsManager->GetSystem<C3D::Platform>(C3D::SystemType::PlatformSystemType)
#define UI2D m_pSystemsManager->GetSystem<C3D::UI2DSystem>(C3D::SystemType::UI2DSystemType)

namespace C3D
{
    enum SystemType
    {
        UI2DSystemType = 0,
        FontSystemType,
        LightSystemType,
        CameraSystemType,
        GeometrySystemType,
        MaterialSystemType,
        TextureSystemType,
        ShaderSystemType,
        RenderSystemType,
        ResourceSystemType,
        InputSystemType,
        EventSystemType,
        JobSystemType,
        CVarSystemType,
        PlatformSystemType,
        MaxKnownSystemType
    };

    class C3D_API SystemManager
    {
    public:
        SystemManager() = default;

        void OnInit();

        template <class System>
        bool RegisterSystem(const u16 systemType)
        {
            static_assert(std::is_base_of_v<BaseSystem, System>, "The provided system does not derive from the ISystem class.");

            if (systemType > MaxKnownSystemType)
            {
                INSTANCE_ERROR_LOG("SYSTEM_MANAGER", "The provided systemType should be 0 <= {} < {}.", systemType,
                                   ToUnderlying(MaxKnownSystemType));
                return false;
            }

            auto s = m_allocator.New<System>(MemoryType::CoreSystem, this);
            if (!s->OnInit())
            {
                INSTANCE_FATAL_LOG("SYSTEM_MANAGER", "Failed to initialize system.");
                return false;
            }

            m_systems[systemType] = s;
            return true;
        }

        template <class System, typename Config>
        bool RegisterSystem(const u16 systemType, const Config& config)
        {
            static_assert(std::is_base_of_v<SystemWithConfig<Config>, System>,
                          "The provided system does not derive from the ISystem class.");

            if (systemType > MaxKnownSystemType)
            {
                INSTANCE_ERROR_LOG("SYSTEM_MANAGER", "The provided systemType should be 0 <= {} < {}.", systemType,
                                   ToUnderlying(MaxKnownSystemType));
                return false;
            }

            auto s = m_allocator.New<System>(MemoryType::CoreSystem, this);
            if (!s->OnInit(config))
            {
                INSTANCE_FATAL_LOG("SYSTEM_MANAGER", "Failed to initialize system.");
                return false;
            }

            m_systems[systemType] = s;
            return true;
        }

        template <class SystemType>
        [[nodiscard]] SystemType& GetSystem(const u16 type) const
        {
            return *reinterpret_cast<SystemType*>(m_systems[type]);
        }

        template <class SystemType>
        [[nodiscard]] SystemType* GetSystemPtr(const u16 type) const
        {
            return reinterpret_cast<SystemType*>(m_systems[type]);
        }

        void OnShutdown();

    private:
        Array<ISystem*, MaxKnownSystemType> m_systems = {};

        LinearAllocator m_allocator;
    };
}  // namespace C3D
