
#pragma once
#include "resource_loader.h"
#include "containers/dynamic_array.h"
#include "renderer/vertex.h"
#include "systems/geometry_system.h"

#include "platform/filesystem.h"

namespace C3D
{
	class File;
	constexpr auto MESH_LOADER_EXTENSION_COUNT = 2;

	template<typename, typename>
	struct GeometryConfig;

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

	struct MeshResource : Resource
	{
		DynamicArray<GeometryConfig<Vertex3D, u32>> geometryConfigs;
	};

	template <>
	class ResourceLoader<MeshResource> final : public IResourceLoader
	{
	public:
		ResourceLoader();

		bool Load(const char* name, MeshResource& resource) const;
		static void Unload(MeshResource& resource);

	private:
		bool ImportObjFile(File& file, const String& outCsmFileName, DynamicArray<GeometryConfig<Vertex3D, u32>>& outGeometries) const;
		void ObjParseVertexLine(const String& line, DynamicArray<vec3>& positions, DynamicArray<vec3>& normals, DynamicArray<vec2>& texCoords) const;
		static void ObjParseFaceLine(const String& line, u64 normalCount, u64 texCoordinateCount, DynamicArray<MeshGroupData>& groups);

		void ProcessSubObject(DynamicArray<vec3>& positions, DynamicArray<vec3>& normals, DynamicArray<vec2>& texCoords, DynamicArray<MeshFaceData>& faces, GeometryConfig<Vertex3D, u32>* outData) const;

		bool ImportObjMaterialLibraryFile(const char* mtlFilePath) const;
		void ObjMaterialParseColorLine(const String& line, MaterialConfig& config) const;
		void ObjMaterialParseMapLine(const String& line, MaterialConfig& config) const;
		void ObjMaterialParseNewMtlLine(const String& line, MaterialConfig& config, bool& hitName, const char* mtlFilePath) const;

		template<typename VertexType, typename IndexType>
		bool LoadCsmFile(File& file, DynamicArray<GeometryConfig<VertexType, IndexType>>& outGeometries) const;

		template<typename VertexType, typename IndexType>
		bool WriteCsmFile(const String& path, const char* name, DynamicArray<GeometryConfig<VertexType, IndexType>>& geometries) const;

		bool WriteMtFile(const char* mtlFilePath, MaterialConfig* config) const;
	};

	template <typename VertexType, typename IndexType>
	bool ResourceLoader<MeshResource>::LoadCsmFile(File& file, DynamicArray<GeometryConfig<VertexType, IndexType>>& outGeometries) const
	{
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

		// For Each geometry
		for (u64 i = 0; i < geometryCount; i++)
		{
			GeometryConfig<VertexType, IndexType> g = {};

			// Vertices (size / count / array)
			u64 vertexSize = 0;
			u64 vertexCount = 0;

			file.Read(&vertexSize);
			file.Read(&vertexCount);
			// Resize so we have enough space and our count is correct
			g.vertices.Resize(vertexCount); 
			file.Read(g.vertices.GetData(), vertexCount);

			// Indices (size / count / array)
			u64 indexSize = 0;
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

		m_logger.Info("ReadCsmFile() - {} Bytes read from file {}", file.bytesRead, file.currentPath);
		file.Close();
		return true;
	}

	template <typename VertexType, typename IndexType>
	bool ResourceLoader<MeshResource>::WriteCsmFile(const String& path, const char* name, DynamicArray<GeometryConfig<VertexType, IndexType>>& geometries) const
	{
		if (File::Exists(path))
		{
			m_logger.Info("WriteCsmFile() - File: {} already exists and will be overwritten", path);
		}

		File file;
		if (!file.Open(path, FileModeWrite | FileModeBinary))
		{
			m_logger.Error("WriteCsmFile() - Failed to open path {}", path);
			return false;
		}

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
			constexpr u64 vertexSize = GeometryConfig<VertexType, IndexType>::GetVertexSize();
			const u64 vertexCount = geometry.vertices.Size();

			file.Write(&vertexSize);
			file.Write(&vertexCount);
			file.Write(geometry.vertices.GetData(), vertexCount);

			// Indices (size / count / array)
			constexpr u64 indexSize = GeometryConfig<VertexType, IndexType>::GetIndexSize();
			const u64 indexCount = geometry.indices.Size();

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

		m_logger.Info("WriteCsmFile() - {} Bytes written to file {}", file.bytesWritten, path);
		file.Close();
		return true;
	}
}
