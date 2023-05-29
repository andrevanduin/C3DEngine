
#include "primitive_renderer.h"

#include "core/identifier.h"
#include "renderer/renderer_frontend.h"
#include "resources/mesh.h"
#include "systems/render_view_system.h"

namespace C3D
{
	PrimitiveRenderer::PrimitiveRenderer() : m_logger("PRIMITIVE_RENDERER") {}

	void PrimitiveRenderer::OnCreate()
	{
		// View
		RenderViewConfig config{};
		config.type = RenderViewKnownType::Primitives;
		config.width = 1280;
		config.height = 720;
		config.name = "primitives";
		config.passCount = 1;
		config.viewMatrixSource = RenderViewViewMatrixSource::SceneCamera;

		RenderPassConfig pass;
		pass.name = "RenderPass.Builtin.Primitives";
		pass.renderArea = { 0, 0, 1280, 720 };
		pass.clearColor = { 0, 0, 0.2f, 1.0f };
		pass.clearFlags = ClearDepthBuffer | ClearStencilBuffer;
		pass.depth = 1.0f;
		pass.stencil = 0;

		RenderTargetAttachmentConfig targetAttachments[2]{};
		targetAttachments[0].type = RenderTargetAttachmentType::Color;
		targetAttachments[0].source = RenderTargetAttachmentSource::Default;
		targetAttachments[0].loadOperation = RenderTargetAttachmentLoadOperation::Load;
		targetAttachments[0].storeOperation = RenderTargetAttachmentStoreOperation::Store;
		targetAttachments[0].presentAfter = false;
		
		targetAttachments[1].type = RenderTargetAttachmentType::Depth;
		targetAttachments[1].source = RenderTargetAttachmentSource::Default;
		targetAttachments[1].loadOperation = RenderTargetAttachmentLoadOperation::DontCare;
		targetAttachments[1].storeOperation = RenderTargetAttachmentStoreOperation::Store;
		targetAttachments[1].presentAfter = false;

		pass.target.attachments.PushBack(targetAttachments[0]);
		pass.target.attachments.PushBack(targetAttachments[1]);
		pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

		config.passes.PushBack(pass);

		if (!Views.Create(config))
		{
			Logger::Fatal("Failed to create Primitive Renderer View");
			return;
		}

		for (auto& mesh : m_meshes)
		{
			mesh.generation = INVALID_ID_U8;
			mesh.uniqueId	= INVALID_ID_U32;
		}
	}

	Mesh* PrimitiveRenderer::AddLine(const vec3& start, const vec3& end, const vec4& color)
	{
		const auto mesh = GetMesh();

		GeometryConfig<PrimitiveVertex3D, u32> config;
		config.name = "Line";
		config.materialName = "";
		config.vertices.PushBack({ start, color });
		config.vertices.PushBack({ end, color	});

		config.indices.PushBack(0);
		config.indices.PushBack(1);

		mesh->geometries.PushBack(Geometric.AcquireFromConfig(config, true));
		return mesh; 
	}

	Mesh* PrimitiveRenderer::AddPlane(const Plane3D& plane, const vec4& color)
	{
		const auto mesh = GetMesh();

		// Plane equation ax + by + cz = d
		// d = plane.distance
		// a = plane.normal.x
		// b = plane.normal.y
		// c = plane.normal.z

		GeometryConfig<PrimitiveVertex3D, u32> config;
		config.name = "Plane";
		config.materialName = "";

		const auto xN = plane.normal.x;
		const auto yN = plane.normal.y;
		const auto zN = plane.normal.z;

		const auto pX = EpsilonEqual(xN, 0.0f) ? 0.0f : plane.distance / xN;
		const auto pY = EpsilonEqual(yN, 0.0f) ? 0.0f : plane.distance / yN;
		const auto pZ = EpsilonEqual(zN, 0.0f) ? 0.0f : plane.distance / zN;

		const vec3 planePoint0 = { pX, 0, 0 };
		const vec3 planePoint1 = { 0, pY, 0 };
		const vec3 planePoint2 = { 0, 0, pZ };

		vec3 p0, p1;
		if (planePoint0 != planePoint1)
		{
			p0 = planePoint0;
			p1 = planePoint1;
		}
		else if (planePoint0 != planePoint2)
		{
			p0 = planePoint0;
			p1 = planePoint2;
		}
		else
		{
			p0 = planePoint1;
			p1 = planePoint2;
		}

		const vec3 t = normalize(p1 - p0);
		const vec3 b = cross(t, plane.normal);

		constexpr float d = 50.0f;

		const vec3 v1 = p1 - t * d - b * d;
		const vec3 v2 = p1 + t * d - b * d;
		const vec3 v3 = p1 + t * d + b * d;
		const vec3 v4 = p1 - t * d + b * d;

		config.vertices.PushBack({ v1, color });
		config.vertices.PushBack({ v2, color });
		config.vertices.PushBack({ v3, color });
		config.vertices.PushBack({ v4, color });

		config.indices.PushBack(0);
		config.indices.PushBack(1);
		config.indices.PushBack(1);
		config.indices.PushBack(2);
		config.indices.PushBack(2);
		config.indices.PushBack(3);
		config.indices.PushBack(3);
		config.indices.PushBack(0);

		mesh->geometries.PushBack(Geometric.AcquireFromConfig(config, true));

		return mesh;
	}

	Mesh* PrimitiveRenderer::AddBox(const vec3& center, const vec3& halfExtents)
	{
		const auto mesh = GetMesh();
		GeometryConfig<PrimitiveVertex3D, u32> config;
		config.name = "Box";
		config.materialName = "";
		config.vertices.Resize(8);
		config.indices.Resize(24);

		f32 minX = center.x - halfExtents.x;
		f32 minY = center.y - halfExtents.y;
		f32 minZ = center.z - halfExtents.z;
		f32 maxX = center.x + halfExtents.x;
		f32 maxY = center.y + halfExtents.y;
		f32 maxZ = center.z + halfExtents.z;

		config.minExtents = { minX, minY, minZ };
		config.maxExtents = { maxX, maxY, maxZ };
		config.center = center;

		// Front face
		config.vertices[0].position = { minX, minY, maxZ };
		config.vertices[1].position = { maxX, minY, maxZ };
		config.vertices[2].position = { maxX, maxY, maxZ };
		config.vertices[3].position = { minX, maxY, maxZ };

		// Back face
		config.vertices[4].position = { minX, minY, minZ };
		config.vertices[5].position = { maxX, minY, minZ };
		config.vertices[6].position = { maxX, maxY, minZ };
		config.vertices[7].position = { minX, maxY, minZ };

		// Front face
		config.vertices[0].color = { 0, 1, 0, 1 };
		config.vertices[1].color = { 0, 1, 0, 1 };
		config.vertices[2].color = { 0, 1, 0, 1 };
		config.vertices[3].color = { 0, 1, 0, 1 };

		// Back face
		config.vertices[4].color = { 0, 1, 0, 1 };
		config.vertices[5].color = { 0, 1, 0, 1 };
		config.vertices[6].color = { 0, 1, 0, 1 };
		config.vertices[7].color = { 0, 1, 0, 1 };

		// Front face
		config.indices.PushBack(0);
		config.indices.PushBack(1);
		config.indices.PushBack(1);
		config.indices.PushBack(2);
		config.indices.PushBack(2);
		config.indices.PushBack(3);
		config.indices.PushBack(3);
		config.indices.PushBack(0);

		// Back face
		config.indices.PushBack(4 + 0);
		config.indices.PushBack(4 + 1);
		config.indices.PushBack(4 + 1);
		config.indices.PushBack(4 + 2);
		config.indices.PushBack(4 + 2);
		config.indices.PushBack(4 + 3);
		config.indices.PushBack(4 + 3);
		config.indices.PushBack(4 + 0);

		// Connection between the faces
		config.indices.PushBack(0);
		config.indices.PushBack(4);
		config.indices.PushBack(1);
		config.indices.PushBack(5);
		config.indices.PushBack(2);
		config.indices.PushBack(6);
		config.indices.PushBack(3);
		config.indices.PushBack(7);

		mesh->geometries.PushBack(Geometric.AcquireFromConfig(config, true));
		return mesh;
	}

	void PrimitiveRenderer::OnRender(LinearAllocator& frameAllocator, RenderPacket& packet)
	{
		PrimitivePacketData primitiveData = {};
		primitiveData.meshes.SetAllocator(&frameAllocator);
		for (auto& mesh : m_meshes)
		{
			if (mesh.generation != INVALID_ID_U8)
			{
				primitiveData.meshes.PushBack(&mesh);
			}
		}

		if (!Views.BuildPacket(Views.Get("primitives"), frameAllocator, &primitiveData, &packet.views[2]))
		{
			m_logger.Error("Failed to build packet for view: 'primitives'");
		}
	}

	void PrimitiveRenderer::Dispose(Mesh* mesh)
	{
		mesh->Unload();
	}

	Mesh* PrimitiveRenderer::GetMesh()
	{
		for (auto& mesh : m_meshes)
		{
			if (mesh.generation == INVALID_ID_U8)
			{
				mesh.uniqueId = Identifier::GetNewId(&mesh);
				mesh.generation++;

				return &mesh;
			}
		}

		return nullptr;
	}
}
