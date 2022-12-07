
#include "game.h"

#include <core/logger.h>
#include <core/metrics/metrics.h>

#include <services/services.h>
#include <systems/camera_system.h>

#include <glm/gtx/matrix_decompose.hpp>

#include <core/input.h>
#include <core/events/event.h>
#include <core/events/event_context.h>
#include <renderer/renderer_types.h>

TestEnv::TestEnv(const C3D::ApplicationConfig& config)
	: Application(config), m_camera(nullptr)
{}

void TestEnv::OnCreate()
{
	m_camera = Cam.GetDefault();
	m_camera->SetPosition({ 10.5f, 5.0f, 9.5f });
}

void TestEnv::OnUpdate(const f64 deltaTime)
{
	static u64 allocCount = 0;
	const u64 prevAllocCount = allocCount;
	allocCount = Metrics.GetAllocCount();

	if (Input.IsKeyUp(C3D::KeyM) && Input.WasKeyDown('m'))
	{
		C3D::Logger::Debug("Allocations: {} of which {} happened this frame", allocCount, allocCount - prevAllocCount);
		Metrics.PrintMemoryUsage();
	}

	if (Input.IsKeyUp('p') && Input.WasKeyDown('p'))
	{
		const auto pos = m_camera->GetPosition();
		C3D::Logger::Debug("Position({:.2f}, {:.2f}, {:.2f})", pos.x, pos.y, pos.z);
	}

	// Renderer Debug functions
	if (Input.IsKeyUp('1') && Input.WasKeyDown('1'))
	{
		C3D::EventContext context = {};
		context.data.i32[0] = C3D::RendererViewMode::Default;
		Event.Fire(C3D::SystemEventCode::SetRenderMode, this, context);
	}

	if (Input.IsKeyUp('2') && Input.WasKeyDown('2'))
	{
		C3D::EventContext context = {};
		context.data.i32[0] = C3D::RendererViewMode::Lighting;
		Event.Fire(C3D::SystemEventCode::SetRenderMode, this, context);
	}

	if (Input.IsKeyUp('3') && Input.WasKeyDown('3'))
	{
		C3D::EventContext context = {};
		context.data.i32[0] = C3D::RendererViewMode::Normals;
		Event.Fire(C3D::SystemEventCode::SetRenderMode, this, context);
	}

	if (Input.IsKeyDown('a') || Input.IsKeyDown(C3D::KeyLeft))
	{
		m_camera->AddYaw(1.0 * deltaTime);
	}

	if (Input.IsKeyDown('d') || Input.IsKeyDown(C3D::KeyRight))
	{
		m_camera->AddYaw(-1.0 * deltaTime);
	}

	if (Input.IsKeyDown(C3D::KeyUp))
	{
		m_camera->AddPitch(1.0 * deltaTime);
	}

	if (Input.IsKeyDown(C3D::KeyDown))
	{
		m_camera->AddPitch(-1.0 * deltaTime);
	}

	static constexpr f64 temp_move_speed = 50.0;

	auto velocity = vec3(0.0f);

	if (Input.IsKeyDown('w'))
	{
		m_camera->MoveForward(temp_move_speed * deltaTime);
	}

	// TEMP
	if (Input.IsKeyUp('t') && Input.WasKeyDown('t'))
	{
		C3D::Logger::Debug("Swapping Texture");
		C3D::EventContext context = {};
		Event.Fire(C3D::SystemEventCode::Debug0, this, context);
	}

	if (Input.IsKeyUp('l') && Input.WasKeyDown('l'))
	{
		C3D::EventContext context = {};
		Event.Fire(C3D::SystemEventCode::Debug1, this, context);
	}
	// TEMP END

	if (Input.IsKeyDown('s'))
	{
		m_camera->MoveBackward(temp_move_speed * deltaTime);
	}

	if (Input.IsKeyDown('q'))
	{
		m_camera->MoveLeft(temp_move_speed * deltaTime);
	}

	if (Input.IsKeyDown('e'))
	{
		m_camera->MoveRight(temp_move_speed * deltaTime);
	}

	if (Input.IsKeyDown(C3D::KeySpace))
	{
		m_camera->MoveUp(temp_move_speed * deltaTime);
	}

	if (Input.IsKeyDown(C3D::KeyX))
	{
		m_camera->MoveDown(temp_move_speed * deltaTime);
	}
}

void TestEnv::OnRender(f64 deltaTime)
{
}