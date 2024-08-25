
#include "mesh_manager.h"

#include "core/exceptions.h"
#include "core/string_utils.h"
#include "math/geometry_utils.h"
#include "platform/file_system.h"
#include "renderer/vertex.h"
#include "systems/geometry/geometry_system.h"
#include "systems/resources/resource_system.h"

namespace C3D
{
    ResourceManager<MeshResource>::ResourceManager() : IResourceManager(MemoryType::Geometry, ResourceType::Mesh, nullptr, "models") {}

    bool ResourceManager<MeshResource>::Read(const String& name, MeshResource& resource) const
    {
        if (name.Empty())
        {
            ERROR_LOG("No valid name was provided.");
            return false;
        }

        String fullPath;
        auto type = MeshFileType::NotFound;
        File file;

        // Try different extensions. First try our optimized binary format, otherwise we try obj.
        SupportedMeshFileType supportedFileTypes[MESH_LOADER_EXTENSION_COUNT] = {
            { "csm", MeshFileType::Csm, true },
            { "obj", MeshFileType::Obj, false },
        };

        for (const auto fileType : supportedFileTypes)
        {
            fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, fileType.extension);

            // Check if the requested file exists with the current extension
            if (File::Exists(fullPath))
            {
                // File exists, let's try to open it
                u8 mode = FileModeRead;
                if (fileType.isBinary) mode |= FileModeBinary;

                if (file.Open(fullPath, mode))
                {
                    // We successfully opened the file
                    type = fileType.type;
                    break;
                }
            }
        }

        if (type == MeshFileType::NotFound)
        {
            // We could not find any file
            ERROR_LOG("Unable to find a mesh file of supported type called: '{}'.", name);
            return false;
        }

        // Copy the path to the file
        resource.fullPath = fullPath;
        // Copy the name of the resource
        resource.name = name;
        // The resource data is just a dynamic array of configs
        resource.geometryConfigs.Reserve(8);

        bool result = false;
        switch (type)
        {
            case MeshFileType::Obj:
                result = ImportObjFile(file, String::FromFormat("{}/{}/{}.csm", Resources.GetBasePath(), typePath, name),
                                       resource.geometryConfigs);
                break;
            case MeshFileType::Csm:
                result = LoadCsmFile(file, resource.geometryConfigs);
                break;
            case MeshFileType::NotFound:
                ERROR_LOG("Unsupported mesh type for file '{}'.", name);
                result = false;
                break;
        }

        file.Close();
        if (!result)
        {
            ERROR_LOG("Failed to process mesh file: '{}'.", fullPath);
            return false;
        }

        return true;
    }

    void ResourceManager<MeshResource>::Cleanup(MeshResource& resource) const
    {
        for (auto& config : resource.geometryConfigs)
        {
            Geometric.DisposeConfig(config);
        }

        resource.geometryConfigs.Destroy();
        resource.name.Destroy();
        resource.fullPath.Destroy();
    }

    bool ResourceManager<MeshResource>::ImportObjFile(File& file, const String& outCsmFileName,
                                                      DynamicArray<GeometryConfig>& outGeometries) const
    {
        // Allocate dynamic arrays with lots of space reserved for our data
        DynamicArray<vec3> positions(16384);
        DynamicArray<vec3> normals(16384);
        DynamicArray<vec2> texCoords(16384);
        DynamicArray<MeshGroupData> groups(4);

        char materialFileName[512] = {};
        char name[512]             = {};

        u8 currentMaterialNameCount = 0;
        char materialNames[32][64];

        String line(512);  // Reserve enough space so we don't have to reallocate this every time
        while (file.ReadLine(line))
        {
            // Skip blank lines
            if (line.Empty()) continue;

            switch (line[0])
            {
                case '#':  // Comment so we skip this line
                    continue;
                case 'v':  // Line starts with 'v' meaning it will contain vertex data
                    ObjParseVertexLine(line, positions, normals, texCoords);
                    break;
                case 's':
                    // Ignore for now
                    break;
                case 'f':
                    ObjParseFaceLine(line, normals.Size(), texCoords.Size(), groups);
                    break;
                case 'm':
                    // Material library file
                    char substr[7];
                    sscanf(line.Data(), "%s %s", substr, materialFileName);

                    // If found, save off the material file name
                    if (StringUtils::IEquals(substr, "mtllib", 6))
                    {
                        // TODO: verification
                    }
                    break;
                case 'u':
                {
                    // Anytime there is a usemtl, assume a new group
                    // New named group or smoothing group, all faces coming after should be added to it
                    MeshGroupData newGroup;
                    newGroup.faces.Reserve(16384);  // Ensure that there is enough space
                    groups.PushBack(newGroup);

                    char t[8];
                    sscanf(line.Data(), "%s %s", t, materialNames[currentMaterialNameCount]);
                    currentMaterialNameCount++;
                    break;
                }
                case 'o':
                {
                    // TODO: Improve this by alot :)
                    char t[3];
                    sscanf(line.Data(), "%s %s", t, &name);
                    break;
                }
                case 'g':
                    for (u64 i = 0; i < groups.Size(); i++)
                    {
                        GeometryConfig newData = {};
                        newData.name           = name;

                        if (i > 0)
                        {
                            newData.name += i;
                        }

                        newData.materialName = materialNames[i];
                        ProcessSubObject(positions, normals, texCoords, groups[i].faces, &newData);

                        outGeometries.PushBack(newData);
                        groups[i].faces.Destroy();
                        std::memset(materialNames[i], 0, 64);
                    }

                    currentMaterialNameCount = 0;
                    groups.Clear();
                    std::memset(name, 0, 512);

                    char t[2];
                    sscanf(line.Data(), "%s %s", t, name);
                    break;
                default:
                    WARN_LOG("Unknown character found: '{}' in line: '{}'.", line[0], line);
                    break;
            }
        }

        for (u64 i = 0; i < groups.Size(); i++)
        {
            GeometryConfig newData = {};
            newData.name           = name;

            if (i > 0)
            {
                newData.name += i;
            }
            newData.materialName = materialNames[i];

            ProcessSubObject(positions, normals, texCoords, groups[i].faces, &newData);

            outGeometries.PushBack(newData);
            groups[i].faces.Destroy();
        }

        if (std::strlen(materialFileName) > 0)
        {
            // Load up the material file
            String fullMtlPath = FileSystem::DirectoryFromPath(outCsmFileName.Data());
            fullMtlPath += materialFileName;

            if (!ImportObjMaterialLibraryFile(fullMtlPath))
            {
                ERROR_LOG("Error reading obj mtl file: '{}'.", fullMtlPath);
            }
        }

        // De-duplicate geometry
        for (auto& geometry : outGeometries)
        {
            INFO_LOG("Geometry de-duplication started on geometry object: '{}'.", geometry.name);
            GeometryUtils::DeduplicateVertices(geometry);
            GeometryUtils::GenerateTangents(geometry.vertices, geometry.indices);
        }

        return WriteCsmFile(outCsmFileName, name, outGeometries);
    }

    void ResourceManager<MeshResource>::ObjParseVertexLine(const String& line, DynamicArray<vec3>& positions, DynamicArray<vec3>& normals,
                                                           DynamicArray<vec2>& texCoords) const
    {
        switch (line[1])
        {
            case ' ':  // Only 'v' so this line contains position
            {
                vec3 pos;
                char tp[3];
                sscanf(line.Data(), "%s %f %f %f", tp, &pos.x, &pos.y, &pos.z);
                positions.PushBack(pos);
                break;
            }
            case 'n':  // 'vn' so this line contains normal
            {
                vec3 norm;
                char tn[3];
                sscanf(line.Data(), "%s %f %f %f", tn, &norm.x, &norm.y, &norm.z);
                normals.PushBack(norm);
                break;
            }
            case 't':  // 'vt' so this line contains texture coords
            {
                vec2 tex;
                char tx[3];
                sscanf(line.Data(), "%s %f %f", tx, &tex.x, &tex.y);
                texCoords.PushBack(tex);
                break;
            }
            default:
                WARN_LOG("Unexpected character after 'v' found: '{}'.", line[1]);
                break;
        }
    }

    void ResourceManager<MeshResource>::ObjParseFaceLine(const String& line, const u64 normalCount, const u64 texCoordinateCount,
                                                         DynamicArray<MeshGroupData>& groups)
    {
        MeshFaceData face{};
        char t[2];

        if (normalCount == 0 || texCoordinateCount == 0)
        {
            sscanf(line.Data(), "%s %d %d %d", t, &face.vertices[0].positionIndex, &face.vertices[1].positionIndex,
                   &face.vertices[2].positionIndex);
        }
        else
        {
            sscanf(line.Data(), "%s %d/%d/%d %d/%d/%d %d/%d/%d", t, &face.vertices[0].positionIndex, &face.vertices[0].texCoordinateIndex,
                   &face.vertices[0].normalIndex, &face.vertices[1].positionIndex, &face.vertices[1].texCoordinateIndex,
                   &face.vertices[1].normalIndex, &face.vertices[2].positionIndex, &face.vertices[2].texCoordinateIndex,
                   &face.vertices[2].normalIndex);
        }

        const u64 groupIndex = groups.Size() - 1;
        groups[groupIndex].faces.PushBack(face);
    }

    void ResourceManager<MeshResource>::ProcessSubObject(DynamicArray<vec3>& positions, DynamicArray<vec3>& normals,
                                                         DynamicArray<vec2>& texCoords, DynamicArray<MeshFaceData>& faces,
                                                         GeometryConfig* outData) const
    {
        auto indices  = DynamicArray<u32>(32768);
        auto vertices = DynamicArray<Vertex3D>(32768);

        bool extentSet      = false;
        outData->minExtents = vec3(0);
        outData->maxExtents = vec3(0);

        u64 faceCount          = faces.Size();
        u64 normalCount        = normals.Size();
        u64 texCoordinateCount = texCoords.Size();

        bool skipNormals            = false;
        bool skipTextureCoordinates = false;

        if (normalCount == 0)
        {
            WARN_LOG("No normals are present in this model.");
            skipNormals = true;
        }

        if (texCoordinateCount == 0)
        {
            WARN_LOG("No texture coordinates are present in this model.");
            skipTextureCoordinates = true;
        }

        for (u64 f = 0; f < faceCount; f++)
        {
            const auto& face = faces[f];

            // For each Vertex
            for (u64 i = 0; i < 3; i++)
            {
                auto& indexData = face.vertices[i];
                indices.PushBack(static_cast<u32>(i + f * 3));

                Vertex3D vertex{};

                vec3 pos        = positions[indexData.positionIndex - 1];
                vertex.position = pos;

                // Check extents - min
                if (pos.x < outData->minExtents.x || !extentSet)
                {
                    outData->minExtents.x = pos.x;
                }
                if (pos.y < outData->minExtents.y || !extentSet)
                {
                    outData->minExtents.y = pos.y;
                }
                if (pos.z < outData->minExtents.z || !extentSet)
                {
                    outData->minExtents.z = pos.z;
                }

                // Check extents - max
                if (pos.x > outData->maxExtents.x || !extentSet)
                {
                    outData->maxExtents.x = pos.x;
                }
                if (pos.y > outData->maxExtents.y || !extentSet)
                {
                    outData->maxExtents.y = pos.y;
                }
                if (pos.z > outData->maxExtents.z || !extentSet)
                {
                    outData->maxExtents.z = pos.z;
                }

                extentSet = true;

                if (skipNormals)
                {
                    vertex.normal = vec3(0, 0, 1);
                }
                else
                {
                    vertex.normal = normals[indexData.normalIndex - 1];
                }

                if (skipTextureCoordinates)
                {
                    vertex.texture = vec2(0, 0);
                }
                else
                {
                    vertex.texture = texCoords[indexData.texCoordinateIndex - 1];
                }

                // TODO: color. Currently hardcoded to white
                vertex.color = vec4(1);

                vertices.PushBack(vertex);
            }
        }

        // Calculate the center based on extents
        for (u8 i = 0; i < 3; i++)
        {
            outData->center[i] = (outData->minExtents[i] + outData->maxExtents[i]) / 2.0f;
        }

        outData->vertices = vertices;
        outData->indices  = indices;
    }

    bool ResourceManager<MeshResource>::ImportObjMaterialLibraryFile(const String& mtlFilePath) const
    {
        // Grab the .mtl file, if it exists, and read the material information.
        File mtlFile;
        if (!mtlFile.Open(mtlFilePath, FileModeRead))
        {
            ERROR_LOG("Unable to open .mtl file: '{}'.", mtlFilePath);
            return false;
        }

        MaterialConfig currentConfig;
        currentConfig.version = 2;
        currentConfig.type    = MaterialType::PBR;

        bool hitName = false;

        String line;
        line.Reserve(512);

        while (mtlFile.ReadLine(line))
        {
            line.Trim();
            if (line.Empty()) continue;  // Skip empty lines

            switch (line[0])
            {
                case '#':
                    // Skip comments
                    continue;
                case 'K':
                {
                    ObjMaterialParseColorLine(line, currentConfig);
                    break;
                }
                case 'N':
                    break;
                    {
                        // HACK: Disable in order not to break current PBR material state
                        if (line[1] == 's')
                        {
                            auto parts = line.Split(' ');
                            auto prop  = MaterialConfigProp("shininess", ShaderUniformType::Uniform_Float32, parts[1].ToF32());
                            if (std::get<f32>(prop.value) <= 0.0f)
                            {
                                // Set a minimal shininess value to reduce rendering artifacts
                                prop.value = 8.0f;
                            }
                            currentConfig.props.EmplaceBack(prop);
                        }
                    }
                case 'm':
                    ObjMaterialParseMapLine(line, currentConfig);
                    break;
                case 'b':
                {
                    auto parts = line.Split(' ');
                    if (parts[0].IEquals("bump"))
                    {
                        currentConfig.maps.EmplaceBack(MaterialConfigMap("normal", FileSystem::FileNameFromPath(parts[1])));
                    }
                    break;
                }
                case 'd':
                {
                    // Dissolved value which describes how transparent the material (range of 0.0 to 1.0)
                    // Here 0 means fully transparent and 1.0 would mean fully opaque. We ignore this property for now.
                    continue;
                }
                case 'i':
                {
                    // illumnation model which we ingore for now
                    continue;
                }
                case 'T':
                {
                    if (line[1] == 'r')
                    {
                        // Transparency Tr = 1 - d meaing 0 means fully opaque and 1 means fully transparent. Ignored for now.
                        continue;
                    }
                    else if (line[1] == 'f')
                    {
                        // Transmission filter color. Ignored for now.
                        continue;
                    }
                    else
                    {
                        ERROR_LOG("Unknown character after 'T': {} on line {}.", line[1], line);
                    }
                }
                case 'n':
                    ObjMaterialParseNewMtlLine(line, currentConfig, hitName, mtlFilePath);
                    break;
                default:
                    ERROR_LOG("Unknown starting character found: '{}' on line {}.", line[0], line);
                    break;
            }
        }

        if (!WriteMtFile(mtlFilePath, currentConfig))
        {
            ERROR_LOG("Unable to write .mt file: '{}'.", mtlFilePath);
            return false;
        }

        mtlFile.Close();
        return true;
    }

    void ResourceManager<MeshResource>::ObjMaterialParseColorLine(const String& line, MaterialConfig& config) const
    {
        switch (line[1])
        {
            case 'a':  // Ambient or diffuse color which we both treat the same
            case 'd':
            {
                auto parts = line.Split(' ');
                config.props.EmplaceBack(MaterialConfigProp("diffuseColor", ShaderUniformType::Uniform_Float32_4,
                                                            vec4(parts[1].ToF32(), parts[2].ToF32(), parts[3].ToF32(), 1.0f)));
                break;
            }
            case 's':  // Specular color
            case 'e':  // Emmisive color
            {
                char t[3];
                // NOTE: this is not used for now
                f32 ignore = 0.0f;
                sscanf(line.Data(), "%s %f %f %f", t, &ignore, &ignore, &ignore);
                break;
            }
            default:
                WARN_LOG("Unknown second character found: '{}' on line: '{}'.", line[1], line);
                break;
        }
    }

    void ResourceManager<MeshResource>::ObjMaterialParseMapLine(const String& line, MaterialConfig& config) const
    {
        auto parts = line.Split(' ');
        MaterialConfigMap map;

        if (parts[0].IEquals("map_Kd"))
        {
            // Diffuse/albedo texture map
            map.name = "albedo";
        }
        else if (parts[0].IEquals("map_Ks"))
        {
            // Specular texture map
            map.name = "specular";
        }
        else if (parts[0].IEquals("map_bump"))
        {
            // Normal texture map
            map.name = "normal";
        }
        else if (parts[0].IEquals("map_Pr"))
        {
            // TODO: This + metallioc and ao should be combined automatically
            map.name = "roughness";
        }
        else if (parts[0].IEquals("map_Pm"))
        {
            // TODO: This + roughness and ao should be combined automatically
            map.name = "metallic";
        }
        else if (parts[0].IEquals("map_Ka"))
        {
            // Ambient texture map, we skip this for now
            return;
        }
        else if (parts[0].IEquals("map_Ke"))
        {
            map.name = "emissive";
        }
        else if (parts[0].IEquals("map_d"))
        {
            // The alpha texture map, we skip this for now
            return;
        }
        else
        {
            throw Exception("Invalid map: '{}' found", parts[0]);
        }

        map.textureName = FileSystem::FileNameFromPath(parts[1]);
        config.maps.PushBack(map);
    }

    void ResourceManager<MeshResource>::ObjMaterialParseNewMtlLine(const String& line, MaterialConfig& config, bool& hitName,
                                                                   const String& mtlFilePath) const
    {
        auto parts = line.Split(' ');

        char substr[10];
        char materialName[512];

        sscanf(line.Data(), "%s %s", substr, materialName);
        if (StringUtils::IEquals(substr, "newmtl", 6))
        {
            // It's a material name
            if (hitName)
            {
                // Write out a mt file and move on.
                if (!WriteMtFile(mtlFilePath, config))
                {
                    ERROR_LOG("Unable to write mt file: '{}'.", mtlFilePath);
                    return;
                }

                // Empty out the config
                config         = {};
                config.version = 2;
            }

            hitName     = true;
            config.name = materialName;
        }
    }

    bool ResourceManager<MeshResource>::WriteMtFile(const String& mtlFilePath, const MaterialConfig& config) const
    {
        // NOTE: The .obj file is in the models directory we have to move up 1 directory and go into the materials
        // directory
        File file;
        String directory = FileSystem::DirectoryFromPath(mtlFilePath);

        auto fullPath = String::FromFormat("{}../materials/{}.{}", directory, config.name, "mt");
        if (!file.Open(fullPath, FileModeWrite))
        {
            ERROR_LOG("Failed to open material file for writing: '{}'.", fullPath);
            return false;
        }

        INFO_LOG("Started writing .mt file to: '{}'.", fullPath);

        CString<512> lineBuffer;
        file.WriteLine("#material file");
        file.WriteLine("");

        lineBuffer.FromFormat("version = {}", config.version);
        file.WriteLine(lineBuffer);

        lineBuffer.FromFormat("type = {}", ToString(config.type));
        file.WriteLine(lineBuffer);

        lineBuffer.FromFormat("name = {}", config.name);
        file.WriteLine(lineBuffer);

        if (!config.shaderName.Empty())
        {
            lineBuffer.FromFormat("shader = {}", config.shaderName);
            file.WriteLine(lineBuffer);
        }

        for (const auto& map : config.maps)
        {
            file.WriteLine("[map]");

            lineBuffer.FromFormat("name = {}", map.name);
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("filterMin = {}", ToString(map.minifyFilter));
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("filterMag = {}", ToString(map.magnifyFilter));
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("repeatU = {}", ToString(map.repeatU));
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("repeatV = {}", ToString(map.repeatV));
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("repeatW = {}", ToString(map.repeatW));
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("textureName = {}", map.textureName);
            file.WriteLine(lineBuffer);

            file.WriteLine("[/map]");
        }

        for (const auto& prop : config.props)
        {
            file.WriteLine("[prop]");

            lineBuffer.FromFormat("name = {}", prop.name);
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("type = {}", ToString(prop.type));
            file.WriteLine(lineBuffer);

            lineBuffer.FromFormat("value = {}", ToString(prop.value));
            file.WriteLine(lineBuffer);

            file.WriteLine("[/prop]");
        }

        file.Close();
        return true;
    }
}  // namespace C3D
