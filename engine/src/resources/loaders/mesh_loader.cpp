
#include "mesh_loader.h"

#include "math/geometry_utils.h"
#include "platform/filesystem.h"
#include "renderer/renderer_types.h"
#include "renderer/vertex.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"

namespace C3D
{
	ResourceLoader<MeshResource>::ResourceLoader()
		: IResourceLoader("MESH_LOADER", MemoryType::Geometry, ResourceType::Mesh, nullptr, "models")
	{}

	bool ResourceLoader<MeshResource>::Load(const char* name, MeshResource* outResource) const
	{
		if (StringLength(name) == 0 || !outResource) {
			m_logger.Error("Load() - Name was empty or outResource was nullptr");
			return false;
		}

		char fullPath[512];
		auto type = MeshFileType::NotFound;
		File file;

		// Try different extensions. First try our optimized binary format, otherwise we try obj.
		SupportedMeshFileType supportedFileTypes[MESH_LOADER_EXTENSION_COUNT] = 
		{
			{ "csm", MeshFileType::Csm, true },
			{ "obj", MeshFileType::Obj, false },
		};

		for (const auto fileType : supportedFileTypes)
		{
			constexpr auto formatStr = "%s/%s/%s.%s";
			StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name, fileType.extension);

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
			m_logger.Error("Load() - Unable to find a mesh file of supported type called: '{}'", name);
			return false;
		}

		// Duplicate the path to the file
		outResource->fullPath = StringDuplicate(fullPath);
		// Duplicate the name of the resource
		outResource->name = StringDuplicate(name);
		// The resource data is just a dynamic array of configs
		outResource->geometryConfigs.Reserve(8);

		bool result = false;
		switch (type)
		{
			case MeshFileType::Obj:
				// Generate CSM filename
				char csmFileName[512];
				StringFormat(csmFileName, "%s/%s/%s.csm", Resources.GetBasePath(), typePath, name);
				result = ImportObjFile(file, csmFileName, outResource->geometryConfigs);
				break;
			case MeshFileType::Csm:
				result = LoadCsmFile(file, outResource->geometryConfigs);
				break;
			case MeshFileType::NotFound:
				m_logger.Error("Load() - Unsupported mesh type for file '{}'", name);
				result = false;
				break;
		}

		file.Close();
		if (!result)
		{
			m_logger.Error("Load() - Failed to process mesh file: '{}'", fullPath);
			return false;
		}

		return true;
	}

	void ResourceLoader<MeshResource>::Unload(MeshResource* resource)
	{
		resource->geometryConfigs.Destroy();
	}

	bool ResourceLoader<MeshResource>::ImportObjFile(File& file, const char* outCsmFileName, DynamicArray<GeometryConfig<Vertex3D, u32>>& outGeometries) const
	{
		// Allocate dynamic arrays with lots of space reserved for our data
		DynamicArray<vec3> positions(16384);
		DynamicArray<vec3> normals(16384);
		DynamicArray<vec2> texCoords(16384);
		DynamicArray<MeshGroupData> groups(4);

		char materialFileName[512];

		char name[512];
		u8 currentMaterialNameCount = 0;
		char materialNames[32][64];

		string line;
		line.reserve(512); // Reserve enough space so we don't have to reallocate this every time
		while (file.ReadLine(line))
		{
			// Skip blank lines
			if (line.empty()) continue;

			switch (line[0])
			{
				case '#': // Comment so we skip this line
					continue;
				case 'v': // Line starts with 'v' meaning it will contain vertex data
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
					sscanf(line.data(), "%s %s", substr, materialFileName);

					// If found, save off the material file name
					if (IEquals(substr, "mtllib", 6))
					{
						// TODO: verification
					}
					break;
				case 'u':
				{
					// Anytime there is a usemtl, assume a new group
					// New named group or smoothing group, all faces coming after should be added to it
					MeshGroupData newGroup;
					newGroup.faces.Reserve(16384); // Ensure that there is enough space
					groups.PushBack(newGroup);

					char t[8];
					sscanf(line.data(), "%s %s", t, materialNames[currentMaterialNameCount]);
					currentMaterialNameCount++;
					break;
				}
				case 'o':
				{
					char t[3];
					sscanf(line.data(), "%s %s", t, &name);
					break;
				}
				case 'g':
					for (u64 i = 0; i < groups.Size(); i++)
					{
						GeometryConfig<Vertex3D, u32> newData{};
						StringNCopy(newData.name, name, 255);

						if (i > 0)
						{
							StringAppend(newData.name, newData.name, i);
						}
						StringNCopy(newData.materialName, materialNames[i], 255);

						ProcessSubObject(positions, normals, texCoords, groups[i].faces, &newData);

						outGeometries.PushBack(newData);
						groups[i].faces.Destroy();
						Memory.Zero(materialNames[i], 64);
					}

					currentMaterialNameCount = 0;
					groups.Clear();
					Memory.Zero(name, 512);

					char t[2];
					sscanf(line.data(), "%s %s", t, name);
					break;
				default:
					m_logger.Warn("ImportObjFile() - Unknown character found: '{}' in line: '{}'", line[0], line);
					break;
			}
		}

		for (u64 i = 0; i < groups.Size(); i++)
		{
			GeometryConfig<Vertex3D, u32> newData{};
			StringNCopy(newData.name, name, 255);

			if (i > 0)
			{
				StringAppend(newData.name, newData.name, i);
			}
			StringNCopy(newData.materialName, materialNames[i], 255);

			ProcessSubObject(positions, normals, texCoords, groups[i].faces, &newData);

			outGeometries.PushBack(newData);
			groups[i].faces.Destroy();
		}

		groups.Destroy();
		positions.Destroy();
		normals.Destroy();
		texCoords.Destroy();

		if (StringLength(materialFileName) > 0)
		{
			// Load up the material file
			char fullMtlPath[512];
			FileSystem::DirectoryFromPath(fullMtlPath, outCsmFileName);
			StringAppend(fullMtlPath, fullMtlPath, materialFileName);

			if (!ImportObjMaterialLibraryFile(fullMtlPath))
			{
				m_logger.Error("ImportObjFile() - Error reading obj mtl file: {}", fullMtlPath);
			}
		}

		// De-duplicate geometry
		for (auto& geometry : outGeometries)
		{
			m_logger.Debug("Geometry de-duplication started on geometry object: '{}'", geometry.name);
			GeometryUtils::DeduplicateVertices(geometry);
			GeometryUtils::GenerateTangents(geometry.vertices.GetData(), geometry.indices.Size(), geometry.indices.GetData());
		}

		return WriteCsmFile(outCsmFileName, name, outGeometries);
	}

	void ResourceLoader<MeshResource>::ObjParseVertexLine(const string& line, DynamicArray<vec3>& positions, DynamicArray<vec3>& normals, DynamicArray<vec2>& texCoords) const
	{
		switch (line[1])
		{
			case ' ': // Only 'v' so this line contains position
			{
				vec3 pos;
				char tp[3];
				sscanf(line.data(), "%s %f %f %f", tp, &pos.x, &pos.y, &pos.z);
				positions.PushBack(pos);
				break;
			}
			case 'n': // 'vn' so this line contains normal
			{
				vec3 norm;
				char tn[3];
				sscanf(line.data(), "%s %f %f %f", tn, &norm.x, &norm.y, &norm.z);
				normals.PushBack(norm);
				break;
			}
			case 't': // 'vt' so this line contains texture coords
			{
				vec2 tex;
				char tx[3];
				sscanf(line.data(), "%s %f %f", tx, &tex.x, &tex.y);
				texCoords.PushBack(tex);
				break;
			}
			default:
				m_logger.Warn("ObjParseVertexLine() - Unexpected character after 'v' found: '{}'", line[1]);
				break;
		}
	}

	void ResourceLoader<MeshResource>::ObjParseFaceLine(const string& line, const u64 normalCount, const u64 texCoordinateCount, DynamicArray<MeshGroupData>& groups)
	{
		MeshFaceData face{};
		char t[2];

		if (normalCount == 0 || texCoordinateCount == 0)
		{
			sscanf(line.data(), "%s %d %d %d", t, 
				&face.vertices[0].positionIndex, &face.vertices[1].positionIndex, &face.vertices[2].positionIndex);
		}
		else
		{
			sscanf(line.data(), "%s %d/%d/%d %d/%d/%d %d/%d/%d", t,
				&face.vertices[0].positionIndex, &face.vertices[0].texCoordinateIndex, &face.vertices[0].normalIndex,
				&face.vertices[1].positionIndex, &face.vertices[1].texCoordinateIndex, &face.vertices[1].normalIndex,
				&face.vertices[2].positionIndex, &face.vertices[2].texCoordinateIndex, &face.vertices[2].normalIndex
			);
		}

		const u64 groupIndex = groups.Size() - 1;
		groups[groupIndex].faces.PushBack(face);
	}

	void ResourceLoader<MeshResource>::ProcessSubObject(DynamicArray<vec3>& positions, DynamicArray<vec3>& normals, DynamicArray<vec2>& texCoords, DynamicArray<MeshFaceData>& faces, GeometryConfig<Vertex3D, u32>* outData) const
	{
		auto indices = DynamicArray<u32>(32768);
		auto vertices = DynamicArray<Vertex3D>(32768);

		bool extentSet = false;
		outData->minExtents = vec3(0);
		outData->maxExtents = vec3(0);

		u64 faceCount = faces.Size();
		u64 normalCount = normals.Size();
		u64 texCoordinateCount = texCoords.Size();

		bool skipNormals = false;
		bool skipTextureCoordinates = false;

		if (normalCount == 0)
		{
			m_logger.Warn("ProcessSubObject() - No normals are present in this model.");
			skipNormals = true;
		}

		if (texCoordinateCount == 0)
		{
			m_logger.Warn("ProcessSubObject() - No texture coordinates are present in this model.");
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

				vec3 pos = positions[indexData.positionIndex - 1];
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
		outData->indices = indices;
	}

	bool ResourceLoader<MeshResource>::ImportObjMaterialLibraryFile(const char* mtlFilePath) const
	{
		m_logger.Debug("Importing .mtl file: '{}'", mtlFilePath);

		// Grab the .mtl file, if it exists, and read the material information.
		File mtlFile;
		if (!mtlFile.Open(mtlFilePath, FileModeRead))
		{
			m_logger.Error("Unable to open .mtl file: '{}'", mtlFilePath);
			return false;
		}

		MaterialConfig currentConfig{};

		bool hitName = false;

		string line;
		line.reserve(512);
		while (mtlFile.ReadLine(line))
		{
			Trim(line);
			if (line.empty()) continue; // Skip empty lines

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
					if (line[1] == 's')
					{
						char t[3];
						sscanf(line.data(), "%s %f", t, &currentConfig.shininess);
					}
					break;
				case 'm':
					ObjMaterialParseMapLine(line, currentConfig);
					break;
				case 'b':
				{
					char substr[10];
					char textureFileName[512];
					sscanf(line.data(), "%s %s", substr, textureFileName);
					if (IEquals(substr, "bump", 4))
					{
						FileSystem::FileNameFromPath(currentConfig.normalMapName, textureFileName);
					}
					break;
				}
				case 'n':
					ObjMaterialParseNewMtlLine(line, currentConfig, hitName, mtlFilePath);
					break;
				default:
					m_logger.Error("ImportObjMaterialLibraryFile() - Unknown starting character found: '{}' in line {}", line[0], line);
					break;
			}
		}

		currentConfig.shaderName = StringDuplicate(BUILTIN_SHADER_NAME_MATERIAL);
		if (currentConfig.shininess == 0.0f) currentConfig.shininess = 8.0f;

		if (!WriteMtFile(mtlFilePath, &currentConfig))
		{
			m_logger.Error("Unable to write mt file: '{}'", mtlFilePath);
			return false;
		}

		mtlFile.Close();
		return true;
	}

	void ResourceLoader<MeshResource>::ObjMaterialParseColorLine(const string& line, MaterialConfig& config) const
	{
		switch (line[1])
		{
			case 'a': // Ambient or diffuse color which we both treat the same
			case 'd':
			{
				char t[3];
				sscanf(line.data(), "%s %f %f %f", t, &config.diffuseColor.r, &config.diffuseColor.g, &config.diffuseColor.b);
				// NOTE: This is only used in the color shader. Transparency could be added as a material property later
				config.diffuseColor.a = 1.0f;
				break;
			}
			case 's': // Specular color
			{
				char t[3];
				// NOTE: this is not used for now
				f32 ignore = 0.0f;
				sscanf(line.data(), "%s %f %f %f", t, &ignore, &ignore, &ignore);
				break;
			}
			default:
				m_logger.Warn("ObjMaterialParseColorLine() - Unknown second character found: '{}' on line: '{}'", line[1], line);
				break;
		}
	}

	void ResourceLoader<MeshResource>::ObjMaterialParseMapLine(const string& line, MaterialConfig& config) const
	{
		char substr[10];
		char textureFileName[512];

		sscanf(line.data(), "%s %s", substr, textureFileName);

		if (IEquals(substr, "map_Kd", 6))
		{
			// Diffuse texture map
			FileSystem::FileNameFromPath(config.diffuseMapName, textureFileName);
		}
		else if (IEquals(substr, "map_Ks", 6))
		{
			FileSystem::FileNameFromPath(config.specularMapName, textureFileName);
		}
		else if (IEquals(substr, "map_bump", 8))
		{
			FileSystem::FileNameFromPath(config.normalMapName, textureFileName);
		}
	}

	void ResourceLoader<MeshResource>::ObjMaterialParseNewMtlLine(const string& line, MaterialConfig& config, bool& hitName, const char* mtlFilePath) const
	{
		char substr[10];
		char materialName[512];

		sscanf(line.data(), "%s %s", substr, materialName);
		if (IEquals(substr, "newmtl", 6))
		{
			// It's a material name

			// NOTE: Hardcoded default material shader name because all objects imported this way will be treated the same
			config.shaderName = StringDuplicate(BUILTIN_SHADER_NAME_MATERIAL);
			// NOTE: Shininess of 0 will cause problems so use a default if not provided
			if (config.shininess == 0.0f) config.shininess = 8.0f;

			if (hitName)
			{
				// Write out a mt file and move on.
				if (!WriteMtFile(mtlFilePath, &config))
				{
					m_logger.Error("Unable to write mt file: '{}'", mtlFilePath);
					return;
				}

				config = {};
			}

			hitName = true;
			StringNCopy(config.name, materialName, 256);
		}
	}

	bool ResourceLoader<MeshResource>::WriteMtFile(const char* mtlFilePath, MaterialConfig* config) const
	{
		// NOTE: The .obj file is in the models directory we have to move up 1 directory and go into the materials directory
		const auto formatStr = "%s../materials/%s.%s";

		File file;
		char directory[320];
		FileSystem::DirectoryFromPath(directory, mtlFilePath);
		char fullPath[512];

		StringFormat(fullPath, formatStr, directory, config->name, "mt");
		if (!file.Open(fullPath, FileModeWrite))
		{
			m_logger.Error("WriteMtFile() - Failed to open material file for writing: '{}'", fullPath);
			return false;
		}

		m_logger.Debug("Writing .mt file: '{}'", fullPath);

		char lineBuffer[512];
		file.WriteLine("#material file");
		file.WriteLine("");
		file.WriteLine("version = 0.1"); // TODO: hardcoded version
		StringFormat(lineBuffer, "name = %s", config->name);
		file.WriteLine(lineBuffer);
		StringFormat(lineBuffer, "diffuseColor = %.6f %.6f %.6f %.6f", config->diffuseColor.r, config->diffuseColor.g, config->diffuseColor.b, config->diffuseColor.a);
		file.WriteLine(lineBuffer);
		StringFormat(lineBuffer, "shininess = %.6f", config->shininess);
		file.WriteLine(lineBuffer);
		if (config->diffuseMapName[0])
		{
			StringFormat(lineBuffer, "diffuseMapName = %s", config->diffuseMapName);
			file.WriteLine(lineBuffer);
		}
		if (config->specularMapName[0])
		{
			StringFormat(lineBuffer, "specularMapName = %s", config->specularMapName);
			file.WriteLine(lineBuffer);
		}
		if (config->normalMapName[0])
		{
			StringFormat(lineBuffer, "normalMapName = %s", config->normalMapName);
			file.WriteLine(lineBuffer);
		}
		StringFormat(lineBuffer, "shader = %s", config->shaderName);
		file.WriteLine(lineBuffer);


		file.Close();
		return true;
	}
}
