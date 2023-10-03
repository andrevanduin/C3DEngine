
#pragma once
#include "resource_loader.h"
#include "resources/terrain/terrain_config.h"

namespace C3D
{
    template <>
    class C3D_API ResourceLoader<TerrainConfig> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, TerrainConfig& resource) const;
        static void Unload(TerrainConfig& resource);
    };
}  // namespace C3D