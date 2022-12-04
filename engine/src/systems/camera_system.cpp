
#include "camera_system.h"

#include "core/c3d_string.h"
#include "memory/global_memory_system.h"

namespace C3D
{
	CameraSystem::CameraSystem()
		: System("CAMERA_SYSTEM"), m_cameras(nullptr)
	{}

	bool CameraSystem::Init(const CameraSystemConfig& config)
	{
		if (config.maxCameraCount == 0)
		{
			m_logger.Error("Init() - config.maxCameraCount must be > 0");
			return false;
		}

		m_config = config;

		m_cameraLookupTable.Create(config.maxCameraCount);
		m_cameraLookupTable.Fill(INVALID_ID_U16);

		const u16 count = config.maxCameraCount;
		m_cameras = Memory.Allocate<CameraLookup>(MemoryType::RenderSystem, config.maxCameraCount);

		for (u16 i = 0; i < count; i++)
		{
			m_cameras[i].id = INVALID_ID_U16;
			m_cameras[i].referenceCount = 0;
		}

		return true;
	}

	void CameraSystem::Shutdown()
	{
		Memory.Free(MemoryType::RenderSystem, m_cameras);
		m_cameraLookupTable.Destroy();
	}

	Camera* CameraSystem::Acquire(const char* name)
	{
		if (IEquals(name, DEFAULT_CAMERA_NAME))
		{
			return &m_defaultCamera;
		}

		u16 id = m_cameraLookupTable.Get(name);
		if (id == INVALID_ID_U16)
		{
			// Find a free slot
			for (u16 i = 0; i < m_config.maxCameraCount; i++)
			{
				if (m_cameras[i].id == INVALID_ID_U16)
				{
					id = i;
					break;
				}
			}
			if (id == INVALID_ID_U16)
			{
				m_logger.Error("Acquire() - Failed to acquire new slot. Adjust camera system config to allow more (returned nullptr).");
				return nullptr;
			}

			m_logger.Trace("Acquire() - Creating new camera: '{}'", name);
			m_cameras[id].id = id;

			m_cameraLookupTable.Set(name, id);
		}

		m_cameras[id].referenceCount++;
		return &m_cameras[id].camera;
	}

	void CameraSystem::Release(const char* name)
	{
		if (IEquals(name, DEFAULT_CAMERA_NAME))
		{
			m_logger.Trace("Release() - Tried to release default camera. Nothing was done.");
			return;
		}

		if (const u16 id = m_cameraLookupTable.Get(name); id != INVALID_ID_U16)
		{
			// Decrement reference count and reset the camera if the counter reaches 0.
			m_cameras[id].referenceCount--;
			if (m_cameras[id].referenceCount == 0)
			{
				m_cameras[id].camera.Reset();
				m_cameras[id].id = INVALID_ID_U16;
				m_cameraLookupTable.Set(name, m_cameras[id].id);
			}
		}
	}

	Camera* CameraSystem::GetDefault()
	{
		return &m_defaultCamera;
	}
}
