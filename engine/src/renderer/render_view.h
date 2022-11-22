
#pragma once
#include "core/defines.h"
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/events/event_context.h"
#include "math/math_types.h"
#include "renderer/renderpass.h"

#include "resources/geometry.h"
#include "resources/skybox.h"

namespace C3D
{
	class Mesh;

	enum class RenderViewKnownType
	{
		World	= 0x01,
		UI		= 0x02,
		Skybox	= 0x03,
		/* @brief A view that simply only renders ui and world objects for the purpose of mouse picking. */
		Pick	= 0x04,
	};

	enum class RenderViewViewMatrixSource
	{
		SceneCamera		= 0x01,
		UiCamera		= 0x02,
		LightCamera		= 0x03,
	};

	enum class RenderViewProjectionMatrixSource
	{
		DefaultPerspective		= 0x01,
		DefaultOrthographic		= 0x02,
	};

	struct RenderViewConfig
	{
		// @brief The name of the view.
		String name;
		// @brief The name of the custom shader used by this view. Empty if not used.
		String customShaderName;
		// @brief The width of this view. Set to 0 for 100% width.
		u16 width;
		// @brief The height of this view. Set to 0 for 100% height.
		u16 height;
		// @brief The known type of the view. Used to associate with view logic.
		RenderViewKnownType type;
		// @brief The source of the view matrix.
		RenderViewViewMatrixSource viewMatrixSource;
		// @brief The source of the projection matrix.
		RenderViewProjectionMatrixSource projectionMatrixSource;
		// @brief The number of renderPasses used in this view.
		u8 passCount;
		// @brief The configurations for the renderPasses used in this view.
		RenderPassConfig* passes;
	};

	struct RenderViewPacket;

	class RenderView
	{
	public:
		RenderView(u16 _id, const RenderViewConfig& config);

		RenderView(const RenderView&) = delete;
		RenderView(RenderView&&) = delete;

		RenderView& operator=(const RenderView&) = delete;
		RenderView& operator=(RenderView&&) = delete;

		virtual ~RenderView() = default;

		virtual bool OnCreate() = 0;
		virtual void OnDestroy();

		virtual void OnResize(u32 width, u32 height) = 0;

		virtual bool OnBuildPacket(void* data, RenderViewPacket* outPacket) = 0;
		virtual bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const = 0;

		virtual bool RegenerateAttachmentTarget(u32 passIndex, RenderTargetAttachment* attachment);

		u16 id;
		String name;

		RenderViewKnownType type;
		DynamicArray<RenderPass*> passes;
	protected:
		bool OnRenderTargetRefreshRequired(u16 code, void* sender, EventContext context);

		u16 m_width;
		u16 m_height;

		char* m_customShaderName;

		LoggerInstance m_logger;
	};

	struct GeometryRenderData
	{
		mat4 model;
		Geometry* geometry;
		u32 uniqueId;
	};

	struct RenderViewPacket
	{
		// @brief A constant pointer to the view this packet is associated with.
		const RenderView* view;

		mat4 viewMatrix;
		mat4 projectionMatrix;
		vec3 viewPosition;
		vec4 ambientColor;

		DynamicArray<GeometryRenderData> geometries;
		// @brief The name of the custom shader to use, if not using it is nullptr.
		const char* customShaderName;
		// @brief Holds a pointer to extra data understood by both the object and consuming view.
		void* extendedData;
	};

	struct MeshPacketData
	{
		DynamicArray<Mesh*> meshes;
	};

	class UIText;

	struct UIPacketData
	{
		MeshPacketData meshData;
		// TEMP
		DynamicArray<UIText*> texts;
		// TEMP END
	};

	struct PickPacketData
	{
		MeshPacketData worldMeshData;
		u32 worldGeometryCount;

		MeshPacketData uiMeshData;
		u32 uiGeometryCount;
		// TEMP:
		DynamicArray<UIText*> texts;
		// TEMP END
	};

	struct SkyboxPacketData
	{
		Skybox* box;
	};
}