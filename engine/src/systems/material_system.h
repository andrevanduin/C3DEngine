
#pragma once
#include <unordered_map>

#include "core/defines.h"

#include "resources/material.h"

namespace C3D
{
	constexpr auto DEFAULT_MATERIAL_NAME = "default";

	struct MaterialSystemConfig
	{
		u32 maxMaterialCount;
	};

	struct MaterialConfig
	{
		char name[MATERIAL_NAME_MAX_LENGTH];
		bool autoRelease;
		vec4 diffuseColor;
		char diffuseMapName[TEXTURE_NAME_MAX_LENGTH];
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

		static bool LoadMaterial(const MaterialConfig& config, Material* mat);

		static void DestroyMaterial(Material* mat);

		static bool LoadConfigurationFile(const string& path, MaterialConfig* outConfig);

		bool m_initialized;

		MaterialSystemConfig m_config;

		Material m_defaultMaterial;

		Material* m_registeredMaterials;
		std::unordered_map<string, MaterialReference> m_registeredMaterialTable;
	};
}