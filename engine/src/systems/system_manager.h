
#pragma once
#include <type_traits>

#include "core/defines.h"
#include "memory/allocators/linear_allocator.h"
#include "system.h"

#define Input C3D::SystemManager::GetSystem<C3D::InputSystem>(C3D::SystemType::InputSystemType)
#define Event C3D::SystemManager::GetSystem<C3D::EventSystem>(C3D::SystemType::EventSystemType)
#define Renderer C3D::SystemManager::GetSystem<C3D::RenderSystem>(C3D::SystemType::RenderSystemType)
#define Textures C3D::SystemManager::GetSystem<C3D::TextureSystem>(C3D::SystemType::TextureSystemType)
#define Materials C3D::SystemManager::GetSystem<C3D::MaterialSystem>(C3D::SystemType::MaterialSystemType)
#define Geometric C3D::SystemManager::GetSystem<C3D::GeometrySystem>(C3D::SystemType::GeometrySystemType)
#define Resources C3D::SystemManager::GetSystem<C3D::ResourceSystem>(C3D::SystemType::ResourceSystemType)
#define Shaders C3D::SystemManager::GetSystem<C3D::ShaderSystem>(C3D::SystemType::ShaderSystemType)
#define Lights C3D::SystemManager::GetSystem<C3D::LightSystem>(C3D::SystemType::LightSystemType)
#define Cam C3D::SystemManager::GetSystem<C3D::CameraSystem>(C3D::SystemType::CameraSystemType)
#define Jobs C3D::SystemManager::GetSystem<C3D::JobSystem>(C3D::SystemType::JobSystemType)
#define Fonts C3D::SystemManager::GetSystem<C3D::FontSystem>(C3D::SystemType::FontSystemType)
#define CVars C3D::SystemManager::GetSystem<C3D::CVarSystem>(C3D::SystemType::CVarSystemType)
#define OS C3D::SystemManager::GetSystem<C3D::Platform>(C3D::SystemType::PlatformSystemType)
#define UI2D C3D::SystemManager::GetSystem<C3D::UI2DSystem>(C3D::SystemType::UI2DSystemType)
#define Audio C3D::SystemManager::GetSystem<C3D::AudioSystem>(C3D::SystemType::AudioSystemType)

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
        AudioSystemType,
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
                ERROR_LOG("The provided systemType should be 0 <= {} < {}.", systemType, ToUnderlying(MaxKnownSystemType));
                return false;
            }

            auto s = m_allocator.New<System>(MemoryType::CoreSystem);
            if (!s->OnInit())
            {
                FATAL_LOG("Failed to initialize system.");
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
                ERROR_LOG("The provided systemType should be 0 <= {} < {}.", systemType, ToUnderlying(MaxKnownSystemType));
                return false;
            }

            auto s = m_allocator.New<System>(MemoryType::CoreSystem);
            if (!s->OnInit(config))
            {
                FATAL_LOG("Failed to initialize system.");
                return false;
            }

            m_systems[systemType] = s;
            return true;
        }

        template <class SystemType>
        static SystemType& GetSystem(const u16 type)
        {
            return *reinterpret_cast<SystemType*>(GetInstance().m_systems[type]);
        }

        template <class SystemType>
        static SystemType* GetSystemPtr(const u16 type)
        {
            return reinterpret_cast<SystemType*>(GetInstance().m_systems[type]);
        }

        bool OnPrepareRender(FrameData& frameData);

        void OnShutdown();

        static SystemManager& GetInstance();

    private:
        Array<ISystem*, MaxKnownSystemType> m_systems = {};
        LinearAllocator m_allocator;

        static SystemManager m_instance;
    };
}  // namespace C3D