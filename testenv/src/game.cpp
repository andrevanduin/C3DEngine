
#include "game.h"

#include <core/memory.h>
#include "core/logger.h"

#include <services/services.h>

#include <glm/gtx/matrix_decompose.hpp>

#include "math/c3d_math.h"

TestEnv::TestEnv(const C3D::ApplicationConfig& config)
	: Application(config), m_view(), m_cameraPosition(), m_cameraEuler(), m_cameraViewDirty(true)
{
}

void TestEnv::OnCreate()
{
	m_cameraPosition = vec3(0, 0, 30.0f);
	m_cameraEuler = vec3(0.0f);
}

void TestEnv::OnUpdate(const f64 deltaTime)
{
	static u64 allocCount = 0;
	const u64 prevAllocCount = allocCount;
	allocCount = C3D::Memory::GetAllocCount();


	if (Input.IsKeyUp(C3D::KeyM) && Input.WasKeyDown('m'))
	{
		C3D::Logger::Debug("Allocations: {} of which {} happened this frame", allocCount, allocCount - prevAllocCount);
	}

	// HACK: temp to move camera around
	if (Input.IsKeyDown('a') || Input.IsKeyDown(C3D::KeyLeft))
	{
		CameraYaw(static_cast<f32>(1.0 * deltaTime));
	}

	if (Input.IsKeyDown('d') || Input.IsKeyDown(C3D::KeyRight))
	{
		CameraYaw(static_cast<f32>(-1.0 * deltaTime));
	}

	if (Input.IsKeyDown(C3D::KeyUp))
	{
		CameraPitch(static_cast<f32>(1.0 * deltaTime));
	}

	if (Input.IsKeyDown(C3D::KeyDown))
	{
		CameraPitch(static_cast<f32>(-1.0 * deltaTime));
	}

	auto velocity = vec3(0.0f);

	if (Input.IsKeyDown('w'))
	{
		const vec3 forward(-m_view[0][2], -m_view[1][2], -m_view[2][2]);
		velocity += glm::normalize(forward);

	}

	// TEMP
	if (Input.IsKeyUp('t') && Input.WasKeyDown('t'))
	{
		C3D::Logger::Debug("Swapping Texture");
		C3D::EventContext context = {};
		Event.Fire(C3D::SystemEventCode::Debug0, this, context);
	}
	// TEMP END

	if (Input.IsKeyDown('s'))
	{
		const vec3 backward(m_view[0][2], m_view[1][2], m_view[2][2]);
		velocity += glm::normalize(backward);
	}

	if (Input.IsKeyDown('q'))
	{
		const vec3 left(-m_view[0][0], -m_view[1][0], -m_view[2][0]);
		velocity += glm::normalize(left);
	}

	if (Input.IsKeyDown('e'))
	{
		const vec3 right(m_view[0][0], m_view[1][0], m_view[2][0]);
		velocity += glm::normalize(right);
	}

	if (Input.IsKeyDown(C3D::KeySpace))
	{
		velocity.y += 1.0f;
	}

	if (Input.IsKeyDown(C3D::KeyX))
	{
		velocity.y -= 1.0f;
	}

	if (vec3(0.0f) != velocity)
	{
		f32 tempMoveSpeed = 50.0f;
		velocity = glm::normalize(velocity);

		float speed = tempMoveSpeed * static_cast<f32>(deltaTime);

		m_cameraPosition += (speed * velocity);
		m_cameraViewDirty = true;
	}

	RecalculateViewMatrix();

	// HACK: This is just for testing
	Render.SetView(m_view);
}

void TestEnv::OnRender(f64 deltaTime)
{
}

void TestEnv::RecalculateViewMatrix()
{
	if (!m_cameraViewDirty) return;

	const mat4 rotation = glm::eulerAngleXYZ(m_cameraEuler.x, m_cameraEuler.y, m_cameraEuler.z);
	const mat4 translation = translate(m_cameraPosition);
	m_view = inverse(rotation * translation);

	m_cameraViewDirty = false;
}

void TestEnv::CameraPitch(const f32 amount)
{
	m_cameraEuler.x += amount;

	// Clamp to avoid gimball lock
	constexpr f32 limit = 89.0f * C3D::DEG_2_RAD_MULTIPLIER;
	m_cameraEuler.x = C3D_CLAMP(m_cameraEuler.x, -limit, limit);

	m_cameraViewDirty = true;
}

void TestEnv::CameraYaw(const f32 amount)
{
	m_cameraEuler.y += amount;
	m_cameraViewDirty = true;
}


