
#pragma once
#include <vector>

#include "RenderPass.h"
#include "../VkFrame.h"

struct MeshObject;

namespace C3D
{
	class RenderScene
	{
	public:
		void Init();

		Handle<RenderObject> RegisterObject(const MeshObject* object);
		void RegisterObjectBatch(std::vector<MeshObject> objects);

		void UpdateTransform(Handle<RenderObject> objectId, const glm::mat4& localToWorld);
		void UpdateObject(Handle<RenderObject> objectId);

		void FillObjectData(GpuObjectData* data);
		void FillIndirectArray(GpuIndirectObject* data, const MeshPass& pass);
		static void FillInstancesArray(GpuInstance* data, MeshPass& pass);

		void WriteObject(GpuObjectData* target, Handle<RenderObject> objectId);

		void ClearDirtyObjects();

		void BuildBatches();

		void MergeMeshes(class VulkanEngine* engine);

		void RefreshPass(MeshPass* pass);

		void BuildIndirectBatches(MeshPass* pass, std::vector<IndirectBatch>& outBatches, std::vector<RenderBatch>& inObjects);

		RenderObject* GetObject(Handle<RenderObject> objectId);
		DrawMesh* GetMesh(Handle<DrawMesh> objectId);
		[[nodiscard]] Material* GetMaterial(Handle<Material> objectId) const;

		MeshPass* GetMeshPass(MeshPassType type);

		Handle<Material> GetMaterialHandle(Material* material);
		Handle<DrawMesh> GetMeshHandle(Mesh* mesh);

	private:
		std::vector<RenderObject> m_renderables;
		std::vector<DrawMesh> m_meshes;
		std::vector<Material*> m_materials;

		std::vector<Handle<RenderObject>> m_dirtyObjects;

		MeshPass m_forwardPass;
		MeshPass m_transparentPass;
		MeshPass m_shadowPass;

		std::unordered_map<Material*, Handle<Material>> m_materialMap;
		std::unordered_map<Mesh*, Handle<DrawMesh>> m_meshMap;

		AllocatedBuffer<Vertex> m_mergedVertexBuffer;
		AllocatedBuffer<uint32_t> m_mergedIndexBuffer;

		AllocatedBuffer<GpuObjectData> m_objectDataBuffer;
	};
}
