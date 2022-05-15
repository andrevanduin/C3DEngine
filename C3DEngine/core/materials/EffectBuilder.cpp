
#include "EffectBuilder.h"

#include "../VkEngine.h"
#include "../shaders/ShaderEffect.h"

namespace C3D
{
	ShaderEffect* EffectBuilder::Build(VulkanEngine* engine, const std::string& vertexShader, const std::string& fragmentShader)
	{
		const std::vector<ShaderEffect::ReflectionOverrides> overrides =
		{
			{ "sceneData", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC },
			{ "cameraData", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC }
		};

		const auto effect = new ShaderEffect();
		effect->AddStage(engine->shaderCache.GetShader("../../../../shaders" + vertexShader), VK_SHADER_STAGE_VERTEX_BIT);

		if (!fragmentShader.empty())
		{
			effect->AddStage(engine->shaderCache.GetShader("../../../../shaders" + fragmentShader), VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		effect->ReflectLayout(engine->vkObjects.device, overrides);
		return effect;
	}
}
