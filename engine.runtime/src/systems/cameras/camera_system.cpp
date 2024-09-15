
#include "camera_system.h"

#include "cson/cson_types.h"
#include "memory/global_memory_system.h"
#include "string/string_utils.h"

namespace C3D
{
    bool CameraSystem::OnInit(const CSONObject& config)
    {
        INFO_LOG("Initializing.");

        // Parse the user provided config
        for (const auto& prop : config.properties)
        {
            if (prop.name.IEquals("maxCameras"))
            {
                m_config.maxCameras = prop.GetI64();
            }
        }

        if (m_config.maxCameras == 0)
        {
            ERROR_LOG("config.maxCameraCount must be > 0.");
            return false;
        }

        m_cameraMap.Create();

        return true;
    }

    void CameraSystem::OnShutdown()
    {
        INFO_LOG("Destroying all registered cameras.");
        m_cameraMap.Destroy();
    }

    Camera* CameraSystem::Acquire(const char* name)
    {
        if (StringUtils::IEquals(name, DEFAULT_CAMERA_NAME))
        {
            return &m_defaultCamera;
        }

        if (m_cameraMap.Has(name))
        {
            // We already have this camera so we increment the reference count and return a pointer
            auto& ref = m_cameraMap.Get(name);
            ref.referenceCount++;
            return &ref.camera;
        }

        // This is a new camera
        CameraReference ref;
        m_cameraMap.Set(name, ref);

        return &m_cameraMap.Get(name).camera;
    }

    void CameraSystem::Release(const char* name)
    {
        if (StringUtils::IEquals(name, DEFAULT_CAMERA_NAME))
        {
            WARN_LOG("Tried to release default camera. Nothing was done.");
            return;
        }

        if (!m_cameraMap.Has(name))
        {
            WARN_LOG("Tried to release camera: '{}' which is registered in the Camera System.", name);
            return;
        }

        auto& ref = m_cameraMap.Get(name);
        // Decrement the reference count
        ref.referenceCount--;
        // If we've reached zero references we delete the camera
        if (ref.referenceCount <= 0)
        {
            INFO_LOG("Camera: '{}' has been deleted since there are zero references to it left.", name);
            m_cameraMap.Delete(name);
        }
    }

    Camera* CameraSystem::GetDefault() { return &m_defaultCamera; }
}  // namespace C3D
