
#include "MaterialSystem.h"
#include "EffectBuilder.h"
#include "../VkEngine.h"
#include "../VkInitializers.h"

#include "../shaders/ShaderEffect.h"
#include "../shaders/DescriptorBuilder.h"

namespace C3D
{
	void MaterialSystem::Init(VulkanEngine* owner)
	{
		m_engine = owner;
		BuildDefaultTemplates();
	}

	void MaterialSystem::Cleanup() {}

	void MaterialSystem::BuildDefaultTemplates()
	{
		FillBuilders();

		const auto texturedLit = EffectBuilder::Build(m_engine, "tri_mesh_ssbo_instanced.vert.spv", "textured_lit.frag.spv");
		const auto defaultLit = EffectBuilder::Build(m_engine, "tri_mesh_ssbo_instanced.vert.spv", "default_lit.frag.spv");
		const auto opaqueShadowCast = EffectBuilder::Build(m_engine, "tri_mesh_ssbo_instanced_shadowcast.vert.spv");

		const auto texturedLitPass = BuildShader(m_engine->renderPass, m_forwardBuilder, texturedLit);
		const auto defaultLitPass = BuildShader(m_engine->renderPass, m_forwardBuilder, defaultLit);
		const auto opaqueShadowCastPass = BuildShader(m_engine->renderPass, m_shadowBuilder, opaqueShadowCast);

		{
			EffectTemplate defaultTextured;
			defaultTextured.passShaders[MeshPassType::Transparency] = nullptr;
			defaultTextured.passShaders[MeshPassType::DirectionalShadow] = opaqueShadowCastPass;
			defaultTextured.passShaders[MeshPassType::Forward] = texturedLitPass;

			defaultTextured.defaultParameters = nullptr;
			defaultTextured.transparency = Assets::TransparencyMode::Opaque;

			m_templateCache["texturedPBR_opaque"] = defaultTextured;
		}
		{
			VkPipelineBuilder transparentForward = m_forwardBuilder;

			transparentForward.colorBlendAttachment.blendEnable = VK_TRUE;
			transparentForward.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			transparentForward.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			transparentForward.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;


			//transparentForward._colorBlendAttachment.colorBlendOp = VK_BLEND_OP_OVERLAY_EXT;
			transparentForward.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

			transparentForward.depthStencil.depthWriteEnable = false;

			transparentForward.rasterizer.cullMode = VK_CULL_MODE_NONE;
			//passes
			ShaderPass* transparentLitPass = BuildShader(m_engine->renderPass, transparentForward, texturedLit);

			EffectTemplate defaultTextured;
			defaultTextured.passShaders[MeshPassType::Transparency] = transparentLitPass;
			defaultTextured.passShaders[MeshPassType::DirectionalShadow] = nullptr;
			defaultTextured.passShaders[MeshPassType::Forward] = nullptr;

			defaultTextured.defaultParameters = nullptr;
			defaultTextured.transparency = Assets::TransparencyMode::Transparent;

			m_templateCache["texturedPBR_transparent"] = defaultTextured;
		}

		{
			EffectTemplate defaultColored;

			defaultColored.passShaders[MeshPassType::Transparency] = nullptr;
			defaultColored.passShaders[MeshPassType::DirectionalShadow] = opaqueShadowCastPass;
			defaultColored.passShaders[MeshPassType::Forward] = defaultLitPass;
			defaultColored.defaultParameters = nullptr;
			defaultColored.transparency = Assets::TransparencyMode::Opaque;

			m_templateCache["colored_opaque"] = defaultColored;
		}
	}

	ShaderPass* MaterialSystem::BuildShader(VkRenderPass renderPass, const VkPipelineBuilder& builder, ShaderEffect* effect) const
	{
		const auto pass = new ShaderPass();
		pass->effect = effect;
		pass->layout = effect->builtLayout;

		auto pipelineBuilder = builder;
		pipelineBuilder.SetShaders(effect);

		pass->pipeline = pipelineBuilder.Build(m_engine->vkObjects.device, renderPass);
		return pass;
	}

	Material* MaterialSystem::BuildMaterial(const std::string& materialName, const MaterialData& info)
	{
		Material* material;
		if (const auto it = m_materialCache.find(info); it != m_materialCache.end())
		{
			material = (*it).second;
			m_materials[materialName] = material;
		}
		else
		{
			material = new Material();
			material->original = &m_templateCache[info.baseTemplate];
			material->parameters = info.parameters;

			material->passSets[MeshPassType::DirectionalShadow] = VK_NULL_HANDLE;
			material->textures = info.textures;

			auto descriptorBuilder = DescriptorBuilder::Begin(m_engine->descriptorLayoutCache, m_engine->descriptorAllocator);

			for (int i = 0; i < info.textures.size(); i++)
			{
				VkDescriptorImageInfo imageBufferInfo{};
				imageBufferInfo.sampler = info.textures[i].sampler;
				imageBufferInfo.imageView = info.textures[i].view;
				imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descriptorBuilder.BindImage(i, &imageBufferInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
			}

			descriptorBuilder.Build(material->passSets[MeshPassType::Forward]);
			descriptorBuilder.Build(material->passSets[MeshPassType::Transparency]);

			m_materialCache[info] = material;
			m_materials[materialName] = material;
		}

		return material;
	}

	Material* MaterialSystem::GetMaterial(const std::string& materialName)
	{
		if (const auto it = m_materials.find(materialName); it != m_materials.end())
		{
			return (*it).second;
		}
		Logger::Warn("Material with name {} could not be found!", materialName);
		return nullptr;
	}

	void MaterialSystem::FillBuilders()
	{
		m_shadowBuilder.vertexDescription = Vertex::GetVertexDescription();

		m_shadowBuilder.inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		m_shadowBuilder.rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		m_shadowBuilder.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
		m_shadowBuilder.rasterizer.depthBiasEnable = VK_TRUE;

		m_shadowBuilder.multiSampling = VkInit::MultiSamplingStateCreateInfo();
		m_shadowBuilder.colorBlendAttachment = VkInit::ColorBlendAttachmentState();

		m_shadowBuilder.depthStencil = VkInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS);

		m_forwardBuilder.vertexDescription = Vertex::GetVertexDescription();

		m_forwardBuilder.inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		m_forwardBuilder.rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		m_forwardBuilder.rasterizer.cullMode = VK_CULL_MODE_NONE;

		m_forwardBuilder.multiSampling = VkInit::MultiSamplingStateCreateInfo();
		m_forwardBuilder.colorBlendAttachment = VkInit::ColorBlendAttachmentState();

		m_forwardBuilder.depthStencil = VkInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL);
	}
}
