
#pragma once
#include <unordered_map>

#include "systems/system.h"
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
		MaterialReference(bool shouldAutoRelease) : autoRelease(shouldAutoRelease) {}

		u32 referenceCount = 1;
		bool autoRelease;
		Material material;
	};

	struct MaterialUniformLocations
	{
		u16 projection		= INVALID_ID_U16;
		u16 view			= INVALID_ID_U16;
		u16 ambientColor	= INVALID_ID_U16;
		u16 shininess		= INVALID_ID_U16;
		u16 viewPosition	= INVALID_ID_U16;
		u16 diffuseColor	= INVALID_ID_U16;
		u16 diffuseTexture	= INVALID_ID_U16;
		u16 specularTexture = INVALID_ID_U16;
		u16 normalTexture	= INVALID_ID_U16;
		u16 model			= INVALID_ID_U16;
		u16 renderMode		= INVALID_ID_U16;
	};

	struct UiUniformLocations
	{
		u16 projection		= INVALID_ID_U16;
		u16 view			= INVALID_ID_U16;
		u16 diffuseColor	= INVALID_ID_U16;
		u16 diffuseTexture	= INVALID_ID_U16;
		u16 model			= INVALID_ID_U16;
	};

	class C3D_API MaterialSystem final : public SystemWithConfig<MaterialSystemConfig>
	{
	public:
		explicit MaterialSystem(const Engine* engine);

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
		// Hashtable to map names to material-references
		HashMap<CString<256>, MaterialReference> m_registeredMaterials;

		// Known locations for the material shader
		MaterialUniformLocations m_materialLocations;
		u32 m_materialShaderId;

		// Known locations for the UI shader
		UiUniformLocations m_uiLocations;
		u32 m_uiShaderId;
	};
}
