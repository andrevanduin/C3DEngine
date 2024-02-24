
#include "terrain_loader.h"

#include "core/exceptions.h"
#include "platform/file_system.h"
#include "resources/loaders/image_loader.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "TERRAIN_LOADER";
    constexpr auto FILE_EXTENSION       = "cterrain";

    ResourceLoader<TerrainConfig>::ResourceLoader(const SystemManager* pSystemsManager)
        : IResourceLoader(pSystemsManager, MemoryType::Terrain, ResourceType::Terrain, nullptr, "terrains")
    {}

    bool ResourceLoader<TerrainConfig>::Load(const char* name, TerrainConfig& resource) const
    {
        if (std::strlen(name) == 0)
        {
            ERROR_LOG("Failed because provided name is empty.");
            return false;
        }

        // TODO: binary format
        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, FILE_EXTENSION);
        auto fileName = String::FromFormat("{}.{}", name, FILE_EXTENSION);

        File file;
        if (!file.Open(fullPath, FileModeRead))
        {
            ERROR_LOG("Failed to open Terrain config file for reading: '{}'.", fullPath);
            return false;
        }

        // Copy the path to the file
        resource.resourceName = fullPath;
        // Copy the name of the resource
        resource.name = name;

        String heightmapFile;
        String line;
        int lineNumber = 1, version = 0;

        try
        {
            while (file.ReadLine(line))
            {
                // Trim the line
                line.Trim();

                // Skip Blank lines and comments
                if (line.Empty() || line.First() == '#')
                {
                    lineNumber++;
                    continue;
                }

                auto parts = line.Split('=');
                if (parts.Size() != 2)
                {
                    throw Exception("Incorrect number of '='");
                }

                auto name  = parts[0];
                auto value = parts[1];

                if (version == 0)
                {
                    if (name.IEquals("version"))
                    {
                        version = value.ToU32();
                    }
                    else
                    {
                        throw Exception("Terrain config should start with version = <parser version>");
                    }
                }
                else if (name.IEquals("heightmapFile"))
                {
                    heightmapFile = value;
                }
                else if (name.IEquals("tileScaleX"))
                {
                    resource.tileScaleX = value.ToF32();
                }
                else if (name.IEquals("tileScaleZ"))
                {
                    resource.tileScaleZ = value.ToF32();
                }
                else if (name.IEquals("tileScaleY"))
                {
                    resource.tileScaleY = value.ToF32();
                }
                else if (name.IEquals("material"))
                {
                    if (resource.materials.Size() >= TERRAIN_MAX_MATERIAL_COUNT)
                    {
                        throw Exception("Maximum amount of materials exceeded must be <= {}.", TERRAIN_MAX_MATERIAL_COUNT);
                    }

                    resource.materials.PushBack(value);
                }
                else
                {
                    throw Exception("Unknown tag found: '{}'", name);
                }

                lineNumber++;
            }
        }
        catch (const std::exception& exc)
        {
            ERROR_LOG("Failed to load file: '{}'. {} on line: '{}'.", fullPath, exc.what(), lineNumber);
        }

        if (!file.Close())
        {
            ERROR_LOG("Failed to close Terrain config file: '{}'.", fullPath);
            return false;
        }

        // Load the heightmap file if one has been configured
        if (!heightmapFile.Empty())
        {
            Image heightmap;
            ImageLoadParams params = { false };
            if (!Resources.Load(heightmapFile, heightmap, params))
            {
                ERROR_LOG("Failed to load HeightmapFile: '{}' for Terrain: '{}'. Setting defaults.", heightmapFile, name);

                resource.tileCountX = 100;
                resource.tileCountZ = 100;
                resource.vertexConfigs.Resize(100 * 100);
            }
            else
            {
                // Read the pixels from the heightmap
                auto totalPixelCount = heightmap.width * heightmap.height;

                resource.vertexConfigs.Reserve(totalPixelCount);
                resource.tileCountX = heightmap.width;
                resource.tileCountZ = heightmap.height;

                assert(heightmap.channelCount == 4);

                for (u32 i = 0; i < totalPixelCount * heightmap.channelCount; i += heightmap.channelCount)
                {
                    u8 r = heightmap.pixels[i + 0];
                    u8 g = heightmap.pixels[i + 1];
                    u8 b = heightmap.pixels[i + 2];

                    f32 height = (((r + g + b) / 3.0f) / 255.0f);
                    resource.vertexConfigs.EmplaceBack(height);
                }

                Resources.Unload(heightmap);
            }
        }

        return true;
    }

    void ResourceLoader<TerrainConfig>::Unload(TerrainConfig& resource) const
    {
        resource.name.Destroy();
        resource.resourceName.Destroy();
        resource.materials.Destroy();
        resource.vertexConfigs.Destroy();
    }
}  // namespace C3D
