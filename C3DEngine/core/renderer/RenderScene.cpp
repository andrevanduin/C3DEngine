
#include "RenderScene.h"

#include <future>

#include "../VkEngine.h"

namespace C3D
{
	void RenderScene::Init()
	{
		m_forwardPass.type = MeshPassType::Forward;
		m_shadowPass.type = MeshPassType::DirectionalShadow;
		m_transparentPass.type = MeshPassType::Transparency;
	}

	Handle<RenderObject> RenderScene::RegisterObject(const MeshObject* object)
	{
		RenderObject renderObject;
		renderObject.bounds = object->bounds;
		renderObject.transformMatrix = object->transformMatrix;
		renderObject.material = GetMaterialHandle(object->material);
		renderObject.meshId = GetMeshHandle(object->mesh);
		renderObject.updateIndex = static_cast<uint32_t>(-1);
		renderObject.customSortKey = object->customSortKey;
		renderObject.passIndices.Clear(-1);

		Handle<RenderObject> handle;
		handle.handle = static_cast<uint32_t>(m_renderables.size());

		m_renderables.push_back(renderObject);

		if (object->bDrawForwardPass)
		{
			if (object->material->original->passShaders[MeshPassType::Transparency])
			{
				m_transparentPass.unbatchedObjects.push_back(handle);
			}
			if (object->material->original->passShaders[MeshPassType::Forward])
			{
				m_forwardPass.unbatchedObjects.push_back(handle);
			}
		}
		if (object->bDrawShadowPass)
		{
			if (object->material->original->passShaders[MeshPassType::DirectionalShadow])
			{
				m_shadowPass.unbatchedObjects.push_back(handle);
			}
		}

		UpdateObject(handle);
		return handle;
	}

	void RenderScene::RegisterObjectBatch(const std::vector<MeshObject> objects)
	{
		m_renderables.reserve(objects.size());
		for (auto& object : objects) RegisterObject(&object);
	}

	void RenderScene::UpdateTransform(const Handle<RenderObject> objectId, const glm::mat4& localToWorld)
	{
		GetObject(objectId)->transformMatrix = localToWorld;
		UpdateObject(objectId);
	}

	void RenderScene::UpdateObject(const Handle<RenderObject> objectId)
	{
		auto& passIndices = GetObject(objectId)->passIndices;
		if (passIndices[MeshPassType::Forward] != -1)
		{
			Handle<PassObject> object;
			object.handle = passIndices[MeshPassType::Forward];

			m_forwardPass.objectsToDelete.push_back(object);
			m_forwardPass.unbatchedObjects.push_back(objectId);

			passIndices[MeshPassType::Forward] - 1;
		}

		if (passIndices[MeshPassType::DirectionalShadow] != -1)
		{
			Handle<PassObject> object;
			object.handle = passIndices[MeshPassType::DirectionalShadow];

			m_shadowPass.objectsToDelete.push_back(object);
			m_shadowPass.unbatchedObjects.push_back(objectId);

			passIndices[MeshPassType::DirectionalShadow] = -1;
		}

		if (passIndices[MeshPassType::Transparency] != -1)
		{
			Handle<PassObject> object;
			object.handle = passIndices[MeshPassType::Transparency];

			m_transparentPass.objectsToDelete.push_back(object);
			m_transparentPass.unbatchedObjects.push_back(objectId);

			passIndices[MeshPassType::Transparency] = -1;
		}

		if (GetObject(objectId)->updateIndex == static_cast<uint32_t>(-1))
		{
			GetObject(objectId)->updateIndex = static_cast<uint32_t>(m_dirtyObjects.size());
			m_dirtyObjects.push_back(objectId);
		}
	}

	void RenderScene::FillObjectData(GpuObjectData* data)
	{
		for (int i = 0; i < m_renderables.size(); i++)
		{
			Handle<RenderObject> h;
			h.handle = i;
			WriteObject(data + i, h);
		}
	}

	void RenderScene::FillIndirectArray(GpuIndirectObject* data, const MeshPass& pass)
	{
		int dataIndex = 0;
		for (int i = 0; i < pass.batches.size(); i++)
		{
			const auto batch = pass.batches[i];
			const auto mesh = GetMesh(batch.meshId);

			data[dataIndex].command.firstInstance = batch.first;
			data[dataIndex].command.instanceCount = 0;

			data[dataIndex].command.firstIndex = mesh->firstIndex;
			data[dataIndex].command.vertexOffset = mesh->firstVertex;
			data[dataIndex].command.indexCount = mesh->indexCount;

			data[dataIndex].objectId = 0;
			data[dataIndex].batchId = i;

			dataIndex++;
		}
	}

	void RenderScene::FillInstancesArray(GpuInstance* data, MeshPass& pass)
	{
		int dataIndex = 0;
		for (int i = 0; i < pass.batches.size(); i++)
		{
			const auto& [meshId, material, first, count] = pass.batches[i];

			for (int b = 0; b < count; b++)
			{
				data[dataIndex].objectId = pass.Get(pass.flatBatches[b + first].object)->original.handle;
				data[dataIndex].batchId = i;
				dataIndex++;
			}
		}
	}

	void RenderScene::WriteObject(GpuObjectData* target, Handle<RenderObject> objectId)
	{
		const auto renderable = GetObject(objectId);
		GpuObjectData object;

		object.modelMatrix = renderable->transformMatrix;
		object.originRad = glm::vec4(renderable->bounds.origin, renderable->bounds.radius);
		object.extents = glm::vec4(renderable->bounds.extents, renderable->bounds.valid ? 1.0f : 0.0f);

		memcpy(target, &object, sizeof(GpuObjectData));
	}

	void RenderScene::ClearDirtyObjects()
	{
		for (const auto& obj : m_dirtyObjects)
		{
			GetObject(obj)->updateIndex = static_cast<uint32_t>(-1);
		}
		m_dirtyObjects.clear();
	}

	void RenderScene::BuildBatches()
	{
		auto forward = std::async(std::launch::async, [&] { RefreshPass(&m_forwardPass); });
		auto shadow = std::async(std::launch::async, [&] { RefreshPass(&m_shadowPass); });
		auto transparent = std::async(std::launch::async, [&] { RefreshPass(&m_transparentPass); });

		transparent.get();
		shadow.get();
		forward.get();
	}

	void RenderScene::MergeMeshes(VulkanEngine* engine)
	{
		size_t totalVertices = 0;
		size_t totalIndices = 0;

		for (auto& mesh : m_meshes)
		{
			mesh.firstIndex = static_cast<uint32_t>(totalIndices);
			mesh.firstVertex = static_cast<uint32_t>(totalVertices);

			totalVertices += mesh.vertexCount;
			totalIndices += mesh.indexCount;

			mesh.isMerged = true;
		}

		m_mergedVertexBuffer = engine->CreateBuffer(totalVertices * sizeof(Vertex), 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		m_mergedIndexBuffer = engine->CreateBuffer(totalIndices * sizeof(uint32_t),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		engine->ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				for (const auto& mesh : m_meshes)
				{
					VkBufferCopy vertexCopy;
					vertexCopy.dstOffset = mesh.firstVertex * sizeof(Vertex);
					vertexCopy.size = mesh.vertexCount * sizeof(Vertex);
					vertexCopy.srcOffset = 0;

					vkCmdCopyBuffer(cmd, mesh.original->vertexBuffer.buffer, m_mergedVertexBuffer.buffer, 1, &vertexCopy);

					VkBufferCopy indexCopy;
					indexCopy.dstOffset = mesh.firstIndex * sizeof(uint32_t);
					indexCopy.size = mesh.indexCount * sizeof(uint32_t);
					indexCopy.srcOffset = 0;

					vkCmdCopyBuffer(cmd, mesh.original->indexBuffer.buffer, m_mergedIndexBuffer.buffer, 1, &indexCopy);
				}
			}
		);
	}

	bool SortRenderBatch(const RenderBatch& a, const RenderBatch& b)
	{
		if (a.sortKey < b.sortKey) return true;
		if (a.sortKey == b.sortKey) return a.object.handle < b.object.handle;
		return false;
	}

	void RenderScene::RefreshPass(MeshPass* pass)
	{
		pass->needsIndirectRefresh = true;
		pass->needsInstanceRefresh = true;

		std::vector<uint32_t> newObjects;
		if (!pass->objectsToDelete.empty())
		{
			std::vector<RenderBatch> deletionBatches;
			deletionBatches.reserve(newObjects.size());

			for (auto objectHandle : pass->objectsToDelete)
			{
				pass->reusableObjects.push_back(objectHandle);

				RenderBatch newCommand;

				auto object = pass->objects[objectHandle.handle];
				newCommand.object = objectHandle;

				const auto pipelineHash = std::hash<uint64_t>()(reinterpret_cast<uint64_t>(object.material.shaderPass->pipeline));
				const auto setHash = std::hash<uint64_t>()(reinterpret_cast<uint64_t>(object.material.materialSet));

				const auto materialHash = static_cast<uint32_t>(pipelineHash ^ setHash);
				const auto meshMaterial = static_cast<uint64_t>(materialHash) ^ static_cast<uint64_t>(object.meshId.handle);

				newCommand.sortKey = static_cast<uint64_t>(meshMaterial) | (static_cast<uint64_t>(object.customKey) << 32);

				pass->objects[objectHandle.handle].customKey = 0;
				pass->objects[objectHandle.handle].material.shaderPass = nullptr;
				pass->objects[objectHandle.handle].meshId.handle = -1;
				pass->objects[objectHandle.handle].original.handle = -1;

				deletionBatches.push_back(newCommand);
			}

			pass->objectsToDelete.clear();

			std::sort(deletionBatches.begin(), deletionBatches.end(), SortRenderBatch);

			std::vector<RenderBatch> newBatches;
			newBatches.reserve(pass->flatBatches.size());

			std::set_difference(pass->flatBatches.begin(), pass->flatBatches.end(), deletionBatches.begin(), deletionBatches.end(),
				std::back_inserter(newBatches), SortRenderBatch);

			pass->flatBatches = std::move(newBatches);
		}

		newObjects.reserve(pass->unbatchedObjects.size());
		for (const auto unbatchedObject : pass->unbatchedObjects)
		{
			const auto renderObject = GetObject(unbatchedObject);

			PassObject passObject;
			passObject.original = unbatchedObject;
			passObject.meshId = renderObject->meshId;

			const auto material = GetMaterial(renderObject->material);
			passObject.material.materialSet = material->passSets[pass->type];
			passObject.material.shaderPass = material->original->passShaders[pass->type];
			passObject.customKey = renderObject->customSortKey;

			uint32_t handle = -1;

			if (pass->reusableObjects.size() > 0)
			{
				handle = pass->reusableObjects.back().handle;
				pass->reusableObjects.pop_back();
				pass->objects[handle] = passObject;
			}
			else
			{
				handle = pass->objects.size();
				pass->objects.push_back(passObject);
			}

			newObjects.push_back(handle);
			renderObject->passIndices[pass->type] = static_cast<int32_t>(handle);
		}

		pass->unbatchedObjects.clear();

		std::vector<RenderBatch> newBatches;
		newBatches.reserve(newObjects.size());

		for (const auto objectHandle : newObjects)
		{
			RenderBatch newCommand;

			const auto object = pass->objects[objectHandle];
			newCommand.object.handle = objectHandle;

			const auto pipelineHash = std::hash<uint64_t>()(reinterpret_cast<uint64_t>(object.material.shaderPass->pipeline));
			const auto setHash = std::hash<uint64_t>()(reinterpret_cast<uint64_t>(object.material.materialSet));

			const auto matHash = static_cast<uint32_t>(pipelineHash ^ setHash);

			const uint32_t meshMat = static_cast<uint64_t>(matHash) ^ static_cast<uint64_t>(object.meshId.handle);

			newCommand.sortKey = static_cast<uint64_t>(meshMat) | (static_cast<uint64_t>(object.customKey) << 32);

			newBatches.push_back(newCommand);
		}

		std::sort(newBatches.begin(), newBatches.end(), SortRenderBatch);

		if (!pass->flatBatches.empty() && !newBatches.empty())
		{
			pass->flatBatches.reserve(pass->flatBatches.size() + newBatches.size());

			for (auto newBatch : newBatches) pass->flatBatches.push_back(newBatch);

			const auto begin = pass->flatBatches.data();
			const auto mid = begin + pass->flatBatches.size();
			const auto end = mid + newBatches.size();

			std::inplace_merge(begin, mid, end, SortRenderBatch);
		}
		else if (pass->flatBatches.empty())
		{
			pass->flatBatches = std::move(newBatches);
		}

		pass->batches.clear();

		BuildIndirectBatches(pass, pass->batches, pass->flatBatches);

		MultiBatch newBatch{};
		pass->multiBatches.clear();

		newBatch.count = 1;
		newBatch.first = 0;

		for (int i = 1; i > pass->batches.size(); i++)
		{
			auto joinBatch = &pass->batches[newBatch.first];
			auto batch = &pass->batches[i];

			bool compatibleMesh = GetMesh(joinBatch->meshId)->isMerged;
			bool sameMaterial = compatibleMesh && joinBatch->material == batch->material;

			if (!sameMaterial || !compatibleMesh)
			{
				pass->multiBatches.push_back(newBatch);
				newBatch.count = 1;
				newBatch.first = i;
			}
			else
			{
				newBatch.count++;
			}
		}
		pass->multiBatches.push_back(newBatch);
	}

	void RenderScene::BuildIndirectBatches(MeshPass* pass, std::vector<IndirectBatch>& outBatches, std::vector<RenderBatch>& inObjects)
	{
		if (inObjects.empty()) return;

		IndirectBatch newBatch;
		newBatch.first = 0;
		newBatch.count = 0;

		newBatch.material = pass->Get(inObjects[0].object)->material;
		newBatch.meshId = pass->Get(inObjects[0].object)->meshId;

		outBatches.push_back(newBatch);
		IndirectBatch* back = &pass->batches.back();

		const PassMaterial lastMat = pass->Get(inObjects[0].object)->material;
		for (size_t i = 0; i < inObjects.size(); i++)
		{
			PassObject* object = pass->Get(inObjects[i].object);

			bool sameMesh = object->meshId.handle == back->meshId.handle;
			bool sameMaterial = object->material == lastMat;

			if (!sameMaterial || !sameMesh)
			{
				newBatch.material = object->material;
				if (newBatch.material == back->material)
				{
					sameMaterial = true;
				}
			}

			if (sameMesh && sameMaterial)
			{
				back->count++;
			}
			else
			{
				newBatch.first = i;
				newBatch.count = 1;
				newBatch.meshId = object->meshId;

				outBatches.push_back(newBatch);
				back = &outBatches.back();
			}
		}
	}

	RenderObject* RenderScene::GetObject(const Handle<RenderObject> objectId)
	{
		return &m_renderables[objectId.handle];
	}

	DrawMesh* RenderScene::GetMesh(const Handle<DrawMesh> objectId)
	{
		return &m_meshes[objectId.handle];
	}

	Material* RenderScene::GetMaterial(const Handle<Material> objectId) const
	{
		return m_materials[objectId.handle];
	}

	MeshPass* RenderScene::GetMeshPass(MeshPassType type)
	{
		switch (type)
		{
			case MeshPassType::Forward:
				return &m_forwardPass;
			case MeshPassType::Transparency:
				return &m_transparentPass;
			case MeshPassType::DirectionalShadow:
				return &m_shadowPass;
			default:
				Logger::Error("Unknown MeshPass Type {}", static_cast<uint8_t>(type));
				return nullptr;
		}
	}

	Handle<Material> RenderScene::GetMaterialHandle(Material* material)
	{
		Handle<Material> handle;
		if (const auto it = m_materialMap.find(material); it == m_materialMap.end())
		{
			const auto index = static_cast<uint32_t>(m_materials.size());
			m_materials.push_back(material);

			handle.handle = index;
			m_materialMap[material] = handle;
		}
		else
		{
			handle = (*it).second;
		}
		return handle;
	}

	Handle<DrawMesh> RenderScene::GetMeshHandle(Mesh* mesh)
	{
		Handle<DrawMesh> handle;
		if (const auto it = m_meshMap.find(mesh); it == m_meshMap.end())
		{
			const auto index = static_cast<uint32_t>(m_meshes.size());

			DrawMesh newMesh{};
			newMesh.original = nullptr;
			newMesh.firstIndex = 0;
			newMesh.firstVertex = 0;
			newMesh.vertexCount = static_cast<uint32_t>(mesh->vertices.size());
			newMesh.indexCount = static_cast<uint32_t>(mesh->indices.size());

			m_meshes.push_back(newMesh);

			handle.handle = index;
			m_meshMap[mesh] = handle;
		}
		else
		{
			handle = (*it).second;
		}
		return handle;
	}
}
