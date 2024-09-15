
#pragma once
#include "defines.h"
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
#define UI2D C3D::SystemManager::GetSystem<C3D::UI2DSystem>(C3D::SystemType::UI2DSystemType)
#define Audio C3D::SystemManager::GetSystem<C3D::AudioSystem>(C3D::SystemType::AudioSystemType)
#define Transforms C3D::SystemManager::GetSystem<C3D::TransformSystem>(C3D::SystemType::TransformSystemType)

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
        TransformSystemType,
        MaxKnownSystemType
    };

    namespace SystemManager
    {
        C3D_API bool OnInit();

        C3D_API void RegisterSystem(const u16 type, ISystem* system);

        C3D_API LinearAllocator& GetAllocator();
        C3D_API C3D_INLINE ISystem* GetSystem(u16 type);

        C3D_API bool OnPrepareRender(FrameData& frameData);

        C3D_API void OnShutdown();

        template <class System>
        bool RegisterSystem(const u16 systemType)
        {
            static_assert(std::is_base_of_v<BaseSystem, System>, "The provided system does not derive from the ISystem class.");

            if (systemType > MaxKnownSystemType)
            {
                ERROR_LOG("The provided systemType should be 0 <= {} < {}.", systemType, ToUnderlying(MaxKnownSystemType));
                return false;
            }

            auto& allocator = GetAllocator();
            auto system     = allocator.New<System>(MemoryType::CoreSystem);
            if (!system->OnInit())
            {
                FATAL_LOG("Failed to initialize system.");
                return false;
            }

            RegisterSystem(systemType, system);
            return true;
        }

        template <class System>
        bool RegisterSystem(const u16 systemType, const CSONObject& config)
        {
            static_assert(std::is_base_of_v<ISystem, System>, "The provided system does not derive from the ISystem class.");

            if (systemType > MaxKnownSystemType)
            {
                ERROR_LOG("The provided systemType should be 0 <= {} < {}.", systemType, ToUnderlying(MaxKnownSystemType));
                return false;
            }

            auto& allocator = GetAllocator();
            auto system     = allocator.New<System>(MemoryType::CoreSystem);
            if (!system->OnInit(config))
            {
                FATAL_LOG("Failed to initialize system.");
                return false;
            }

            RegisterSystem(systemType, system);
            return true;
        }

        template <class SystemType>
        inline SystemType& GetSystem(const u16 type)
        {
            return *reinterpret_cast<SystemType*>(GetSystem(type));
        }

    };  // namespace SystemManager
}  // namespace C3D