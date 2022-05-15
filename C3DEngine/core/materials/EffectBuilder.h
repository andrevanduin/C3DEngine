
#pragma once
#include <string_view>

namespace C3D
{
	struct VulkanEngine;
	class ShaderEffect;

	enum class VertexAttributeTemplate
	{
		DefaultVertex, DefaultVertexPosOnly
	};

	class EffectBuilder
	{
	public:
		static ShaderEffect* Build(VulkanEngine* engine, const std::string& vertexShader, const std::string& fragmentShader = "");
	};
}