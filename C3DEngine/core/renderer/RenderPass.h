
#pragma once
#include "../VkTypes.h"
#include "../materials/MaterialSystem.h"

#include "Mesh.h"

#include <vector>

namespace C3D
{
	template<typename T>
	struct Handle {
		uint32_t handle;
	};

	struct GpuIndirectObject
	{
		VkDrawIndexedIndirectCommand command;
		uint32_t objectId;
		uint32_t batchId;
	};

	struct DrawMesh
	{
		uint32_t firstVertex;
		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t vertexCount;

		bool isMerged;

		Mesh* original;
	};

	struct GpuInstance
	{
		uint32_t objectId;
		uint32_t batchId;
	};

	struct PassMaterial
	{
		VkDescriptorSet materialSet{ nullptr };
		ShaderPass* shaderPass{ nullptr };

		bool operator==(const PassMaterial& other) const
		{
			return materialSet == other.materialSet && shaderPass == other.shaderPass;
		}
	};

	struct RenderObject
	{
		Handle<DrawMesh> meshId;
		Handle<Material> material;

		uint32_t updateIndex = 0;
		uint32_t customSortKey = 0;

		PerPassData<int32_t> passIndices;

		glm::mat4 transformMatrix;

		RenderBounds bounds;
	};

	struct PassObject
	{
		PassMaterial material;
		Handle<DrawMesh> meshId;
		Handle<RenderObject> original;

		uint32_t builtBatch = 0;
		uint32_t customKey = 0;
	};

	struct RenderBatch
	{
		Handle<PassObject> object;
		uint64_t sortKey = 0;

		bool operator==(const RenderBatch& other) const
		{
			return object.handle == other.object.handle && sortKey == other.sortKey;
		}
	};

	struct IndirectBatch
	{
		Handle<DrawMesh> meshId;
		PassMaterial material;

		uint32_t first;
		uint32_t count;
	};

	struct MultiBatch
	{
		uint32_t first;
		uint32_t count;
	};

	struct MeshPass
	{
		std::vector<MultiBatch> multiBatches;
		std::vector<IndirectBatch> batches;
		std::vector<RenderBatch> flatBatches;

		std::vector<Handle<RenderObject>> unbatchedObjects;
		
		std::vector<PassObject> objects;

		std::vector<Handle<PassObject>> reusableObjects;
		std::vector<Handle<PassObject>> objectsToDelete;

		AllocatedBuffer<uint32_t> compactedInstanceBuffer;
		AllocatedBuffer<GpuInstance> passObjectsBuffer;

		AllocatedBuffer<GpuIndirectObject> drawIndirectBuffer;
		AllocatedBuffer<GpuIndirectObject> clearIndirectBuffer;

		MeshPassType type;

		bool needsIndirectRefresh = true;
		bool needsInstanceRefresh = true;

		PassObject* Get(Handle<PassObject> handle);
	};
}
