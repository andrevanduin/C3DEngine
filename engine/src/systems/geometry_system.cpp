
#include "geometry_system.h"

#include "core/c3d_string.h"
#include "core/logger.h"
#include "core/memory.h"

#include "systems/material_system.h"
#include "renderer/renderer_frontend.h"

#include "services/services.h"

namespace C3D
{
	GeometrySystem::GeometrySystem()
		: m_logger("GEOMETRY_SYSTEM"), m_initialized(false), m_config(), m_defaultGeometry(),
		  m_default2DGeometry(), m_registeredGeometries(nullptr)
	{}

	bool GeometrySystem::Init(const GeometrySystemConfig& config)
	{
		m_config = config;

		m_registeredGeometries = Memory.Allocate<GeometryReference>(m_config.maxGeometryCount, MemoryType::Geometry);

		const u32 count = m_config.maxGeometryCount;
		for (u32 i = 0; i < count; i++)
		{
			m_registeredGeometries[i].geometry.id = INVALID_ID;
			m_registeredGeometries[i].geometry.internalId = INVALID_ID;
			m_registeredGeometries[i].geometry.generation = INVALID_ID;
		}

		if (!CreateDefaultGeometries())
		{
			m_logger.Error("Failed to create default geometries");
			return false;
		}

		m_initialized = true;
		return true;
	}

	void GeometrySystem::Shutdown() const
	{
		Memory.Free(m_registeredGeometries, sizeof(GeometryReference) * m_config.maxGeometryCount, MemoryType::Geometry);
	}

	Geometry* GeometrySystem::AcquireById(const u32 id) const
	{
		if (id != INVALID_ID && m_registeredGeometries[id].geometry.id != INVALID_ID)
		{
			m_registeredGeometries->referenceCount++;
			return &m_registeredGeometries[id].geometry;
		}

		// NOTE: possible should return the default geometry instead
		m_logger.Error("AcquireById() cannot load invalid geometry id. Returning nullptr");
		return nullptr;
	}

	Geometry* GeometrySystem::AcquireFromConfig(const GeometryConfig& config, const bool autoRelease) const
	{
		Geometry* g = nullptr;
		for (u32 i = 0; i < m_config.maxGeometryCount; i++)
		{
			if (m_registeredGeometries[i].geometry.id == INVALID_ID)
			{
				// We found an empty slot
				m_registeredGeometries[i].autoRelease = autoRelease;
				m_registeredGeometries[i].referenceCount = 1;

				g = &m_registeredGeometries[i].geometry;
				g->id = i;
				break;
			}
		}

		if (!g)
		{
			m_logger.Error("Unable to obtain free slot for geometry. Adjust config to allow for more space. Returning nullptr");
			return nullptr;
		}

		if (!CreateGeometry(config, g))
		{
			m_logger.Error("Failed to create geometry. Returning nullptr");
			return nullptr;
		}

		return g;
	}

	void GeometrySystem::Release(const Geometry* geometry) const
	{
		if (geometry && geometry->id != INVALID_ID)
		{
			GeometryReference* ref = &m_registeredGeometries[geometry->id];

			const u32 id = geometry->id; // Copy the id
			if (ref->geometry.id == geometry->id)
			{
				if (ref->referenceCount > 0) ref->referenceCount--;

				if (ref->referenceCount < 1 && ref->autoRelease)
				{
					DestroyGeometry(&ref->geometry);
					ref->referenceCount = 0;
					ref->autoRelease = false;
				}
			}
			else
			{
				m_logger.Fatal("Geometry id mismatch. Check registration logic as this should never occur!");
			}
			return;
		}

		m_logger.Warn("Release() called with invalid geometry id. Noting was done");
	}

	Geometry* GeometrySystem::GetDefault()
	{
		if (!m_initialized)
		{
			m_logger.Fatal("GetDefault() called before system was initialized");
			return nullptr;
		}

		return &m_defaultGeometry;
	}

	Geometry* GeometrySystem::GetDefault2D()
	{
		if (!m_initialized)
		{
			m_logger.Fatal("GetDefault2D() called before system was initialized");
			return nullptr;
		}

		return &m_default2DGeometry;
	}

	GeometryConfig GeometrySystem::GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 tileX, f32 tileY, const string& name, const string& materialName) const
	{
		if (width == 0.f)	width = 1.0f;
		if (height == 0.f)	height = 1.0f;
		if (xSegmentCount < 1) xSegmentCount = 1;
		if (ySegmentCount < 1) ySegmentCount = 1;
		if (tileX == 0.f) tileX = 1.0f;
		if (tileY == 0.f) tileY = 1.0f;

		GeometryConfig config{};
		config.vertexSize = sizeof(Vertex3D);
		config.vertexCount = xSegmentCount * ySegmentCount * 4; // 4 vertices per segment
		config.vertices = Memory.Allocate<Vertex3D>(config.vertexCount, MemoryType::Array);
		config.indexSize = sizeof(u32);
		config.indexCount = xSegmentCount * ySegmentCount * 6; // 6 indices per segment
		config.indices = Memory.Allocate<u32>(config.indexCount, MemoryType::Array);

		// TODO: this generates extra vertices, but we can always deduplicate them later
		const f32 segWidth = width / static_cast<f32>(xSegmentCount);
		const f32 segHeight = height / static_cast<f32>(ySegmentCount);
		const f32 halfWidth = width * 0.5f;
		const f32 halfHeight = height * 0.5f;

		for (u32 y = 0; y < ySegmentCount; y++)
		{
			for (u32 x = 0; x < xSegmentCount; x++)
			{
				const f32 fx = static_cast<f32>(x);
				const f32 fy = static_cast<f32>(y);

				const f32 fxSegmentCount = static_cast<f32>(xSegmentCount);
				const f32 fySegmentCount = static_cast<f32>(ySegmentCount);

				// Generate vertices
				const f32 minX = (fx * segWidth) - halfWidth;
				const f32 minY = (fy * segHeight) - halfHeight;
				const f32 maxX = minX + segWidth;
				const f32 maxY = minY + segHeight;
				const f32 minUvx = (fx / fxSegmentCount) * tileX;
				const f32 minUvy = (fy / fySegmentCount) * tileY;
				const f32 maxUvx = ((fx + 1) / fxSegmentCount) * tileX;
				const f32 maxUvy = ((fy + 1) / fySegmentCount) * tileY;

				const u32 vOffset = (y * xSegmentCount + x) * 4;
				Vertex3D* v0 = &static_cast<Vertex3D*>(config.vertices)[vOffset + 0];
				Vertex3D* v1 = &static_cast<Vertex3D*>(config.vertices)[vOffset + 1];
				Vertex3D* v2 = &static_cast<Vertex3D*>(config.vertices)[vOffset + 2];
				Vertex3D* v3 = &static_cast<Vertex3D*>(config.vertices)[vOffset + 3];

				v0->position.x = minX;
				v0->position.y = minY;
				v0->texture.x = minUvx;
				v0->texture.y = minUvy;

				v1->position.x = maxX;
				v1->position.y = maxY;
				v1->texture.x = maxUvx;
				v1->texture.y = maxUvy;

				v2->position.x = minX;
				v2->position.y = maxY;
				v2->texture.x = minUvx;
				v2->texture.y = maxUvy;

				v3->position.x = maxX;
				v3->position.y = minY;
				v3->texture.x = maxUvx;
				v3->texture.y = minUvy;

				// Generate indices
				const u32 iOffset = (y * xSegmentCount + x) * 6;
				static_cast<u32*>(config.indices)[iOffset + 0] = vOffset + 0;
				static_cast<u32*>(config.indices)[iOffset + 1] = vOffset + 1;
				static_cast<u32*>(config.indices)[iOffset + 2] = vOffset + 2;
				static_cast<u32*>(config.indices)[iOffset + 3] = vOffset + 0;
				static_cast<u32*>(config.indices)[iOffset + 4] = vOffset + 3;
				static_cast<u32*>(config.indices)[iOffset + 5] = vOffset + 1;
			}
		}

		if (!name.empty())
		{
			StringNCopy(config.name, name.data(), GEOMETRY_NAME_MAX_LENGTH);
		}
		else
		{
			StringNCopy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
		}

		if (!materialName.empty())
		{
			StringNCopy(config.materialName, materialName.data(), MATERIAL_NAME_MAX_LENGTH);
		}
		else
		{
			StringNCopy(config.materialName, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
		}

		return config;
	}

	GeometryConfig GeometrySystem::GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 tileX, f32 tileY, const string& name, const string& materialName) const
	{
		if (width == 0.f)  width = 1.0f;
		if (height == 0.f) height = 1.0f;
		if (tileX == 0.f)  tileX = 1.0f;
		if (tileY == 0.f)  tileY = 1.0f;

		GeometryConfig config;
		config.vertexSize = sizeof(Vertex3D);
		config.vertexCount = 4 * 6; // 4 Vertices per side with 6 sides
		config.vertices = Memory.Allocate(sizeof(Vertex3D) * config.vertexCount, MemoryType::Array);
		config.indexSize = sizeof(u32);
		config.indexCount = 6 * 6; // 6 indices per side with 6 sides
		config.indices = Memory.Allocate(sizeof(u32) * config.indexCount, MemoryType::Array);

		const f32 halfWidth = width * 0.5f;
		const f32 halfHeight = height * 0.5f;
		const f32 halfDepth = depth * 0.5f;

		f32 minX = -halfWidth;
		f32 minY = -halfHeight;
		f32 minZ = -halfDepth;
		f32 maxX = halfWidth;
		f32 maxY = halfHeight;
		f32 maxZ = halfDepth;
		f32 minUvX = 0.0f;
		f32 minUvY = 0.0f;
		f32 maxUvX = tileX;
		f32 maxUvY = tileY;

		Vertex3D vertices[24]; // 6 * 4 vertices

		// Front face
		vertices[(0 * 4) + 0].position = { minX, minY, maxZ };
		vertices[(0 * 4) + 1].position = { maxX, maxY, maxZ };
		vertices[(0 * 4) + 2].position = { minX, maxY, maxZ };
		vertices[(0 * 4) + 3].position = { maxX, minY, maxZ };
		vertices[(0 * 4) + 0].texture = { minUvX, minUvY };
		vertices[(0 * 4) + 1].texture = { maxUvX, maxUvY };
		vertices[(0 * 4) + 2].texture = { minUvX, maxUvY };
		vertices[(0 * 4) + 3].texture = { maxUvX, minUvY };
		vertices[(0 * 4) + 0].normal = { 0.0f, 0.0f, 1.0f };
		vertices[(0 * 4) + 1].normal = { 0.0f, 0.0f, 1.0f };
		vertices[(0 * 4) + 2].normal = { 0.0f, 0.0f, 1.0f };
		vertices[(0 * 4) + 3].normal = { 0.0f, 0.0f, 1.0f };

		// Back face
		vertices[(1 * 4) + 0].position = { maxX, minY, minZ };
		vertices[(1 * 4) + 1].position = { minX, maxY, minZ };
		vertices[(1 * 4) + 2].position = { maxX, maxY, minZ };
		vertices[(1 * 4) + 3].position = { minX, minY, minZ };
		vertices[(1 * 4) + 0].texture = { minUvX, minUvY };
		vertices[(1 * 4) + 1].texture = { maxUvX, maxUvY };
		vertices[(1 * 4) + 2].texture = { minUvX, maxUvY };
		vertices[(1 * 4) + 3].texture = { maxUvX, minUvY };
		vertices[(1 * 4) + 0].normal = { 0.0f, 0.0f, -1.0f };
		vertices[(1 * 4) + 1].normal = { 0.0f, 0.0f, -1.0f };
		vertices[(1 * 4) + 2].normal = { 0.0f, 0.0f, -1.0f };
		vertices[(1 * 4) + 3].normal = { 0.0f, 0.0f, -1.0f };

		// Left face
		vertices[(2 * 4) + 0].position = { minX, minY, minZ };
		vertices[(2 * 4) + 1].position = { minX, maxY, maxZ };
		vertices[(2 * 4) + 2].position = { minX, maxY, minZ };
		vertices[(2 * 4) + 3].position = { minX, minY, maxZ };
		vertices[(2 * 4) + 0].texture = { minUvX, minUvY };
		vertices[(2 * 4) + 1].texture = { maxUvX, maxUvY };
		vertices[(2 * 4) + 2].texture = { minUvX, maxUvY };
		vertices[(2 * 4) + 3].texture = { maxUvX, minUvY };
		vertices[(2 * 4) + 0].normal = { -1.0f, 0.0f, 0.0f };
		vertices[(2 * 4) + 1].normal = { -1.0f, 0.0f, 0.0f };
		vertices[(2 * 4) + 2].normal = { -1.0f, 0.0f, 0.0f };
		vertices[(2 * 4) + 3].normal = { -1.0f, 0.0f, 0.0f };

		// Right face
		vertices[(3 * 4) + 0].position = { maxX, minY, maxZ };
		vertices[(3 * 4) + 1].position = { maxX, maxY, minZ };
		vertices[(3 * 4) + 2].position = { maxX, maxY, maxZ };
		vertices[(3 * 4) + 3].position = { maxX, minY, minZ };
		vertices[(3 * 4) + 0].texture = { minUvX, minUvY };
		vertices[(3 * 4) + 1].texture = { maxUvX, maxUvY };
		vertices[(3 * 4) + 2].texture = { minUvX, maxUvY };
		vertices[(3 * 4) + 3].texture = { maxUvX, minUvY };
		vertices[(3 * 4) + 0].normal = { 1.0f, 0.0f, 0.0f };
		vertices[(3 * 4) + 1].normal = { 1.0f, 0.0f, 0.0f };
		vertices[(3 * 4) + 2].normal = { 1.0f, 0.0f, 0.0f };
		vertices[(3 * 4) + 3].normal = { 1.0f, 0.0f, 0.0f };

		// Bottom face
		vertices[(4 * 4) + 0].position = { maxX, minY, maxZ };
		vertices[(4 * 4) + 1].position = { minX, minY, minZ };
		vertices[(4 * 4) + 2].position = { maxX, minY, minZ };
		vertices[(4 * 4) + 3].position = { minX, minY, maxZ };
		vertices[(4 * 4) + 0].texture = { minUvX, minUvY };
		vertices[(4 * 4) + 1].texture = { maxUvX, maxUvY };
		vertices[(4 * 4) + 2].texture = { minUvX, maxUvY };
		vertices[(4 * 4) + 3].texture = { maxUvX, minUvY };
		vertices[(4 * 4) + 0].normal = { 0.0f, -1.0f, 0.0f };
		vertices[(4 * 4) + 1].normal = { 0.0f, -1.0f, 0.0f };
		vertices[(4 * 4) + 2].normal = { 0.0f, -1.0f, 0.0f };
		vertices[(4 * 4) + 3].normal = { 0.0f, -1.0f, 0.0f };

		// Top face
		vertices[(5 * 4) + 0].position = { minX, maxY, maxZ };
		vertices[(5 * 4) + 1].position = { maxX, maxY, minZ };
		vertices[(5 * 4) + 2].position = { minX, maxY, minZ };
		vertices[(5 * 4) + 3].position = { maxX, maxY, maxZ };
		vertices[(5 * 4) + 0].texture = { minUvX, minUvY };
		vertices[(5 * 4) + 1].texture = { maxUvX, maxUvY };
		vertices[(5 * 4) + 2].texture = { minUvX, maxUvY };
		vertices[(5 * 4) + 3].texture = { maxUvX, minUvY };
		vertices[(5 * 4) + 0].normal = { 0.0f, 1.0f, 0.0f };
		vertices[(5 * 4) + 1].normal = { 0.0f, 1.0f, 0.0f };
		vertices[(5 * 4) + 2].normal = { 0.0f, 1.0f, 0.0f };
		vertices[(5 * 4) + 3].normal = { 0.0f, 1.0f, 0.0f };

		Memory.Copy(config.vertices, vertices, static_cast<u64>(config.vertexSize) * config.vertexCount);

		for (u32 i = 0; i < 6; i++)
		{
			const u32 vOffset = i * 4;
			const u32 iOffset = i * 6;

			static_cast<u32*>(config.indices)[iOffset + 0] = vOffset + 0;
			static_cast<u32*>(config.indices)[iOffset + 1] = vOffset + 1;
			static_cast<u32*>(config.indices)[iOffset + 2] = vOffset + 2;
			static_cast<u32*>(config.indices)[iOffset + 3] = vOffset + 0;
			static_cast<u32*>(config.indices)[iOffset + 4] = vOffset + 3;
			static_cast<u32*>(config.indices)[iOffset + 5] = vOffset + 1;
		}

		if (!name.empty())
		{
			StringNCopy(config.name, name.data(), GEOMETRY_NAME_MAX_LENGTH);
		}
		else
		{
			StringNCopy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
		}

		if (!materialName.empty())
		{
			StringNCopy(config.materialName, materialName.data(), MATERIAL_NAME_MAX_LENGTH);
		}
		else
		{
			StringNCopy(config.materialName, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
		}

		//GenerateTangents(config.vertexCount, config.vertices, config.indexCount, config.indices);
		return config;
	}

	bool GeometrySystem::CreateGeometry(const GeometryConfig& config, Geometry* g) const
	{
		// Send the geometry off to the renderer to be uploaded to the gpu
		if (!Renderer.CreateGeometry(g, config.vertexSize, config.vertexCount, config.vertices, config.indexSize, config.indexCount, config.indices))
		{
			m_registeredGeometries[g->id].referenceCount = 0;
			m_registeredGeometries[g->id].autoRelease = false;
			g->id = INVALID_ID;
			g->generation = INVALID_ID;
			g->internalId = INVALID_ID;

			return false;
		}

		// Acquire the material
		if (StringLength(config.materialName) > 0)
		{
			g->material = Materials.Acquire(config.materialName);
			if (!g->material)
			{
				g->material = Materials.GetDefault();
			}
		}

		return true;
	}

	void GeometrySystem::DestroyGeometry(Geometry* g)
	{
		Renderer.DestroyGeometry(g);
		g->internalId = INVALID_ID;
		g->generation = INVALID_ID;
		g->id = INVALID_ID;

		StringEmpty(g->name);

		// Release the material
		if (g->material && StringLength(g->material->name) > 0)
		{
			Materials.Release(g->material->name);
			g->material = nullptr;
		}
	}

	bool GeometrySystem::CreateDefaultGeometries()
	{
		// Create default geometry
		constexpr u32 vertexCount = 4;
		Vertex3D vertices[vertexCount];
		Memory.Zero(vertices, sizeof(Vertex3D) * vertexCount);

		constexpr f32 f = 10.0f;

		vertices[0].position.x = -0.5f * f;
		vertices[0].position.y = -0.5f * f;
		vertices[0].texture.x = 0.0f;
		vertices[0].texture.y = 0.0f;

		vertices[1].position.x = 0.5f * f;
		vertices[1].position.y = 0.5f * f;
		vertices[1].texture.x = 1.0f;
		vertices[1].texture.y = 1.0f;

		vertices[2].position.x = -0.5f * f;
		vertices[2].position.y = 0.5f * f;
		vertices[2].texture.x = 0.0f;
		vertices[2].texture.y = 1.0f;

		vertices[3].position.x = 0.5f * f;
		vertices[3].position.y = -0.5f * f;
		vertices[3].texture.x = 1.0f;
		vertices[3].texture.y = 0.0f;

		constexpr u32 indexCount = 6;
		const u32 indices[indexCount] = { 0, 1, 2, 0, 3, 1 };

		m_defaultGeometry.internalId = INVALID_ID;
		if (!Renderer.CreateGeometry(&m_defaultGeometry, sizeof(Vertex3D), vertexCount, vertices, sizeof(u32), indexCount, indices))
		{
			m_logger.Fatal("Failed to create default geometry");
			return false;
		}

		// Acquire the default material
		m_defaultGeometry.material = Materials.GetDefault();

		// Create default 2d geometry
		Vertex2D vertices2d[vertexCount];
		Memory.Zero(vertices2d, sizeof(Vertex2D) * 4);

		vertices2d[0].position.x = -0.5f * f;
		vertices2d[0].position.y = -0.5f * f;
		vertices2d[0].texture.x = 0.0f;
		vertices2d[0].texture.y = 0.0f;

		vertices2d[1].position.x = 0.5f * f;
		vertices2d[1].position.y = 0.5f * f;
		vertices2d[1].texture.x = 1.0f;
		vertices2d[1].texture.y = 1.0f;

		vertices2d[2].position.x = -0.5f * f;
		vertices2d[2].position.y = 0.5f * f;
		vertices2d[2].texture.x = 0.0f;
		vertices2d[2].texture.y = 1.0f;

		vertices2d[3].position.x = 0.5f * f;
		vertices2d[3].position.y = -0.5f * f;
		vertices2d[3].texture.x = 1.0f;
		vertices2d[3].texture.y = 0.0f;

		// Indices (NOTE: counter-clockwise)
		const u32 indices2d[indexCount] = { 2, 1, 0, 3, 0, 1 };

		m_default2DGeometry.internalId = INVALID_ID;
		if (!Renderer.CreateGeometry(&m_default2DGeometry, sizeof(Vertex2D), vertexCount, vertices2d, sizeof(u32), indexCount, indices2d))
		{
			m_logger.Fatal("Failed to create default 2d geometry");
			return false;
		}

		m_default2DGeometry.material = Materials.GetDefault();

		return true;
	}
}
