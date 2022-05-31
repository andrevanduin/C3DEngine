
#pragma once
#include "vulkan_types.h"
#include "vulkan_buffer.h"

#include "resources/texture.h"

#include "../renderer_backend.h"
#include "shaders/vulkan_material_shader.h"

struct SDL_Window;

namespace C3D
{
	class RendererVulkan final : public RendererBackend
	{
	public:
		RendererVulkan();
		virtual ~RendererVulkan() = default;

		bool Init(Application* application) override;

		void OnResize(u16 width, u16 height) override;

		bool BeginFrame(f32 deltaTime) override;

		void UpdateGlobalState(mat4 projection, mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode) override;

		void DrawGeometry(GeometryRenderData data) override;

		bool EndFrame(f32 deltaTime) override;

		void Shutdown() override;

		void CreateTexture(const u8* pixels, Texture* texture) override;
		bool CreateMaterial(Material* material) override;
		bool CreateGeometry(Geometry* geometry, u32 vertexCount, const Vertex3D* vertices, u32 indexCount, const u32* indices) override;

		void DestroyTexture(Texture* texture) override;
		void DestroyMaterial(Material* material) override;
		void DestroyGeometry(Geometry* geometry) override;

	private:
		void CreateCommandBuffers();
		void RegenerateFrameBuffers(const VulkanSwapChain* swapChain, VulkanRenderPass* renderPass);
		
		bool RecreateSwapChain();
		bool CreateBuffers();

		// TEMP
		void UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, const VulkanBuffer* buffer, u64 offset, u64 size, const void* data) const;
		void FreeDataRange(VulkanBuffer* buffer, u64 offset, u64 size);

		VulkanContext m_context;

		VulkanMaterialShader m_materialShader;

		VulkanBuffer m_objectVertexBuffer, m_objectIndexBuffer;

		u64 m_geometryVertexOffset;
		u64 m_geometryIndexOffset;

		// TODO: make dynamic
		VulkanGeometryData m_geometries[VULKAN_MAX_GEOMETRY_COUNT];

		VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };
	};
}