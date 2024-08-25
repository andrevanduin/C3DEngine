
#include "terrain_manager.h"

#include "core/colors.h"
#include "core/exceptions.h"
#include "platform/file_system.h"
#include "resources/managers/image_manager.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr auto FILE_EXTENSION = "cterrain";

    ResourceManager<TerrainConfig>::ResourceManager() : IResourceManager(MemoryType::Terrain, ResourceType::Terrain, nullptr, "terrains") {}

    bool ResourceManager<TerrainConfig>::Read(const String& name, TerrainConfig& resource) const
    {
        if (name.Empty())
        {
            ERROR_LOG("No valid name was provided.");
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
        // Default chunk size to 16 if it's not provided
        resource.chunkSize = 16;

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
                else if (name.IEquals("chunkSize"))
                {
                    resource.chunkSize = value.ToU32();
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
            if (!Resources.Read(heightmapFile, heightmap, params))
            {
                WARN_LOG("Failed to load HeightmapFile: '{}' for Terrain: '{}'. Setting defaults.", heightmapFile, name);

                resource.tileCountX = 128;
                resource.tileCountZ = 128;
                resource.chunkSize  = 16;
                resource.vertexConfigs.Resize(resource.tileCountX * resource.tileCountZ);
            }
            else
            {
                // Read the pixels from the heightmap

                // Total number of pixels is with * height but we add 1 to width and height to compensate for the terrain chunks having 1
                // row/col overlap which results in the last row needing to be duplicated
                auto totalPixelCount = (heightmap.width + 1) * (heightmap.height + 1);
                resource.vertexConfigs.Resize(totalPixelCount);
                resource.tileCountX = heightmap.width;
                resource.tileCountZ = heightmap.height;

                assert(heightmap.channelCount == 4);

                if (resource.tileCountX % resource.chunkSize != 0 || resource.tileCountZ % resource.chunkSize)
                {
                    ERROR_LOG(
                        "The heightmap dimensions must be a multiple of the chunksize. The heightmap dimensions are: {}x{} and the "
                        "chunksize is: {}.",
                        resource.tileCountX, resource.tileCountZ, resource.chunkSize);
                    return false;
                }

                u32 j = 0;
                for (u32 y = 0, i = 0; y < heightmap.height; ++y)
                {
                    for (u32 x = 0; x < heightmap.width; ++x, ++i)
                    {
                        u8 r = heightmap.pixels[(i * 4) + 0];
                        u8 g = heightmap.pixels[(i * 4) + 1];
                        u8 b = heightmap.pixels[(i * 4) + 2];

                        // Convert RGB to a u32 and divide by the max value it can be to get a range between 0 an 1
                        resource.vertexConfigs[j].height = static_cast<f32>(RgbToU32(r, g, b)) / 16777215;
                        j++;
                    }

                    // Use the previous pixel's height again for the last row
                    resource.vertexConfigs[j].height = resource.vertexConfigs[j - 1].height;
                    j++;
                }

                // Iterate the last row of the iamge and sample the height from there again for the last row of the terrain
                for (u32 i = heightmap.width * (heightmap.height - 1); i < (heightmap.width * heightmap.height); ++i)
                {
                    u8 r = heightmap.pixels[(i * 4) + 0];
                    u8 g = heightmap.pixels[(i * 4) + 1];
                    u8 b = heightmap.pixels[(i * 4) + 2];

                    // Convert RGB to a u32 and divide by the max value it can be to get a range between 0 an 1
                    resource.vertexConfigs[j].height = static_cast<f32>(RgbToU32(r, g, b)) / 16777215;
                    j++;
                }
                // The final vertex also needs a copy of the previous height
                resource.vertexConfigs[j].height = resource.vertexConfigs[j - 1].height;

                Resources.Cleanup(heightmap);
            }
        }

        return true;
    }

    void ResourceManager<TerrainConfig>::Cleanup(TerrainConfig& resource) const
    {
        resource.name.Destroy();
        resource.resourceName.Destroy();
        resource.materials.Destroy();
        resource.vertexConfigs.Destroy();
    }
}  // namespace C3D
