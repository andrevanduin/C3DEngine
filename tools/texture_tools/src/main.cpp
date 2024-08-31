
#include <defines.h>
#include <logger/logger.h>
#include <string/string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void PrintHelp()
{
    C3D::Logger::Info(
        "C3DEngine Texture Tools, Copyright 2022-2024 Cesar Pulles\n"
        "usage: TextureTools <mode> [arguments...]\n"
        "Modes: "
        " combineMaps | cMaps\n"
        "  Description:\n"
        "   Combines metallic, roughness and ao texture into a single 'combined' texture.\n"
        "   Uses one channel per texture: R = metallic, G = roughness and B = ao.\n"
        "   The outFile argument and one of the maps is required the other maps are optional.\n"
        "   The order you provide the maps in does not matter, and the maps you don't provide will get a default value assigned .\n"
        "  Usage:\n"
        "   combineMaps outFile=<fileName> metallic=<fileName> roughness=<fileName> ao=<fileName>");
}

enum MapType
{
    Metallic,
    Roughness,
    Ao,
    Max
};

struct ChannelMap
{
    C3D::String filePath;
    i32 width          = 0;
    i32 height         = 0;
    i32 channelsInFile = 0;
    u8* data           = nullptr;
};

void Cleanup(ChannelMap* maps)
{
    for (u32 i = 0; i < MapType::Max; i++)
    {
        if (maps[i].data)
        {
            stbi_image_free(maps[i].data);
        }
    }
}

i32 CombineTextureMaps(i32 argc, char** argv)
{
    if (argc < 3)
    {
        C3D::Logger::Info("combineMaps requires at least and outFile argument.");
        PrintHelp();
        return -3;
    }

    stbi_set_flip_vertically_on_load(true);

    ChannelMap maps[MapType::Max];
    C3D::String outFilePath;

    for (u32 i = 2; i < argc; i++)
    {
        C3D::String arg = argv[i];
        auto parts      = arg.Split('=');
        if (parts.Size() != 2)
        {
            C3D::Logger::Error("Invalid argument provided: '{}'.", arg);
            PrintHelp();
            return -4;
        }

        C3D::String name = parts[0];
        C3D::String path = parts[1];

        if (name.IEquals("outfile"))
        {
            outFilePath = path;
        }
        else if (name.IEquals("metallic"))
        {
            maps[MapType::Metallic].filePath = path;
        }
        else if (name.IEquals("roughness"))
        {
            maps[MapType::Roughness].filePath = path;
        }
        else if (name.IEquals("ao"))
        {
            maps[MapType::Ao].filePath = path;
        }
        else
        {
            C3D::Logger::Error("Unknown argument provided: '{}'.", arg);
            return -5;
        }
    }

    if (outFilePath.Empty())
    {
        C3D::Logger::Error("No outFile provided.");
        PrintHelp();
        return -6;
    }

    for (u32 i = 0; i < MapType::Max; ++i)
    {
        if (!maps[i].filePath)
        {
            // Skip maps with an empty path
            continue;
        }

        // Load the image data.
        const i32 channelsRequired = 4;
        maps[i].data = stbi_load(maps[i].filePath.Data(), &maps[i].width, &maps[i].height, &maps[i].channelsInFile, channelsRequired);
        if (!maps[i].data)
        {
            C3D::Logger::Error("Failed to load file: '{}'.", maps[i].filePath);
            Cleanup(maps);
            return -6;
        }
    }

    i32 width  = -1;
    i32 height = -1;

    for (u32 i = 0; i < MapType::Max; i++)
    {
        if (maps[i].filePath)
        {
            if (width == -1 || height == -1)
            {
                // First time we find dimensions lets save them off
                width  = maps[i].width;
                height = maps[i].height;
            }
            else
            {
                // Ensure that other dimensions are the same as previous maps
                if (maps[i].width != width || maps[i].height != height)
                {
                    C3D::Logger::Error("Not all texture maps have the same dimensions.");
                    Cleanup(maps);
                    return -7;
                }
            }
        }
    }

    if (width == -1 || height == -1)
    {
        C3D::Logger::Error("Unable to obtain width or height. Did you provide any valid textures?");
        Cleanup(maps);
        return -8;
    }

    C3D::Logger::Info("Successfully processed all maps.");

    const auto size = width * height * 4;

    for (u32 i = 0; i < MapType::Max; i++)
    {
        auto& map = maps[i];

        if (maps[i].filePath.Empty())
        {
            // We did not load any file for this map so let's add some defaults
            map.data = Memory.Allocate<u8>(C3D::MemoryType::Texture, size);

            if (i == MapType::Ao)
            {
                // Default is a plain white texture
                // We can set everything to 255 since we only care about the B channel (we ignore others)
                std::memset(map.data, 255, size);
            }
            else if (i == MapType::Roughness)
            {
                // Default is a medium gray
                // We can set everything to 128 since we only care about the G channel (we ignore others)
                std::memset(map.data, 128, size);
            }
            else if (i == MapType::Metallic)
            {
                // Default is a plain black texture
                // We can set everything to 0 since we only care about the R channel (we ignore others)
                std::memset(map.data, 0, size);
            }
        }
    }

    C3D::Logger::Info("Generated default maps for any that were not present.");

    // Combine the maps
    u8* targetBuffer = Memory.Allocate<u8>(C3D::MemoryType::Texture, size);
    for (u64 row = 0; row < width; ++row)
    {
        for (u64 col = 0; col < height; ++col)
        {
            u64 index    = (row * width) + col;
            u64 indexBpp = index * 4;
            // Set R, G and B to our input maps data
            targetBuffer[indexBpp + 0] = maps[MapType::Metallic].data[indexBpp + 0];
            targetBuffer[indexBpp + 1] = maps[MapType::Roughness].data[indexBpp + 1];
            targetBuffer[indexBpp + 2] = maps[MapType::Ao].data[indexBpp + 2];
            targetBuffer[indexBpp + 3] = 255;  // reserved
        }
    }

    C3D::Logger::Info("Combined all maps into a single buffer.");

    if (!stbi_write_png(outFilePath.Data(), width, height, 4, targetBuffer, 4 * width))
    {
        C3D::Logger::Error("Error writing the output image to: '{}'.", outFilePath);
        Memory.Free(targetBuffer);
        Cleanup(maps);
        return -9;
    }

    C3D::Logger::Info("Successfully written generated image to file.");

    Memory.Free(targetBuffer);
    Cleanup(maps);

    C3D::Logger::Info("Cleaned up memory.");

    return 0;
}

int main(int argc, char** argv)
{
    C3D::Logger::Init();
    Metrics.Init();
    C3D::GlobalMemorySystem::Init({ MebiBytes(128) });

    if (argc < 2)
    {
        C3D::Logger::Info("TextureTools requires at least one argument.");
        PrintHelp();
        return -1;
    }

    C3D::String arg1 = argv[1];

    if (arg1.IEquals("combineMaps") || arg1.IEquals("cMaps"))
    {
        return CombineTextureMaps(argc, argv);
    }
    else
    {
        C3D::Logger::Info("Unknown argument provided: {}.", arg1);
        PrintHelp();
        return -2;
    }

    return 0;
}