
#include <fstream>
#include <iostream>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <glm/gtx/transform.hpp>

#include <VkBootstrap.h>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VkEngine.h"

#include "Logger.h"
#include "VkTextures.h"
#include "VkInitializers.h"
#include "VkPipeline.h"

#define VK_CHECK(x)															\
	do																		\
	{																		\
		VkResult err = x;													\
		if (err)															\
		{																	\
			Logger::Error("VK_CHECK failed: {}", err);						\
			abort();														\
		}																	\
	} while (0)

namespace C3D
{
	void VulkanEngine::Init()
	{
		SDL_Init(SDL_INIT_VIDEO);

		m_window = SDL_CreateWindow(
			"C3DEngine",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			static_cast<int>(m_windowExtent.width),
			static_cast<int>(m_windowExtent.height),
			SDL_WINDOW_VULKAN
		);

		if (!m_window)
		{
			std::cout << "Failed to create window: " << SDL_GetError() << std::endl;
			abort();
		}

		Logger::Init();

		InitVulkan();
		InitSwapchain();
		InitCommands();
		InitDefaultRenderPass();
		InitFramebuffers();
		InitSyncStructures();
		InitDescriptors();
		InitPipelines();

		LoadImages();
		LoadMeshes();

		InitScene();
		InitImGui();

		m_isInitialized = true;
	}

	void VulkanEngine::Draw()
	{
		ImGui::Render();

		VK_CHECK(vkWaitForFences(m_device, 1, &GetCurrentFrame().renderFence, true, ONE_SECOND_NS));
		VK_CHECK(vkResetFences(m_device, 1, &GetCurrentFrame().renderFence));

		VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().commandBuffer, 0));

		uint32_t swapchainImageIndex;
		VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapChain, ONE_SECOND_NS, GetCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

		auto cmd = GetCurrentFrame().commandBuffer;

		const auto cmdBeginInfo = VkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		VkClearValue clearValue;
		//const float flash = abs(sin(static_cast<float>(m_frameNumber) / 120.f));
		clearValue.color = { { 153 / 255.0f, 1.0f, 204 / 255.0f, 1.0f } };

		VkClearValue depthClear;
		depthClear.depthStencil.depth = 1.0f;

		auto rpInfo = VkInit::RenderPassBeginInfo(m_renderPass, m_windowExtent, m_frameBuffers[swapchainImageIndex]);

		rpInfo.clearValueCount = 2;

		const VkClearValue clearValues[] = { clearValue, depthClear };
		rpInfo.pClearValues = &clearValues[0];

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		DrawObjects(cmd, m_renderObjects.data(), static_cast<int>(m_renderObjects.size()));

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRenderPass(cmd);
		VK_CHECK(vkEndCommandBuffer(cmd));

		VkSubmitInfo submit = VkInit::SubmitInfo(&cmd);
		constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		submit.pWaitDstStageMask = &waitStage;

		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &GetCurrentFrame().presentSemaphore;

		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &GetCurrentFrame().renderSemaphore;

		VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submit, GetCurrentFrame().renderFence));

		VkPresentInfoKHR presentInfo = VkInit::PresentInfo();

		presentInfo.pSwapchains = &m_swapChain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

		m_frameNumber++;
	}

	void VulkanEngine::DrawObjects(VkCommandBuffer cmd, RenderObject* first, const int count)
	{
		const glm::vec3 camPos = { 0.0f, -6.0f, 10.0f };
		const glm::mat4 view = translate(glm::mat4{ 1.0f }, camPos);

		glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1700.0f / 900.0f, 0.1f, 200.0f);
		projection[1][1] *= -1;

		GpuCameraData camData;
		camData.projection = projection;
		camData.view = view;
		camData.viewProjection = projection * view;

		void* data;

		vmaMapMemory(allocator, GetCurrentFrame().cameraBuffer.allocation, &data);

		memcpy(data, &camData, sizeof(GpuCameraData));

		vmaUnmapMemory(allocator, GetCurrentFrame().cameraBuffer.allocation);

		const auto framed = m_frameNumber / 500.0f;
		m_sceneData.ambientColor = { 0, 0, sin(framed), 1 };

		char* sceneData;
		vmaMapMemory(allocator, m_sceneParameterBuffer.allocation, reinterpret_cast<void**>(&sceneData));

		const int frameIndex = m_frameNumber % FRAME_OVERLAP;
		sceneData += PadUniformBufferSize(sizeof(GpuSceneData)) * frameIndex;

		memcpy(sceneData, &m_sceneData, sizeof(GpuSceneData));

		vmaUnmapMemory(allocator, m_sceneParameterBuffer.allocation);

		void* objectData;
		vmaMapMemory(allocator, GetCurrentFrame().objectBuffer.allocation, &objectData);

		const auto objectSsbo = static_cast<GpuObjectData*>(objectData);
		for (int i = 0; i < count; i++)
		{
			const RenderObject& object = first[i];
			objectSsbo[i].modelMatrix = object.transformMatrix;
		}

		vmaUnmapMemory(allocator, GetCurrentFrame().objectBuffer.allocation);

		const Mesh* lastMesh = nullptr;
		const Material* lastMaterial = nullptr;

		for (int i = 0; i < count; i++)
		{
			auto& object = first[i];

			if (object.material != lastMaterial)
			{
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
				lastMaterial = object.material;

				uint32_t uniformOffset = PadUniformBufferSize(sizeof(GpuSceneData)) * frameIndex;
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &GetCurrentFrame().globalDescriptor, 1, &uniformOffset);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &GetCurrentFrame().objectDescriptor, 0, nullptr);

				if (object.material->textureSet != nullptr)
				{
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1, &object.material->textureSet, 0, nullptr);
				}
			}

			glm::mat4 model = object.transformMatrix;

			MeshPushConstants constants{};
			constants.renderMatrix = model;

			vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

			if (object.mesh != lastMesh)
			{
				VkDeviceSize offset = 0;
				vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);
				lastMesh = object.mesh;
			}

			vkCmdDraw(cmd, static_cast<uint32_t>(object.mesh->vertices.size()), 1, 0, i);
		}
	}

	void VulkanEngine::Run()
	{
		SDL_Event e;
		auto quit = false;

		while (!quit)
		{
			while (SDL_PollEvent(&e) != 0)
			{
				ImGui_ImplSDL2_ProcessEvent(&e);

				if (e.type == SDL_QUIT) quit = true;
			}

			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL2_NewFrame(m_window);

			ImGui::NewFrame();

			ImGui::ShowDemoWindow();

			Draw();
		}
	}

	void VulkanEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const
	{
		auto cmd = m_uploadContext.commandBuffer;
		const auto cmdBeginInfo = VkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		function(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		const auto submit = VkInit::SubmitInfo(&cmd);

		VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submit, m_uploadContext.uploadFence));

		vkWaitForFences(m_device, 1, &m_uploadContext.uploadFence, true, ONE_SECOND_NS * 9);
		vkResetFences(m_device, 1, &m_uploadContext.uploadFence);

		vkResetCommandPool(m_device, m_uploadContext.commandPool, 0);
	}

	bool VulkanEngine::LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule) const
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			return false;
		}

		const auto fileSize = static_cast<size_t>(file.tellg());

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
		file.close();

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;

		createInfo.codeSize = buffer.size() * sizeof(uint32_t);
		createInfo.pCode = buffer.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			return false;
		}

		*outShaderModule = shaderModule;
		return true;
	}

	Material* VulkanEngine::CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
	{
		Material mat = { };
		mat.pipeline = pipeline;
		mat.pipelineLayout = layout;

		m_materials[name] = mat;
		return &m_materials[name];
	}

	Material* VulkanEngine::GetMaterial(const std::string& name)
	{
		const auto it = m_materials.find(name);
		if (it == m_materials.end()) return nullptr;
		return &(*it).second;
	}

	Mesh* VulkanEngine::GetMesh(const std::string& name)
	{
		const auto it = m_meshes.find(name);
		if (it == m_meshes.end()) return nullptr;
		return &(*it).second;
	}

	void VulkanEngine::InitVulkan()
	{
		Logger::SetPrefix("VULKAN");
		Logger::Info("Init()");

		vkb::InstanceBuilder instanceBuilder;

		auto vkbInstanceResult = instanceBuilder
			.set_app_name("C3DEngine")
			.request_validation_layers(true)
			.use_default_debug_messenger()
			.require_api_version(1, 1, 0)
			.build();

		vkb::Instance vkbInstance = vkbInstanceResult.value();

		m_vkInstance = vkbInstance.instance;
		m_debugMessenger = vkbInstance.debug_messenger;

		if (!SDL_Vulkan_CreateSurface(m_window, m_vkInstance, &m_surface))
		{
			Logger::Error("Failed to create Vulkan Surface");
			abort();
		}

		// Use VKBootstrap to select a GPU for us
		vkb::PhysicalDeviceSelector selector{ vkbInstance };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 1)
			.set_surface(m_surface)
			.add_required_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME)
			.select()
			.value();

		// Create the Vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };
		vkb::Device vkbDevice = deviceBuilder.build().value();

		m_device = vkbDevice.device;
		m_defaultGpu = physicalDevice.physical_device;

		m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_defaultGpu;
		allocatorInfo.device = m_device;
		allocatorInfo.instance = m_vkInstance;
		vmaCreateAllocator(&allocatorInfo, &allocator);

		deletionQueue.Push([&] { vmaDestroyAllocator(allocator); });

		vkGetPhysicalDeviceProperties(m_defaultGpu, &m_defaultGpuProperties);

		Logger::Info("GPU: {}", m_defaultGpuProperties.deviceName);
		Logger::Info("Done Initializing Core");
	}

	void VulkanEngine::InitImGui()
	{
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000;
		poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
		poolInfo.pPoolSizes = poolSizes;

		VkDescriptorPool imguiPool;
		VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &imguiPool));

		ImGui::CreateContext();

		ImGui_ImplSDL2_InitForVulkan(m_window);

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = m_vkInstance;
		initInfo.PhysicalDevice = m_defaultGpu;
		initInfo.Device = m_device;
		initInfo.Queue = m_graphicsQueue;
		initInfo.DescriptorPool = imguiPool;
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&initInfo, m_renderPass);

		ImmediateSubmit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

		ImGui_ImplVulkan_DestroyFontUploadObjects();

		deletionQueue.Push([=]
			{
				vkDestroyDescriptorPool(m_device, imguiPool, nullptr);
				ImGui_ImplVulkan_Shutdown();
			});
	}

	void VulkanEngine::InitSwapchain()
	{
		Logger::Info("InitSwapchain()");

		vkb::SwapchainBuilder swapchainBuilder{ m_defaultGpu, m_device, m_surface };

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.use_default_format_selection()
			// Use Vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(m_windowExtent.width, m_windowExtent.height)
			.set_format_feature_flags(VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
			.build()
			.value();

		m_swapChain = vkbSwapchain.swapchain;
		m_swapchainImages = vkbSwapchain.get_images().value();
		m_swapchainImageViews = vkbSwapchain.get_image_views().value();

		m_swapchainImageFormat = vkbSwapchain.image_format;

		deletionQueue.Push([=] { vkDestroySwapchainKHR(m_device, m_swapChain, nullptr); });

		VkExtent3D depthImageExtent = { m_windowExtent.width, m_windowExtent.height, 1 };

		m_depthFormat = VK_FORMAT_D32_SFLOAT;

		const auto depthImgInfo = VkInit::ImageCreateInfo(m_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

		VmaAllocationCreateInfo depthImgAllocInfo = {};
		depthImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		depthImgAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vmaCreateImage(allocator, &depthImgInfo, &depthImgAllocInfo, &m_depthImage.image, &m_depthImage.allocation, nullptr);

		auto depthImgViewInfo = VkInit::ImageViewCreateInfo(m_depthFormat, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

		VK_CHECK(vkCreateImageView(m_device, &depthImgViewInfo, nullptr, &m_depthImageView));

		deletionQueue.Push([=]
		{
			vkDestroyImageView(m_device, m_depthImageView, nullptr);
			vmaDestroyImage(allocator, m_depthImage.image, m_depthImage.allocation);
		});

		Logger::Info("Finished initializing Swapchain");
	}

	void VulkanEngine::InitCommands()
	{
		Logger::Info("InitCommands()");

		const auto commandPoolInfo = VkInit::CommandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		for (auto& frame : m_frames)
		{
			VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &frame.commandPool));

			const auto cmdAllocInfo = VkInit::CommandBufferAllocateInfo(frame.commandPool);
			VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &frame.commandBuffer));

			deletionQueue.Push([=] { vkDestroyCommandPool(m_device, frame.commandPool, nullptr); });
		}

		const auto uploadCommandPoolInfo = VkInit::CommandPoolCreateInfo(m_graphicsQueueFamily);
		VK_CHECK(vkCreateCommandPool(m_device, &uploadCommandPoolInfo, nullptr, &m_uploadContext.commandPool));

		deletionQueue.Push([=] { vkDestroyCommandPool(m_device, m_uploadContext.commandPool, nullptr); });

		const auto cmdAllocInfo = VkInit::CommandBufferAllocateInfo(m_uploadContext.commandPool);
		VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_uploadContext.commandBuffer));

		Logger::Info("Finished initializing Commands");
	}

	void VulkanEngine::InitDefaultRenderPass()
	{
		Logger::Info("InitDefaultRenderPass()");

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.flags = 0;
		depthAttachment.format = m_depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPass.colorAttachmentCount = 1;
		subPass.pColorAttachments = &colorAttachmentRef;
		subPass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency depthDependency = {};
		depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depthDependency.dstSubpass = 0;
		depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.srcAccessMask = 0;
		depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		const VkSubpassDependency dependencies[2] = { dependency, depthDependency };
		const VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = &attachments[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = &dependencies[0];

		VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass));

		deletionQueue.Push([=] { vkDestroyRenderPass(m_device, m_renderPass, nullptr); });

		Logger::Info("Finished initializing DefaultRenderPass");
	}

	void VulkanEngine::InitFramebuffers()
	{
		Logger::Info("InitFramebuffers()");

		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.pNext = nullptr;

		fbInfo.renderPass = m_renderPass;
		fbInfo.attachmentCount = 1;
		fbInfo.width = m_windowExtent.width;
		fbInfo.height = m_windowExtent.height;
		fbInfo.layers = 1;

		const auto swapchainImageCount = static_cast<uint32_t>(m_swapchainImages.size());
		m_frameBuffers = std::vector<VkFramebuffer>(swapchainImageCount);

		for (uint32_t i = 0; i < swapchainImageCount; i++)
		{
			VkImageView attachments[2];
			attachments[0] = m_swapchainImageViews[i];
			attachments[1] = m_depthImageView;

			fbInfo.pAttachments = attachments;
			fbInfo.attachmentCount = 2;

			VK_CHECK(vkCreateFramebuffer(m_device, &fbInfo, nullptr, &m_frameBuffers[i]));

			deletionQueue.Push([=]
			{
				vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
				vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
			});
		}

		Logger::Info("Finished initializing Framebuffers");
	}

	void VulkanEngine::InitDescriptors()
	{
		Logger::Info("InitDescriptors()");

		const std::vector<VkDescriptorPoolSize> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
		};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = 10;
		poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
		poolInfo.pPoolSizes = sizes.data();

		vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);

		const auto cameraBind = VkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
		const auto sceneBind = VkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

		const VkDescriptorSetLayoutBinding bindings[] = { cameraBind, sceneBind };

		VkDescriptorSetLayoutCreateInfo setInfo = {};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.pNext = nullptr;

		setInfo.bindingCount = 2;
		setInfo.flags = 0;
		setInfo.pBindings = bindings;

		vkCreateDescriptorSetLayout(m_device, &setInfo, nullptr, &m_globalSetLayout);

		VkDescriptorSetLayoutBinding objectBind = VkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

		VkDescriptorSetLayoutCreateInfo set2Info = {};
		set2Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set2Info.pNext = nullptr;

		set2Info.bindingCount = 1;
		set2Info.flags = 0;
		set2Info.pBindings = &objectBind;

		vkCreateDescriptorSetLayout(m_device, &set2Info, nullptr, &m_objectSetLayout);

		const auto textureBind = VkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

		VkDescriptorSetLayoutCreateInfo set3Info = {};
		set3Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set3Info.pNext = nullptr;

		set3Info.bindingCount = 1;
		set3Info.flags = 0;
		set3Info.pBindings = &textureBind;

		vkCreateDescriptorSetLayout(m_device, &set3Info, nullptr, &m_singleTextureSetLayout);

		const size_t sceneParamBufferSize = FRAME_OVERLAP * PadUniformBufferSize(sizeof(GpuSceneData));
		m_sceneParameterBuffer = CreateBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		for (auto i = 0; i < FRAME_OVERLAP; i++)
		{
			auto& frame = m_frames[i];

			frame.cameraBuffer = CreateBuffer(sizeof(GpuCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			frame.objectBuffer = CreateBuffer(sizeof(GpuObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.pNext = nullptr;

			allocInfo.descriptorPool = m_descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &m_globalSetLayout;

			vkAllocateDescriptorSets(m_device, &allocInfo, &frame.globalDescriptor);

			VkDescriptorSetAllocateInfo objectAllocInfo = {};
			objectAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			objectAllocInfo.pNext = nullptr;

			objectAllocInfo.descriptorPool = m_descriptorPool;
			objectAllocInfo.descriptorSetCount = 1;
			objectAllocInfo.pSetLayouts = &m_objectSetLayout;

			vkAllocateDescriptorSets(m_device, &objectAllocInfo, &frame.objectDescriptor);

			VkDescriptorBufferInfo cameraInfo = {};
			cameraInfo.buffer = frame.cameraBuffer.buffer;
			cameraInfo.offset = 0;
			cameraInfo.range = sizeof(GpuCameraData);

			VkDescriptorBufferInfo sceneInfo = {};
			sceneInfo.buffer = m_sceneParameterBuffer.buffer;
			sceneInfo.offset = 0;
			sceneInfo.range = sizeof(GpuSceneData);

			VkDescriptorBufferInfo objectBufferInfo = {};
			objectBufferInfo.buffer = frame.objectBuffer.buffer;
			objectBufferInfo.offset = 0;
			objectBufferInfo.range = sizeof(GpuObjectData) * MAX_OBJECTS;

			const auto cameraWrite = VkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame.globalDescriptor, &cameraInfo, 0);
			const auto sceneWrite = VkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, frame.globalDescriptor, &sceneInfo, 1);
			const auto objectWrite = VkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.objectDescriptor, &objectBufferInfo, 0);

			const VkWriteDescriptorSet setWrites[] = { cameraWrite, sceneWrite, objectWrite };
			vkUpdateDescriptorSets(m_device, 3, setWrites, 0, nullptr);
		}

		deletionQueue.Push([&]
		{
			vmaDestroyBuffer(allocator, m_sceneParameterBuffer.buffer, m_sceneParameterBuffer.allocation);

			vkDestroyDescriptorSetLayout(m_device, m_objectSetLayout, nullptr);
			vkDestroyDescriptorSetLayout(m_device, m_globalSetLayout, nullptr);
			vkDestroyDescriptorSetLayout(m_device, m_singleTextureSetLayout, nullptr);

			vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

			for (const auto& frame : m_frames)
			{
				vmaDestroyBuffer(allocator, frame.cameraBuffer.buffer, frame.cameraBuffer.allocation);
				vmaDestroyBuffer(allocator, frame.objectBuffer.buffer, frame.objectBuffer.allocation);
			}
		});

		Logger::Info("Finished initializing Descriptors");
	}

	void VulkanEngine::InitPipelines()
	{
		Logger::Info("InitPipelines()");

		VkShaderModule colorMeshShader;
		if (!LoadShaderModule("../../../../shaders/default_lit.frag.spv", &colorMeshShader))
		{
			std::cout << "Error while building triangle fragment shader module" << std::endl;
		}
		else
		{
			std::cout << "Triangle Fragment shader successfully loaded" << std::endl;
		}

		VkShaderModule texturedMeshShader;
		if (!LoadShaderModule("../../../../shaders/textured_lit.frag.spv", &texturedMeshShader))
		{
			std::cout << "Error while building textured mesh fragment shader module" << std::endl;
		}
		else
		{
			std::cout << "Textured mesh fragment shader successfully loaded" << std::endl;
		}

		VkShaderModule meshVertShader;
		if (!LoadShaderModule("../../../../shaders/tri_mesh.vert.spv", &meshVertShader))
		{
			std::cout << "Error while building triangle vertex module" << std::endl;
		}
		else
		{
			std::cout << "Triangle Vertex shader successfully loaded" << std::endl;
		}

		PipelineBuilder builder;
		builder.shaderStages.push_back(VkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
		builder.shaderStages.push_back(VkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, colorMeshShader));

		auto meshPipelineLayoutInfo = VkInit::PipelineLayoutCreateInfo();

		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(MeshPushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
		meshPipelineLayoutInfo.pushConstantRangeCount = 1;

		const VkDescriptorSetLayout setLayouts[] = { m_globalSetLayout, m_objectSetLayout };

		meshPipelineLayoutInfo.setLayoutCount = 2;
		meshPipelineLayoutInfo.pSetLayouts = setLayouts;

		VkPipelineLayout meshPipelineLayout;
		VK_CHECK(vkCreatePipelineLayout(m_device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));

		auto texturedPipelineLayoutInfo = meshPipelineLayoutInfo;

		const VkDescriptorSetLayout texturedSetLayouts[] = { m_globalSetLayout, m_objectSetLayout, m_singleTextureSetLayout };

		texturedPipelineLayoutInfo.setLayoutCount = 3;
		texturedPipelineLayoutInfo.pSetLayouts = texturedSetLayouts;

		VkPipelineLayout texturedPipelineLayout;
		VK_CHECK(vkCreatePipelineLayout(m_device, &texturedPipelineLayoutInfo, nullptr, &texturedPipelineLayout));

		builder.layout = meshPipelineLayout;

		builder.vertexInputInfo = VkInit::VertexInputStateCreateInfo();
		builder.inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		builder.viewport.x = 0.0f;
		builder.viewport.y = 0.0f;
		builder.viewport.width = static_cast<float>(m_windowExtent.width);
		builder.viewport.height = static_cast<float>(m_windowExtent.height);
		builder.viewport.minDepth = 0.0f;
		builder.viewport.maxDepth = 1.0f;

		builder.scissor.offset = { 0, 0 };
		builder.scissor.extent = m_windowExtent;

		builder.rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		builder.multisampling = VkInit::MultisamplingStateCreateInfo();
		builder.colorBlendAttachment = VkInit::ColorBlendAttachmentState();
		builder.depthStencil = VkInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

		const auto vertexDescription = Vertex::GetVertexDescription();

		builder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
		builder.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size());

		builder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
		builder.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDescription.bindings.size());

		VkPipeline meshPipeline = builder.Build(m_device, m_renderPass);
		CreateMaterial(meshPipeline, meshPipelineLayout, "defaultMesh");

		builder.shaderStages.clear();
		builder.shaderStages.push_back(VkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
		builder.shaderStages.push_back(VkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, texturedMeshShader));

		builder.layout = texturedPipelineLayout;

		VkPipeline texPipeline = builder.Build(m_device, m_renderPass);
		CreateMaterial(texPipeline, texturedPipelineLayout, "texturedMesh");

		vkDestroyShaderModule(m_device, meshVertShader, nullptr);
		vkDestroyShaderModule(m_device, colorMeshShader, nullptr);
		vkDestroyShaderModule(m_device, texturedMeshShader, nullptr);

		deletionQueue.Push([=]
		{
			vkDestroyPipeline(m_device, meshPipeline, nullptr);
			vkDestroyPipeline(m_device, texPipeline, nullptr);

			vkDestroyPipelineLayout(m_device, meshPipelineLayout, nullptr);
			vkDestroyPipelineLayout(m_device, texturedPipelineLayout, nullptr);
		});

		Logger::Info("Finished initializing Pipelines");
	}

	void VulkanEngine::LoadMeshes()
	{
		Logger::SetPrefix("Engine");
		Logger::Info("LoadMeshes()");
		/*
		m_triangleMesh.vertices.resize(3);

		m_triangleMesh.vertices[0].position = { 1.0f, 1.0f, 0.0f };
		m_triangleMesh.vertices[1].position = { -1.0f, 1.0f, 0.0f };
		m_triangleMesh.vertices[2].position = { 0.0f, -1.0f, 0.0f };

		m_triangleMesh.vertices[0].color = { 0.0f, 1.0f, 0.0f };
		m_triangleMesh.vertices[1].color = { 0.0f, 1.0f, 0.0f };
		m_triangleMesh.vertices[2].color = { 0.0f, 1.0f, 0.0f };

		m_monkeyMesh.LoadFromObj("../../assets/monkey_smooth.obj");

		UploadMesh(m_triangleMesh);
		UploadMesh(m_monkeyMesh);

		m_meshes["monkey"] = m_monkeyMesh;
		m_meshes["triangle"] = m_triangleMesh;*/

		Mesh monkey;
		monkey.LoadFromObj("../../../../assets/monkey_smooth.obj");

		Mesh lostEmpire{};
		lostEmpire.LoadFromObj("../../../../assets/lost_empire.obj");

		UploadMesh(monkey);
		UploadMesh(lostEmpire);

		m_meshes["monkey"] = monkey;
		m_meshes["empire"] = lostEmpire;

		Logger::Info("Finished loading Meshes");
	}

	void VulkanEngine::LoadImages()
	{
		Logger::Info("LoadImages()");

		Texture lostEmpire{};
		if (!VkUtil::LoadImageFromFile(*this, "../../../../assets/lost_empire-RGBA.png", lostEmpire.image)) abort();

		const auto imageCreateInfo = VkInit::ImageViewCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, lostEmpire.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCreateImageView(m_device, &imageCreateInfo, nullptr, &lostEmpire.view);

		deletionQueue.Push([=] { vkDestroyImageView(m_device, lostEmpire.view, nullptr); });

		m_textures["empire_diffuse"] = lostEmpire;

		Logger::Info("Finished loading Images");
	}

	void VulkanEngine::InitScene()
	{
		Logger::Info("InitScene()");

		const RenderObject monkey = {
			GetMesh("monkey"),
			GetMaterial("defaultMesh"),
			glm::mat4{ 1.0f }
		};
		m_renderObjects.push_back(monkey);

		const RenderObject map = {
			GetMesh("empire"),
			GetMaterial("texturedMesh"),
			translate(glm::vec3 { 5, -10, 0 })
		};
		m_renderObjects.push_back(map);

		const auto texturedMaterial = GetMaterial("texturedMesh");

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;

		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_singleTextureSetLayout;

		vkAllocateDescriptorSets(m_device, &allocInfo, &texturedMaterial->textureSet);

		const auto samplerInfo = VkInit::SamplerCreateInfo(VK_FILTER_NEAREST);

		VkSampler blockySampler;
		vkCreateSampler(m_device, &samplerInfo, nullptr, &blockySampler);

		deletionQueue.Push([=] { vkDestroySampler(m_device, blockySampler, nullptr); });

		VkDescriptorImageInfo imageBufferInfo;
		imageBufferInfo.sampler = blockySampler;
		imageBufferInfo.imageView = m_textures["empire_diffuse"].view;
		imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		const auto texture1 = VkInit::WriteDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMaterial->textureSet, &imageBufferInfo, 0);
		vkUpdateDescriptorSets(m_device, 1, &texture1, 0, nullptr);

		Logger::Info("Finished initializing Scene");
	}

	void VulkanEngine::UploadMesh(Mesh& mesh)
	{
		Logger::Info("UploadMesh()");

		const size_t bufferSize = mesh.vertices.size() * sizeof(Vertex);

		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;

		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBuffer stagingBuffer{};
		VK_CHECK(
			vmaCreateBuffer(
				allocator, &stagingBufferInfo, &vmaAllocInfo, &stagingBuffer.buffer,
				&stagingBuffer.allocation, nullptr
			)
		);

		void* data;
		vmaMapMemory(allocator, stagingBuffer.allocation, &data);

		memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));

		vmaUnmapMemory(allocator, stagingBuffer.allocation);

		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;

		vertexBufferInfo.size = bufferSize;
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VK_CHECK(
			vmaCreateBuffer(
				allocator, &vertexBufferInfo, &vmaAllocInfo, &mesh.vertexBuffer.buffer,
				&mesh.vertexBuffer.allocation, nullptr
			)
		);

		ImmediateSubmit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copy);
			});

		deletionQueue.Push([=]
		{
			vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
		});

		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

		Logger::Info("Finished uploading Mesh");
	}

	size_t VulkanEngine::PadUniformBufferSize(const size_t originalSize) const
	{
		const size_t minUboAlignment = m_defaultGpuProperties.limits.minUniformBufferOffsetAlignment;
		size_t alignedSize = originalSize;
		if (minUboAlignment > 0)
		{
			alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		return alignedSize;
	}

	AllocatedBuffer VulkanEngine::CreateBuffer(const size_t allocSize, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) const
	{
		Logger::Info("CreateBuffer()");

		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;

		createInfo.size = allocSize;
		createInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;

		AllocatedBuffer buffer{};
		VK_CHECK(vmaCreateBuffer(allocator, &createInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr));

		Logger::Info("Created Buffer");
		return buffer;
	}

	FrameData& VulkanEngine::GetCurrentFrame()
	{
		return m_frames[m_frameNumber % FRAME_OVERLAP];
	}

	void VulkanEngine::InitSyncStructures()
	{
		Logger::Info("InitSyncStructures()");

		const auto fenceCreateInfo = VkInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		const auto semaphoreCreateInfo = VkInit::SemaphoreCreateInfo();

		for (auto& frame : m_frames)
		{
			VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &frame.renderFence));

			deletionQueue.Push([=] { vkDestroyFence(m_device, frame.renderFence, nullptr); });

			VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &frame.presentSemaphore));
			VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore));

			deletionQueue.Push([=]
				{
					vkDestroySemaphore(m_device, frame.presentSemaphore, nullptr);
					vkDestroySemaphore(m_device, frame.renderSemaphore, nullptr);
				});
		}

		const auto uploadFenceCreateInfo = VkInit::FenceCreateInfo();
		VK_CHECK(vkCreateFence(m_device, &uploadFenceCreateInfo, nullptr, &m_uploadContext.uploadFence));

		deletionQueue.Push([=] { vkDestroyFence(m_device, m_uploadContext.uploadFence, nullptr); });

		Logger::Info("Finished initializing SyncStructures");
	}

	void VulkanEngine::Cleanup()
	{
		Logger::Info("Cleanup()");

		if (m_isInitialized)
		{
			vkDeviceWaitIdle(m_device);

			deletionQueue.Cleanup();

			vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);

			vkDestroyDevice(m_device, nullptr);
			vkb::destroy_debug_utils_messenger(m_vkInstance, m_debugMessenger);
			vkDestroyInstance(m_vkInstance, nullptr);

			SDL_DestroyWindow(m_window);
		}
	}
}