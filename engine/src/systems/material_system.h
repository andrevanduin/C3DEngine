
#pragma once
#include <unordered_map>

#include "system.h"
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

	struct MaterialUniformLocations
	{
		u16 projection;
		u16 view;
		u16 ambientColor;
		u16 shininess;
		u16 viewPosition;
		u16 diffuseColor;
		u16 diffuseTexture;
		u16 specularTexture;
		u16 normalTexture;
		u16 model;
	};

	struct UiUniformLocations
	{
		u16 projection;
		u16 view;
		u16 diffuseColor;
		u16 diffuseTexture;
		u16 model;
	};

	class MaterialSystem : System<MaterialSystemConfig>
	{
	public:
		MaterialSystem();

		bool Init(const MaterialSystemConfig& config) override;

		void Shutdown() override;

		Material* Acquire(const string& name);
		Material* AcquireFromConfig(const MaterialConfig& config);

		void Release(const string& name);

		Material* GetDefault();

		bool ApplyGlobal(u32 shaderId, const mat4* projection, const mat4* view, const vec4* ambientColor, const vec3* viewPosition) const;
		bool ApplyInstance(Material* material) const;
		bool ApplyLocal(Material* material, const mat4* model) const;

	private:
		bool CreateDefaultMaterial();

		bool LoadMaterial(const MaterialConfig& config, Material* mat) const;

		void DestroyMaterial(Material* mat) const;

		bool m_initialized;

		Material m_defaultMaterial;
		Material* m_registeredMaterials;
		std::unordered_map<string, MaterialReference> m_registeredMaterialTable;

		// Known locations for the material shader
		MaterialUniformLocations m_materialLocations;
		u32 m_materialShaderId;

		// Known locations for the UI shader
		UiUniformLocations m_uiLocations;
		u32 m_uiShaderId;

	};
}
