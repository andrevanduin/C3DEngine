
#pragma once
#include <unordered_map>

#include "system.h"
#include "containers/hash_map.h"
#include "containers/hash_table.h"
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
		MaterialReference() : referenceCount(0), handle(INVALID_ID), autoRelease(false) {}

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
		u16 renderMode;

		MaterialUniformLocations()
			: projection(INVALID_ID_U16), view(INVALID_ID_U16), ambientColor(INVALID_ID_U16), shininess(INVALID_ID_U16),
			  viewPosition(INVALID_ID_U16), diffuseColor(INVALID_ID_U16), diffuseTexture(INVALID_ID_U16), specularTexture(INVALID_ID_U16),
			  normalTexture(INVALID_ID_U16), model(INVALID_ID_U16), renderMode(INVALID_ID_U16)
		{}
	};

	struct UiUniformLocations
	{
		u16 projection;
		u16 view;
		u16 diffuseColor;
		u16 diffuseTexture;
		u16 model;

		UiUniformLocations()
			: projection(INVALID_ID_U16), view(INVALID_ID_U16), diffuseColor(INVALID_ID_U16), diffuseTexture(INVALID_ID_U16), model(INVALID_ID_U16)
		{}
	};

	class C3D_API MaterialSystem final : System<MaterialSystemConfig>
	{
	public:
		MaterialSystem();

		bool Init(const MaterialSystemConfig& config) override;

		void Shutdown() override;

		Material* Acquire(const char* name);
		Material* AcquireFromConfig(const MaterialConfig& config);

		void Release(const char* name);

		Material* GetDefault();

		bool ApplyGlobal(u32 shaderId, u64 frameNumber, const mat4* projection, const mat4* view, const vec4* ambientColor, const vec3* viewPosition, u32 renderMode) const;
		bool ApplyInstance(Material* material, bool needsUpdate) const;
		bool ApplyLocal(Material* material, const mat4* model) const;

	private:
		bool CreateDefaultMaterial();

		bool LoadMaterial(const MaterialConfig& config, Material* mat) const;

		void DestroyMaterial(Material* mat) const;

		bool m_initialized;

		Material m_defaultMaterial;
		Material* m_registeredMaterials;

		// Hashtable to map names to material-references
		HashTable<MaterialReference> m_registeredMaterialTable;

		// Known locations for the material shader
		MaterialUniformLocations m_materialLocations;
		u32 m_materialShaderId;

		// Known locations for the UI shader
		UiUniformLocations m_uiLocations;
		u32 m_uiShaderId;

	};
}
