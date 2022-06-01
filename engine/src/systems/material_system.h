
#pragma once
#include <unordered_map>

#include "core/defines.h"
#include "core/logger.h"

#include "resources/material.h"
#include "resources/resource_types.h"

namespace C3D
{
	constexpr auto DEFAULT_MATERIAL_NAME = "default";

	struct MaterialSystemConfig
	{
		u32 maxMaterialCount;
	};

	struct MaterialReference
	{
		u64 referenceCount;
		u32 handle;
		bool autoRelease;
	};

	class MaterialSystem
	{
	public:
		MaterialSystem();

		bool Init(const MaterialSystemConfig& config);

		void Shutdown();

		Material* Acquire(const string& name);
		Material* AcquireFromConfig(const MaterialConfig& config);

		void Release(const string& name);

		Material* GetDefault();
	private:
		bool CreateDefaultMaterial();

		bool LoadMaterial(const MaterialConfig& config, Material* mat);

		void DestroyMaterial(Material* mat) const;

		LoggerInstance m_logger;

		bool m_initialized;

		MaterialSystemConfig m_config;

		Material m_defaultMaterial;

		Material* m_registeredMaterials;
		std::unordered_map<string, MaterialReference> m_registeredMaterialTable;
	};
}
