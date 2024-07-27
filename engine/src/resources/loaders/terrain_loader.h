
#pragma once
#include "resource_loader.h"
#include "resources/terrain/terrain_config.h"

namespace C3D
{
    template <>
    class C3D_API ResourceLoader<TerrainConfig> final : public IResourceLoader
    {
    public:
        ResourceLoader();

        bool Load(const char* name, TerrainConfig& resource) const;
        void Unload(TerrainConfig& resource) const;
    };
}  // namespace C3D