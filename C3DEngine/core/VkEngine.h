
#pragma once
#include <vector>
#include <unordered_map>

#include "VkTypes.h"
#include "VkDeletionQueue.h"
#include "VkMesh.h"
#include "VkRenderObject.h"
#include "VkFrame.h"
#include "VkObjects.h"

namespace C3D
{
	constexpr auto FRAME_OVERLAP = 2;
	constexpr uint64_t ONE_SECOND_NS = 1000000000;
	constexpr int MAX_OBJECTS = 10000;

	struct Texture
	{
		AllocatedImage image;
		VkImageView view;
	};

	class VulkanEngine
	{
	public:
		void Init();

		void Cleanup();

		void Draw();

		void DrawObjects(VkCommandBuffer cmd, RenderObject* first, int count);

		void Run();

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

		bool LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule) const;

		Material* CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);

		Material* GetMaterial(const std::string& name);

		Mesh* GetMesh(const std::string& name);

		[[nodiscard]] AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;

		VmaAllocator allocator;
		DeletionQueue deletionQueue;
	private:
		void InitSyncStructures();

		void InitVulkan();

		void InitImGui();

		void InitSwapchain();

		void InitCommands();

		void InitDefaultRenderPass();

		void InitFramebuffers();

		void InitDescriptors();

		void InitPipelines();

		void LoadMeshes();

		void LoadImages();

		void InitScene();

		void UploadMesh(Mesh& mesh);

		[[nodiscard]] size_t PadUniformBufferSize(size_t originalSize) const;

		FrameData& GetCurrentFrame();

		bool m_isInitialized{ false };
		int m_frameNumber{ 0 };

		VkExtent2D m_windowExtent{ 1700, 900 };

		struct SDL_Window* m_window{ nullptr };

		VkObjects m_vkObjects;

		VkDebugUtilsMessengerEXT m_debugMessenger;

		VkSwapchainKHR m_swapChain;
		VkFormat m_swapchainImageFormat, m_depthFormat;

		FrameData m_frames[FRAME_OVERLAP];

		VkRenderPass m_renderPass;

		VkImageView m_depthImageView;
		AllocatedImage m_depthImage;

		GpuSceneData m_sceneData;
		AllocatedBuffer m_sceneParameterBuffer;

		VkDescriptorSetLayout m_globalSetLayout, m_objectSetLayout, m_singleTextureSetLayout;
		VkDescriptorPool m_descriptorPool;

		UploadContext m_uploadContext;

		std::vector<VkImage> m_swapchainImages;
		std::vector<VkImageView> m_swapchainImageViews;
		std::vector<VkFramebuffer> m_frameBuffers;

		std::vector<RenderObject> m_renderObjects;

		std::unordered_map<std::string, Material> m_materials;
		std::unordered_map<std::string, Mesh> m_meshes;
		std::unordered_map<std::string, Texture> m_textures;

		VkQueue m_graphicsQueue;
		uint32_t m_graphicsQueueFamily;
	};
}

