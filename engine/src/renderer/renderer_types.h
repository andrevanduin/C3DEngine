
#pragma once
#include "core/defines.h"

#include "math/math_types.h"
#include "resources/geometry.h"

namespace C3D
{
	constexpr auto BUILTIN_SHADER_NAME_MATERIAL = "Shader.Builtin.Material";
	constexpr auto BUILTIN_SHADER_NAME_UI = "Shader.Builtin.UI";

	enum class RendererBackendType
	{
		Vulkan,
		OpenGl,
		DirectX,
	};

	enum BuiltinRenderPass : u8
	{
		World	= 0x01,
		Ui		= 0x02,
	};

	struct GeometryRenderData
	{
		mat4 model;
		Geometry* geometry;
	};

	struct RenderPacket
	{
		f32 deltaTime;

		u32 geometryCount;
		GeometryRenderData* geometries;

		u32 uiGeometryCount;
		GeometryRenderData* uiGeometries;
	};

	struct RendererBackendState
	{
		u64 frameNumber;
	};

	enum class ShaderStage
	{
		Vertex, Geometry, Fragment, Compute
	};
}
