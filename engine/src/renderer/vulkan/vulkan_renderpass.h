
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

#include "vulkan_command_buffer.h"

namespace C3D
{
	struct VulkanContext;

	enum class VulkanRenderPassState : u8
	{
		Ready, Recording, InRenderPass, RecordingEnded, Submitted, NotAllocated
	};

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass();

		void Create(VulkanContext* context, const ivec4& renderArea, const vec4& clearColor, f32 depth, u32 stencil);

		void Destroy(const VulkanContext* context);

		void Begin(VulkanCommandBuffer* commandBuffer, VkFramebuffer frameBuffer) const;

		void End(VulkanCommandBuffer* commandBuffer) const;

		VkRenderPass handle;
		VulkanRenderPassState state;

		ivec4 area;
	private:
		vec4 m_clearColor;

		f32 m_depth;
		u32 m_stencil;
	};
}
