
#pragma once
#include "math/math_types.h"
#include "math/plane.h"
#include "resources/mesh.h"

namespace C3D
{
	class C3D_API PrimitiveRenderer
	{
	public:
		PrimitiveRenderer();

		void OnCreate();
		
		Mesh* AddLine(const vec3& start, const vec3& end, const vec4& color);

		// Mesh* AddRect();

		// Mesh* AddCircle();

		Mesh* AddPlane(const Plane3D& plane, const vec4& color);
		Mesh* AddBox(const vec3& center, const vec3& halfExtents);

		void OnRender(LinearAllocator& frameAllocator, RenderPacket& packet);

		static void Dispose(Mesh* mesh);

	private:
		Mesh* GetMesh();

		LoggerInstance<32> m_logger;

		Array<Mesh, 512> m_meshes;
	};
}