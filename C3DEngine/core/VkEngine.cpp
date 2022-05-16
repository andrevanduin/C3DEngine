
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
#include "Utils.h"
#include "materials/VkPipelineBuilder.h"

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

		shaderCache.Init(vkObjects.device);

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
		static bool open = true;
		static float camPos[] = { 0.0f, -6.0f, 10.0f };
		static float angle = 0.0f;

		ImGui::NewFrame();
		if (open) {
			ImGui::Begin("Rens", &open);
				ImGui::SliderFloat("Angle", &angle, 0.0f, 360.0f, "%.1f");
				ImGui::SliderFloat3("Position", camPos, -100.0f, 100.0f, "%.1f");
			ImGui::End();
		}
		ImGui::Render();

		// Wait for the GPU to finish rendering the last frame
		VK_CHECK(vkWaitForFences(vkObjects.device, 1, &GetCurrentFrame().renderFence, true, ONE_SECOND_NS));
		VK_CHECK(vkResetFences(vkObjects.device, 1, &GetCurrentFrame().renderFence));

		GetCurrentFrame().deletionQueue.Cleanup();
		GetCurrentFrame().dynamicDescriptorAllocator->ResetPools();

		VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().commandBuffer, 0));

		uint32_t swapchainImageIndex;
		VK_CHECK(vkAcquireNextImageKHR(vkObjects.device, m_swapChain, ONE_SECOND_NS, GetCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

		auto cmd = GetCurrentFrame().commandBuffer;

		const auto cmdBeginInfo = VkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		VkClearValue clearValue;
		const float flash = abs(sin(static_cast<float>(m_frameNumber) / 120.f));
		clearValue.color = { { 153 / 255.0f, 1.0f, 204 / 255.0f, 1.0f } };

		{
			// Render Loop
		}

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

		/*
		VkClearValue depthClear;
		depthClear.depthStencil.depth = 1.0f;

		auto rpInfo = VkInit::RenderPassBeginInfo(renderPass, m_windowExtent, m_frameBuffers[swapchainImageIndex]);

		rpInfo.clearValueCount = 2;

		const VkClearValue clearValues[] = { clearValue, depthClear };
		rpInfo.pClearValues = &clearValues[0];

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		DrawObjects(cmd, m_renderObjects.data(), static_cast<int>(m_renderObjects.size()), view);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		*/
	}

	void VulkanEngine::DrawObjects(VkCommandBuffer cmd, RenderObject* first, const int count, glm::mat4 view)
	{
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
			const auto& object = first[i];

			if (object.material != lastMaterial)
			{
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
				lastMaterial = object.material;


				auto uniformOffset = static_cast<uint32_t>(PadUniformBufferSize(sizeof(GpuSceneData))) * frameIndex;
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

		vkWaitForFences(vkObjects.device, 1, &m_uploadContext.uploadFence, true, ONE_SECOND_NS * 9);
		vkResetFences(vkObjects.device, 1, &m_uploadContext.uploadFence);

		vkResetCommandPool(vkObjects.device, m_uploadContext.commandPool, 0);
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
		if (vkCreateShaderModule(vkObjects.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
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
		Logger::PushPrefix("VULKAN");
		Logger::Info("Init()");

		vkb::InstanceBuilder instanceBuilder;

		auto vkbInstanceResult = instanceBuilder
			.set_app_name("C3DEngine")
			.request_validation_layers(true)
			.set_debug_callback(Logger::VkDebugLog)
			.require_api_version(1, 1, 0)
			.build();

		vkb::Instance vkbInstance = vkbInstanceResult.value();

		vkObjects.instance = vkbInstance.instance;
		m_debugMessenger = vkbInstance.debug_messenger;

		if (!SDL_Vulkan_CreateSurface(m_window, vkObjects.instance, &vkObjects.surface))
		{
			Logger::Error("Failed to create Vulkan Surface");
			abort();
		}

		// Use VKBootstrap to select a GPU for us
		vkb::PhysicalDeviceSelector selector{ vkbInstance };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 1)
			.set_surface(vkObjects.surface)
			.add_required_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME)
			.select()
			.value();

		// Create the Vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };
		vkb::Device vkbDevice = deviceBuilder.build().value();

		vkObjects.device = vkbDevice.device;
		vkObjects.physicalDevice = physicalDevice.physical_device;

		m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		allocator.Init(vkObjects.device, vkObjects.physicalDevice, vkObjects.instance);

		deletionQueue.Push([&] { allocator.Cleanup(); });

		vkGetPhysicalDeviceProperties(vkObjects.physicalDevice, &vkObjects.physicalDeviceProperties);

		Logger::Info("GPU            - {}", vkObjects.physicalDeviceProperties.deviceName);
		Logger::Info("Driver Version - {}", Utils::GetGpuDriverVersion(vkObjects.physicalDeviceProperties));
		Logger::Info("API Version    - {}", Utils::GetVulkanApiVersion(vkObjects.physicalDeviceProperties));
	}

	void VulkanEngine::InitImGui()
	{
		const VkDescriptorPoolSize poolSizes[] =
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
		VK_CHECK(vkCreateDescriptorPool(vkObjects.device, &poolInfo, nullptr, &imguiPool));

		ImGui::CreateContext();

		ImGui_ImplSDL2_InitForVulkan(m_window);

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = vkObjects.instance;
		initInfo.PhysicalDevice = vkObjects.physicalDevice;
		initInfo.Device = vkObjects.device;
		initInfo.Queue = m_graphicsQueue;
		initInfo.DescriptorPool = imguiPool;
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&initInfo, renderPass);

		ImmediateSubmit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

		ImGui_ImplVulkan_DestroyFontUploadObjects();

		deletionQueue.Push([=]
		{
			vkDestroyDescriptorPool(vkObjects.device, imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
		});
	}

	void VulkanEngine::InitCommands()
	{
		Logger::Info("InitCommands()");

		const auto commandPoolInfo = VkInit::CommandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		for (auto& frame : m_frames)
		{
			VK_CHECK(vkCreateCommandPool(vkObjects.device, &commandPoolInfo, nullptr, &frame.commandPool));

			const auto cmdAllocInfo = VkInit::CommandBufferAllocateInfo(frame.commandPool);
			VK_CHECK(vkAllocateCommandBuffers(vkObjects.device, &cmdAllocInfo, &frame.commandBuffer));

			deletionQueue.Push([=] { vkDestroyCommandPool(vkObjects.device, frame.commandPool, nullptr); });
		}

		const auto uploadCommandPoolInfo = VkInit::CommandPoolCreateInfo(m_graphicsQueueFamily);
		VK_CHECK(vkCreateCommandPool(vkObjects.device, &uploadCommandPoolInfo, nullptr, &m_uploadContext.commandPool));

		deletionQueue.Push([=] { vkDestroyCommandPool(vkObjects.device, m_uploadContext.commandPool, nullptr); });

		const auto cmdAllocInfo = VkInit::CommandBufferAllocateInfo(m_uploadContext.commandPool);
		VK_CHECK(vkAllocateCommandBuffers(vkObjects.device, &cmdAllocInfo, &m_uploadContext.commandBuffer));
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

		VK_CHECK(vkCreateRenderPass(vkObjects.device, &renderPassInfo, nullptr, &renderPass));

		deletionQueue.Push([=] { vkDestroyRenderPass(vkObjects.device, renderPass, nullptr); });
	}

	void VulkanEngine::InitDescriptors()
	{
		Logger::Info("InitDescriptors()");

		descriptorAllocator = new DescriptorAllocator();
		descriptorAllocator->Init(vkObjects.device);

		descriptorLayoutCache = new DescriptorLayoutCache();
		descriptorLayoutCache->Init(vkObjects.device);

		const auto textureBind = VkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

		VkDescriptorSetLayoutCreateInfo set3Info = {};
		set3Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set3Info.pNext = nullptr;

		set3Info.bindingCount = 1;
		set3Info.flags = 0;
		set3Info.pBindings = &textureBind;

		m_singleTextureSetLayout = descriptorLayoutCache->CreateDescriptorLayout(&set3Info);

		for (auto i = 0; i < FRAME_OVERLAP; i++)
		{
			m_frames[i].dynamicDescriptorAllocator = new DescriptorAllocator();
			m_frames[i].dynamicDescriptorAllocator->Init(vkObjects.device);

			auto dynamicDataBuffer = CreateBuffer(MEGABYTE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
			m_frames[i].dynamicData.Init(allocator, dynamicDataBuffer, vkObjects.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

			m_frames[i].debugOutputBuffer = CreateBuffer(MEGABYTE * 20, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
		}
	}

	void VulkanEngine::InitPipelines()
	{
		Logger::Info("InitPipelines()");

		VkShaderModule colorMeshShader;
		if (!LoadShaderModule("../../../../shaders/default_lit.frag.spv", &colorMeshShader))
		{
			Logger::Error("Error while building triangle fragment shader module");
			abort();
		}
		else
		{
			Logger::Info("Triangle Fragment shader successfully loaded");
		}

		VkShaderModule texturedMeshShader;
		if (!LoadShaderModule("../../../../shaders/textured_lit.frag.spv", &texturedMeshShader))
		{
			Logger::Error("Error while building textured mesh fragment shader module");
			abort();
		}
		else
		{
			Logger::Info("Textured mesh fragment shader successfully loaded");
		}

		VkShaderModule meshVertShader;
		if (!LoadShaderModule("../../../../shaders/tri_mesh.vert.spv", &meshVertShader))
		{
			Logger::Error("Error while building triangle vertex module");
			abort();
		}
		else
		{
			Logger::Info("Triangle Vertex shader successfully loaded");
		}

		VkPipelineBuilder builder;
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
		VK_CHECK(vkCreatePipelineLayout(vkObjects.device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));

		auto texturedPipelineLayoutInfo = meshPipelineLayoutInfo;

		const VkDescriptorSetLayout texturedSetLayouts[] = { m_globalSetLayout, m_objectSetLayout, m_singleTextureSetLayout };

		texturedPipelineLayoutInfo.setLayoutCount = 3;
		texturedPipelineLayoutInfo.pSetLayouts = texturedSetLayouts;

		VkPipelineLayout texturedPipelineLayout;
		VK_CHECK(vkCreatePipelineLayout(vkObjects.device, &texturedPipelineLayoutInfo, nullptr, &texturedPipelineLayout));

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
		builder.multiSampling = VkInit::MultiSamplingStateCreateInfo();
		builder.colorBlendAttachment = VkInit::ColorBlendAttachmentState();
		builder.depthStencil = VkInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

		const auto vertexDescription = Vertex::GetVertexDescription();

		builder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
		builder.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size());

		builder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
		builder.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDescription.bindings.size());

		VkPipeline meshPipeline = builder.Build(vkObjects.device, renderPass);
		CreateMaterial(meshPipeline, meshPipelineLayout, "defaultMesh");

		builder.shaderStages.clear();
		builder.shaderStages.push_back(VkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
		builder.shaderStages.push_back(VkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, texturedMeshShader));

		builder.layout = texturedPipelineLayout;

		VkPipeline texPipeline = builder.Build(vkObjects.device, renderPass);
		CreateMaterial(texPipeline, texturedPipelineLayout, "texturedMesh");

		vkDestroyShaderModule(vkObjects.device, meshVertShader, nullptr);
		vkDestroyShaderModule(vkObjects.device, colorMeshShader, nullptr);
		vkDestroyShaderModule(vkObjects.device, texturedMeshShader, nullptr);

		deletionQueue.Push([=]
		{
			vkDestroyPipeline(vkObjects.device, meshPipeline, nullptr);
			vkDestroyPipeline(vkObjects.device, texPipeline, nullptr);

			vkDestroyPipelineLayout(vkObjects.device, meshPipelineLayout, nullptr);
			vkDestroyPipelineLayout(vkObjects.device, texturedPipelineLayout, nullptr);
		});
	}

	void VulkanEngine::LoadMeshes()
	{
		Logger::PushPrefix("Engine");
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
	}

	void VulkanEngine::LoadImages()
	{
		Logger::Info("LoadImages()");

		Texture lostEmpire{};
		if (!LoadImageFromFile(*this, "../../../../assets/lost_empire-RGBA.png", lostEmpire.image)) abort();

		const auto imageCreateInfo = VkInit::ImageViewCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, lostEmpire.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCreateImageView(vkObjects.device, &imageCreateInfo, nullptr, &lostEmpire.view);

		deletionQueue.Push([=] { vkDestroyImageView(vkObjects.device, lostEmpire.view, nullptr); });

		m_textures["empire_diffuse"] = lostEmpire;
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

		vkAllocateDescriptorSets(vkObjects.device, &allocInfo, &texturedMaterial->textureSet);

		const auto samplerInfo = VkInit::SamplerCreateInfo(VK_FILTER_NEAREST);

		VkSampler blockySampler;
		vkCreateSampler(vkObjects.device, &samplerInfo, nullptr, &blockySampler);

		deletionQueue.Push([=] { vkDestroySampler(vkObjects.device, blockySampler, nullptr); });

		VkDescriptorImageInfo imageBufferInfo;
		imageBufferInfo.sampler = blockySampler;
		imageBufferInfo.imageView = m_textures["empire_diffuse"].view;
		imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		const auto texture1 = VkInit::WriteDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMaterial->textureSet, &imageBufferInfo, 0);
		vkUpdateDescriptorSets(vkObjects.device, 1, &texture1, 0, nullptr);
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

		AllocatedBufferUntyped stagingBuffer{};
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
	}

	size_t VulkanEngine::PadUniformBufferSize(const size_t originalSize) const
	{
		const size_t minUboAlignment = vkObjects.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
		size_t alignedSize = originalSize;
		if (minUboAlignment > 0)
		{
			alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		return alignedSize;
	}

	AllocatedBufferUntyped VulkanEngine::CreateBuffer(const size_t allocSize, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags) const
	{
		Logger::Info("CreateBuffer()");

		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;

		createInfo.size = allocSize;
		createInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;

		AllocatedBufferUntyped buffer{};
		VK_CHECK(vmaCreateBuffer(allocator, &createInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr));

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
			VK_CHECK(vkCreateFence(vkObjects.device, &fenceCreateInfo, nullptr, &frame.renderFence));

			deletionQueue.Push([=] { vkDestroyFence(vkObjects.device, frame.renderFence, nullptr); });

			VK_CHECK(vkCreateSemaphore(vkObjects.device, &semaphoreCreateInfo, nullptr, &frame.presentSemaphore));
			VK_CHECK(vkCreateSemaphore(vkObjects.device, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore));

			deletionQueue.Push([=]
			{
				vkDestroySemaphore(vkObjects.device, frame.presentSemaphore, nullptr);
				vkDestroySemaphore(vkObjects.device, frame.renderSemaphore, nullptr);
			});
		}

		const auto uploadFenceCreateInfo = VkInit::FenceCreateInfo();
		VK_CHECK(vkCreateFence(vkObjects.device, &uploadFenceCreateInfo, nullptr, &m_uploadContext.uploadFence));

		deletionQueue.Push([=] { vkDestroyFence(vkObjects.device, m_uploadContext.uploadFence, nullptr); });
	}

	void VulkanEngine::Cleanup()
	{
		Logger::Info("Cleanup()");

		if (m_isInitialized)
		{
			for (auto& frame : m_frames) vkWaitForFences(vkObjects.device, 1, &frame.renderFence, true, ONE_SECOND_NS);

			deletionQueue.Cleanup();

			for (auto& frame : m_frames) frame.dynamicDescriptorAllocator->Cleanup();

			descriptorAllocator->Cleanup();
			descriptorLayoutCache->Cleanup();

			vkDestroySurfaceKHR(vkObjects.instance, vkObjects.surface, nullptr);

			vkDestroyDevice(vkObjects.device, nullptr);
			vkb::destroy_debug_utils_messenger(vkObjects.instance, m_debugMessenger);
			vkDestroyInstance(vkObjects.instance, nullptr);

			SDL_DestroyWindow(m_window);
		}
	}
}