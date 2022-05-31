
#include "geometry_system.h"

#include "core/c3d_string.h"
#include "core/logger.h"
#include "core/memory.h"

#include "systems/material_system.h"

#include "services/services.h"

namespace C3D
{
	GeometrySystem::GeometrySystem()
		: m_initialized(false), m_config(), m_defaultGeometry(), m_registeredGeometries(nullptr)
	{
	}

	bool GeometrySystem::Init(const GeometrySystemConfig& config)
	{
		m_config = config;

		m_registeredGeometries = Memory::Allocate<GeometryReference>(m_config.maxGeometryCount, MemoryType::Geometry);

		const u32 count = m_config.maxGeometryCount;
		for (u32 i = 0; i < count; i++)
		{
			m_registeredGeometries[i].geometry.id = INVALID_ID;
			m_registeredGeometries[i].geometry.internalId = INVALID_ID;
			m_registeredGeometries[i].geometry.generation = INVALID_ID;
		}

		if (!CreateDefaultGeometry())
		{
			Logger::PrefixError("GEOMETRY_SYSTEM", "Failed to create default geometry");
			return false;
		}

		m_initialized = true;
		return true;
	}

	void GeometrySystem::Shutdown() const
	{
		Memory::Free(m_registeredGeometries, sizeof(GeometryReference) * m_config.maxGeometryCount, MemoryType::Geometry);
	}

	Geometry* GeometrySystem::AcquireById(const u32 id) const
	{
		if (id != INVALID_ID && m_registeredGeometries[id].geometry.id != INVALID_ID)
		{
			m_registeredGeometries->referenceCount++;
			return &m_registeredGeometries[id].geometry;
		}

		// NOTE: possible should return the default geometry instead
		Logger::PrefixError("GEOMETRY_SYSTEM", "AcquireById() cannot load invalid geometry id. Returning nullptr");
		return nullptr;
	}

	Geometry* GeometrySystem::AcquireFromConfig(const GeometryConfig& config, bool autoRelease)
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
			Logger::PrefixError("GEOMETRY_SYSTEM", "Unable to obtain free slot for geometry. Adjust config to allow for more space. Returning nullptr");
			return nullptr;
		}

		if (!CreateGeometry(config, g))
		{
			Logger::PrefixError("GEOMETRY_SYSTEM", "Failed to create geometry. Returning nullptr");
			return nullptr;
		}

		return g;
	}

	void GeometrySystem::Release(const Geometry* geometry)
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
				Logger::PrefixFatal("GEOMETRY_SYSTEM", "Geometry id mismatch. Check registration logic as this should never occur!");
			}
			return;
		}

		Logger::PrefixWarn("GEOMETRY_SYSTEM", "Release() called with invalid geometry id. Noting was done");
	}

	Geometry* GeometrySystem::GetDefault()
	{
		if (!m_initialized)
		{
			Logger::PrefixFatal("GEOMETRY_SYSTEM", "GetDefault() called before system was initialized");
			return nullptr;
		}

		return &m_defaultGeometry;
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
		config.vertexCount = xSegmentCount * ySegmentCount * 4; // 4 vertices per segment
		config.vertices = Memory::Allocate<Vertex3D>(config.vertexCount, MemoryType::Array);
		config.indexCount = xSegmentCount * ySegmentCount * 6; // 6 indices per segment
		config.indices = Memory::Allocate<u32>(config.indexCount, MemoryType::Array);

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

				Vertex3D* v0 = &config.vertices[vOffset + 0];
				Vertex3D* v1 = &config.vertices[vOffset + 1];
				Vertex3D* v2 = &config.vertices[vOffset + 2];
				Vertex3D* v3 = &config.vertices[vOffset + 3];

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
				config.indices[iOffset + 0] = vOffset + 0;
				config.indices[iOffset + 1] = vOffset + 1;
				config.indices[iOffset + 2] = vOffset + 2;
				config.indices[iOffset + 3] = vOffset + 0;
				config.indices[iOffset + 4] = vOffset + 3;
				config.indices[iOffset + 5] = vOffset + 1;
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

	bool GeometrySystem::CreateGeometry(const GeometryConfig& config, Geometry* g) const
	{
		// Send the geometry off to the renderer to be uploaded to the gpu
		if (!Renderer.CreateGeometry(g, config.vertexCount, config.vertices, config.indexCount, config.indices))
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

	bool GeometrySystem::CreateDefaultGeometry()
	{
		constexpr u32 vertexCount = 4;
		Vertex3D vertices[vertexCount];
		Memory::Zero(vertices, sizeof(Vertex3D) * vertexCount);

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

		if (!Renderer.CreateGeometry(&m_defaultGeometry, vertexCount, vertices, indexCount, indices))
		{
			Logger::PrefixFatal("GEOMETRY_SYSTEM", "Failed to create default geometry");
			return false;
		}

		// Acquire the default material
		m_defaultGeometry.material = Materials.GetDefault();

		return true;
	}
}
