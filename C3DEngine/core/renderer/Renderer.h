
#pragma once
#include "RenderPass.h"
#include "CullParams.h"
#include "RenderScene.h"

namespace C3D
{
	class Renderer
	{
	public:
		void Init(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

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

		VkExtent2D m_windowExtent{ 1700, 900 };

		VkSurfaceKHR m_surface;
		VkSwapchainKHR m_swapChain;
		VkFormat m_swapchainImageFormat;

		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;

		VkFormat m_renderFormat;
		

		RenderScene m_scene;

		DeletionQueue m_deletionQueue;
	};
}