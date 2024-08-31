
#pragma once
#include "containers/hash_map.h"
#include "renderer/camera.h"
#include "string/string.h"
#include "systems/system.h"

namespace C3D
{
    constexpr const char* DEFAULT_CAMERA_NAME = "default";

    struct CameraSystemConfig
    {
        u16 maxCameraCount;
    };

    struct CameraReference
    {
        u16 referenceCount = 1;
        Camera camera;
    };

    class C3D_API CameraSystem final : public SystemWithConfig<CameraSystemConfig>
    {
    public:
        bool OnInit(const CameraSystemConfig& config) override;
        void OnShutdown() override;

        Camera* Acquire(const char* name);
        void Release(const char* name);

        Camera* GetDefault();

    private:
        HashMap<String, CameraReference> m_cameraMap;

        Camera m_defaultCamera;
    };
}  // namespace C3D
