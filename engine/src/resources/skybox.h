
#pragma once
#include "core/defines.h"
#include "geometry_config.h"
#include "renderer/vertex.h"
#include "resources/textures/texture_map.h"

namespace C3D
{
    class SystemManager;
    struct Geometry;

    struct SkyboxConfig
    {
        String cubeMapName;
    };

    class C3D_API Skybox
    {
    public:
        Skybox();

        bool Create(const SystemManager* pSystemsManager, const SkyboxConfig& config);

        bool Initialize();

        bool Load();

        bool Unload();

        void Destroy();

        TextureMap cubeMap;
        Geometry* g = nullptr;

        u32 instanceId = INVALID_ID;
        /* @brief Synced to the renderer's current frame number
         * when the material has been applied for that frame. */
        u64 frameNumber = INVALID_ID_U64;

    private:
        LoggerInstance<16> m_logger;

        SkyboxConfig m_config;
        GeometryConfig m_skyboxGeometryConfig;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D