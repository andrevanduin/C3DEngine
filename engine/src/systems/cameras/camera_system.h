
#pragma once
#include "systems/system.h"
#include "containers/hash_table.h"
#include "renderer/camera.h"

namespace C3D
{
	constexpr const char* DEFAULT_CAMERA_NAME = "default";

	struct CameraSystemConfig
	{
		u16 maxCameraCount;
	};

	struct CameraLookup
	{
		u16 id;
		u16 referenceCount;
		Camera camera;
	};

	class C3D_API CameraSystem final : public SystemWithConfig<CameraSystemConfig>
	{
	public:
		explicit CameraSystem(const Engine* engine);

		bool Init(const CameraSystemConfig& config) override;
		void Shutdown() override;

		Camera* Acquire(const char* name);
		void Release(const char* name);

		Camera* GetDefault();

	private:
		HashTable<u16> m_cameraLookupTable;
		CameraLookup* m_cameras;

		Camera m_defaultCamera;
	};
}
