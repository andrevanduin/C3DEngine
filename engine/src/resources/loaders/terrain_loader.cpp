
#include "terrain_loader.h"

#include "platform/filesystem.h"
#include "resources/loaders/image_loader.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr auto FILE_EXTENSION = "cterrain";

    ResourceLoader<TerrainConfig>::ResourceLoader(const SystemManager* pSystemsManager)
        : IResourceLoader(pSystemsManager, "TERRAIN_LOADER", MemoryType::Terrain, ResourceType::Terrain, nullptr,
                          "terrains")
    {}

    bool ResourceLoader<TerrainConfig>::Load(const char* name, TerrainConfig& resource) const
    {
        if (std::strlen(name) == 0)
        {
            m_logger.Error("Load() - Failed because provided name is empty.");
            return false;
        }

        // TODO: binary format
        auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, FILE_EXTENSION);
        auto fileName = String::FromFormat("{}.{}", name, FILE_EXTENSION);

        File file;
        if (!file.Open(fullPath, FileModeRead))
        {
            m_logger.Error("Load() - Failed to open Terrain config file for reading: '{}'.", fullPath);
            return false;
        }

        // Copy the path to the file
        resource.resourceName = fullPath;
        // Copy the name of the resource
        resource.name = name;

        String heightmapFile;
        String line;
        int lineNumber = 1, version = 0;
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
                m_logger.Error("Load() - Failed to load file: '{}'. Incorrect number of '=' on line: '{}'", fullPath,
                               lineNumber);
                return false;
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
                    m_logger.Error(
                        "Load() - Failed to load file: '{}'. Terrain config should start with version = <parser "
                        "version>",
                        fullPath);
                    return false;
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
            else
            {
                m_logger.Error("Load - Failed to load file: '{}'. Unknown tag found: '{}' on line: '{}'", fullPath,
                               name, lineNumber);
                return false;
            }

            lineNumber++;
        }

        if (!file.Close())
        {
            m_logger.Error("Load() - Failed to close Terrain config file: '{}'", fullPath);
            return false;
        }

        // Load the heightmap file if one has been configured
        if (!heightmapFile.Empty())
        {
            ImageResource heightmap;
            ImageResourceParams params = { false };
            if (!Resources.Load(heightmapFile, heightmap, params))
            {
                m_logger.Error("Load() - Failed to load HeightmapFile: '{}' for Terrain: '{}'. Setting defaults.",
                               heightmapFile, name);

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
        resource.name.Clear();
        resource.resourceName.Clear();
        resource.materials.Clear();
        resource.vertexConfigs.Clear();
    }
}  // namespace C3D
