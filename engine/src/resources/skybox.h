
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
        Skybox() = default;

        bool Create(const SystemManager* pSystemsManager, const SkyboxConfig& config);

        bool Initialize();

        bool Load();

        bool Unload();

        void Destroy();

        TextureMap cubeMap;
        Geometry* g = nullptr;

        u32 instanceId = INVALID_ID;

        /** @brief Synced to the renderer's current frame number when the material has been applied that frame. */
        u64 frameNumber = INVALID_ID_U64;
        /** @brief Synced to the renderer's current draw index when the material has been applied that frame.*/
        u8 drawIndex = INVALID_ID_U8;

    private:
        SkyboxConfig m_config;
        GeometryConfig m_skyboxGeometryConfig;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D