
#pragma once
#include "core/defines.h"
#include "geometry.h"
#include "texture.h"

namespace C3D
{
    class SystemManager;

    class C3D_API Skybox
    {
    public:
        bool Create(const SystemManager* pSystemsManager, const char* cubeMapName);

        void Destroy();

        TextureMap cubeMap;
        Geometry* g = nullptr;

        u32 instanceId = INVALID_ID;
        /* @brief Synced to the renderer's current frame number
         * when the material has been applied for that frame. */
        u64 frameNumber = INVALID_ID_U64;

    private:
        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D