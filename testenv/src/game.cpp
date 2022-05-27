
#include "game.h"

#include <core/memory.h>
#include <services/services.h>

#include "core/logger.h"

TestEnv::TestEnv(const C3D::ApplicationConfig& config)
	: Application(config) {}

void TestEnv::OnUpdate(f64 deltaTime)
{
	static u64 allocCount = 0;
	const u64 prevAllocCount = allocCount;
	allocCount = C3D::Memory::GetAllocCount();


	if (INPUT.IsKeyUp(C3D::KeyM) && INPUT.WasKeyDown('m'))
	{
		C3D::Logger::Debug("Allocations: {} of which {} happened this frame", allocCount, allocCount - prevAllocCount);
	}
}

void TestEnv::OnRender(f64 deltaTime)
{
}
