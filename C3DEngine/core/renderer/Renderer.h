
#pragma once
#include "RenderPass.h"
#include "CullParams.h"
#include "RenderScene.h"
#include "../Allocator.h"
#include "DepthPyramid.h"

namespace C3D
{
	class Renderer
	{
	public:
		void Init(Allocator* allocator, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		void Draw();

		void ForwardPass(VkClearValue clearValue, VkCommandBuffer cmd);

		void ShadowPass(VkCommandBuffer cmd);

		void DrawObjectsForward(VkCommandBuffer cmd, MeshPass& pass);

		void ExecuteDrawCommands(VkCommandBuffer cmd, MeshPass& pass, VkDescriptorSet objectDataSet, std::vector<uint32_t> dynamicOffsets, VkDescriptorSet globalSet);

		void DrawObjectsShadow(VkCommandBuffer cmd, MeshPass& pass);

		void ReduceDepth(VkCommandBuffer cmd);

		void ExecuteComputeCull(VkCommandBuffer cmd, MeshPass& pass, CullParams& params);

		void ReadyCullData(MeshPass& pass, VkCommandBuffer cmd);

		void Cleanup();

	private:
		void CreateImageAndView(VkFormat format, VkImageUsageFlags imageUsageFlags, VkExtent3D extent, VkImageAspectFlags aspectFlags, AllocatedImage& outImage) const;

		static uint32_t GetImageMipLevels(uint32_t width, uint32_t height);

		void InitSwapChain();

		void InitFrameBuffers();

		void InitCommands();

		void InitSyncStructures();

		void InitDescriptors();

		void InitPipelines();

		void InitForwardRenderPass();

		void InitCopyRenderPass();

		void InitShadowRenderPass();

		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;

		Allocator* m_allocator{ nullptr };

		VkExtent2D m_windowExtent{ 1700, 900 };
		VkExtent2D m_shadowExtent{ 1024 * 4, 1024 * 4 };

		VkSurfaceKHR m_surface;
		VkSwapchainKHR m_swapChain;
		VkFormat m_swapchainImageFormat;

		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::vector<VkFramebuffer> m_frameBuffers;

		VkRenderPass m_renderPass;
		VkRenderPass m_shadowPass;
		VkRenderPass m_copyPass;

		AllocatedImage m_rawRenderImage;
		AllocatedImage m_depthImage;
		AllocatedImage m_shadowImage;

		VkSampler m_depthSampler;
		VkSampler m_smoothSampler;
		VkSampler m_shadowSampler;

		VkFramebuffer m_forwardFrameBuffer;
		VkFramebuffer m_shadowFrameBuffer;

		DepthPyramid m_depthPyramid;
		
		VkFormat m_renderFormat;
		VkFormat m_depthFormat;

		RenderScene m_scene;

		DeletionQueue m_deletionQueue;
	};
}