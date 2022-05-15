
#pragma once
#include "../Logger.h"
#include "../shaders/Shader.h"

#include "VkPipelineBuilder.h"
#include "Material.h"

#include <MaterialAsset.h>

namespace C3D
{
	class ShaderEffect;
	struct VulkanEngine;

	class MaterialSystem
	{
	public:
		void Init(VulkanEngine* owner);

		void Cleanup();

		void BuildDefaultTemplates();

		ShaderPass* BuildShader(VkRenderPass renderPass, const VkPipelineBuilder& builder, ShaderEffect* effect) const;

		Material* BuildMaterial(const std::string& materialName, const MaterialData& info);
		Material* GetMaterial(const std::string& materialName);

		void FillBuilders();

	private:
		struct MaterialInfoHash
		{
			size_t operator()(const MaterialData& k) const { return k.Hash(); }
		};

		VkPipelineBuilder m_forwardBuilder{};
		VkPipelineBuilder m_shadowBuilder{};

		std::unordered_map<std::string, EffectTemplate> m_templateCache;
		std::unordered_map<std::string, Material*> m_materials;
		std::unordered_map<MaterialData, Material*, MaterialInfoHash> m_materialCache;

		VulkanEngine* m_engine{ nullptr };
	};
}
