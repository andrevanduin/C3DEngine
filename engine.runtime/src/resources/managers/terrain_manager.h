
#pragma once
#include "resource_manager.h"
#include "resources/terrain/terrain_config.h"

namespace C3D
{
    template <>
    class C3D_API ResourceManager<TerrainConfig> final : public IResourceManager
    {
    public:
        ResourceManager();

        bool Read(const String& name, TerrainConfig& resource) const;
        void Cleanup(TerrainConfig& resource) const;
    };
}  // namespace C3D