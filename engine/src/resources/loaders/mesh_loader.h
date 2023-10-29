
#pragma once
#include "containers/dynamic_array.h"
#include "core/clock.h"
#include "platform/file_system.h"
#include "renderer/vertex.h"
#include "resource_loader.h"
#include "resources/geometry_config.h"
#include "resources/materials/material_types.h"
#include "systems/system_manager.h"

namespace C3D
{
    class File;
    constexpr auto MESH_LOADER_EXTENSION_COUNT = 2;

    enum class MeshFileType
    {
        NotFound,
        Csm,
        Obj,
    };

    struct SupportedMeshFileType
    {
        const char* extension;
        MeshFileType type;
        bool isBinary;
    };

    struct MeshVertexIndexData
    {
        u32 positionIndex;
        u32 normalIndex;
        u32 texCoordinateIndex;
    };

    struct MeshFaceData
    {
        MeshVertexIndexData vertices[3];
    };

    struct MeshGroupData
    {
        DynamicArray<MeshFaceData> faces;
    };

    struct MeshResource final : Resource
    {
        DynamicArray<GeometryConfig> geometryConfigs;
    };

    struct UIMeshResource final : Resource
    {
        DynamicArray<UIGeometryConfig> geometryConfigs;
    };

    template <>
    class ResourceLoader<MeshResource> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, MeshResource& resource) const;
        void Unload(MeshResource& resource) const;

    private:
        bool ImportObjFile(File& file, const String& outCsmFileName, DynamicArray<GeometryConfig>& outGeometries) const;
        void ObjParseVertexLine(const String& line, DynamicArray<vec3>& positions, DynamicArray<vec3>& normals,
                                DynamicArray<vec2>& texCoords) const;
        static void ObjParseFaceLine(const String& line, u64 normalCount, u64 texCoordinateCount, DynamicArray<MeshGroupData>& groups);

        void ProcessSubObject(DynamicArray<vec3>& positions, DynamicArray<vec3>& normals, DynamicArray<vec2>& texCoords,
                              DynamicArray<MeshFaceData>& faces, GeometryConfig* outData) const;

        bool ImportObjMaterialLibraryFile(const String& mtlFilePath) const;
        void ObjMaterialParseColorLine(const String& line, MaterialConfig& config) const;
        void ObjMaterialParseMapLine(const String& line, MaterialConfig& config) const;
        void ObjMaterialParseNewMtlLine(const String& line, MaterialConfig& config, bool& hitName, const String& mtlFilePath) const;

        template <typename VertexType, typename IndexType>
        bool LoadCsmFile(File& file, DynamicArray<IGeometryConfig<VertexType, IndexType>>& outGeometries) const;

        template <typename VertexType, typename IndexType>
        bool WriteCsmFile(const String& path, const char* name, DynamicArray<IGeometryConfig<VertexType, IndexType>>& geometries) const;

        bool WriteMtFile(const String& mtlFilePath, const MaterialConfig& config) const;
    };

    template <typename VertexType, typename IndexType>
    bool ResourceLoader<MeshResource>::LoadCsmFile(File& file, DynamicArray<IGeometryConfig<VertexType, IndexType>>& outGeometries) const
    {
        Clock clock(m_pSystemsManager->GetSystemPtr<Platform>(PlatformSystemType));
        clock.Start();

        // Version
        u16 version = 0;
        file.Read(&version);

        // Name Length
        u64 nameLength = 0;
        file.Read(&nameLength);

        // Name + null terminator
        char name[256];
        file.Read(name, nameLength);

        // Geometry count
        u64 geometryCount = 0;
        file.Read(&geometryCount);

        // Reserve enough space for the geometries
        outGeometries.Reserve(geometryCount);
        // For Each geometry
        for (u64 i = 0; i < geometryCount; i++)
        {
            IGeometryConfig<VertexType, IndexType> g = {};

            // Vertices (size / count / array)
            u64 vertexSize  = 0;
            u64 vertexCount = 0;

            file.Read(&vertexSize);
            file.Read(&vertexCount);
            // Resize so we have enough space and our count is correct
            g.vertices.Resize(vertexCount);
            file.Read(g.vertices.GetData(), vertexCount);

            // Indices (size / count / array)
            u64 indexSize  = 0;
            u64 indexCount = 0;

            file.Read(&indexSize);
            file.Read(&indexCount);
            // Resize so we have enough space and our count is correct
            g.indices.Resize(indexCount);
            file.Read(g.indices.GetData(), indexCount);

            // Name
            file.Read(g.name);

            // Material Name
            file.Read(g.materialName);

            // Center
            file.Read(&g.center);

            // Extents (min / max)
            file.Read(&g.minExtents);
            file.Read(&g.maxExtents);

            outGeometries.PushBack(g);
        }

        clock.Update();
        INSTANCE_INFO_LOG("MESH_LOADER", "{} Bytes read from file {} in {:.4f} ms.", file.bytesRead, file.currentPath,
                          clock.GetElapsedMs());
        file.Close();
        return true;
    }

    template <typename VertexType, typename IndexType>
    bool ResourceLoader<MeshResource>::WriteCsmFile(const String& path, const char* name,
                                                    DynamicArray<IGeometryConfig<VertexType, IndexType>>& geometries) const
    {
        if (File::Exists(path))
        {
            INSTANCE_INFO_LOG("MESH_LOADER", "File: '{}' already exists and will be overwritten.", path);
        }

        File file;
        if (!file.Open(path, FileModeWrite | FileModeBinary))
        {
            INSTANCE_ERROR_LOG("MESH_LOADER", "Failed to open path '{}'.", path);
            return false;
        }

        INSTANCE_INFO_LOG("MESH_LOADER", "Started writing CSM file to: '{}'.", path);

        // Version
        constexpr u16 version = 0x0001u;
        file.Write(&version);

        // Name Length
        const u64 nameLength = std::strlen(name) + 1;
        file.Write(&nameLength);
        // Name + null terminator
        file.Write(name, nameLength);

        // Geometry count
        const u64 geometryCount = geometries.Size();
        file.Write(&geometryCount);

        for (auto& geometry : geometries)
        {
            // Vertices (size / count / array)
            constexpr u64 vertexSize = IGeometryConfig<VertexType, IndexType>::GetVertexSize();
            const u64 vertexCount    = geometry.vertices.Size();

            file.Write(&vertexSize);
            file.Write(&vertexCount);
            file.Write(geometry.vertices.GetData(), vertexCount);

            // Indices (size / count / array)
            constexpr u64 indexSize = IGeometryConfig<VertexType, IndexType>::GetIndexSize();
            const u64 indexCount    = geometry.indices.Size();

            file.Write(&indexSize);
            file.Write(&indexCount);
            file.Write(geometry.indices.GetData(), indexCount);

            // Name
            file.Write(geometry.name);

            // Material Name
            file.Write(geometry.materialName);

            // Center
            file.Write(&geometry.center);

            // Extents
            file.Write(&geometry.minExtents);
            file.Write(&geometry.maxExtents);
        }

        INSTANCE_INFO_LOG("MESH_LOADER", "{} Bytes written to file: '{}'.", file.bytesWritten, name);
        file.Close();
        return true;
    }
}  // namespace C3D
