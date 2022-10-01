
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

#include "vulkan_command_buffer.h"
#include "renderer/renderpass.h"

namespace C3D
{
	struct VulkanContext;

	enum class VulkanRenderPassState : u8
	{
		Ready, Recording, InRenderPass, RecordingEnded, Submitted, NotAllocated
	};

	constexpr auto VULKAN_MAX_REGISTERED_RENDER_PASSES = 64;

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass();

		VulkanRenderPass(VulkanContext* context, u16 _id, const RenderPassConfig& config);

		void Create(f32 depth, u32 stencil) override;

		void Destroy() override;

		void Begin(VulkanCommandBuffer* commandBuffer, const RenderTarget* target) const;

		static void End(VulkanCommandBuffer* commandBuffer);

		VkRenderPass handle;
		VulkanRenderPassState state;

	private:
		f32 m_depth;
		u32 m_stencil;

		VulkanContext* m_context;
	};
}
