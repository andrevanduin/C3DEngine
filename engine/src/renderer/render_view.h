
#pragma once
#include "core/defines.h"
#include "containers/dynamic_array.h"
#include "math/math_types.h"
#include "renderer/renderpass.h"

#include "resources/geometry.h"
#include "resources/mesh.h"

namespace C3D
{
	enum class RenderViewKnownType
	{
		World	= 0x01,
		UI		= 0x02,
	};

	enum class RenderViewViewMatrixSource
	{
		SceneCamera		= 0x01,
		UiCamera		= 0x02,
		LightCamera		= 0x03,
	};

	enum class RenderViewProjectionMatrixSource
	{
		DefaultPerpective		= 0x01,
		DefualtOrthographic		= 0x02,
	};

	struct RenderViewPassConfig
	{
		const char* name;
	};

	struct RenderViewConfig
	{
		// @brief The name of the view.
		const char* name;
		// @brief The name of the custom shader used by this view. Nullptr if not used.
		const char* customShaderName;
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
		RenderViewPassConfig* passes;
	};

	struct RenderViewPacket;

	class RenderView
	{
	public:
		explicit RenderView(const u16 id, const RenderViewConfig& config)
			: id(id), name(nullptr), type(), m_width(config.width), m_height(config.height), m_customShaderName(nullptr), m_logger(config.name)
		{}

		virtual ~RenderView() = default;

		virtual bool OnCreate() = 0;
		virtual void OnDestroy() { }

		virtual void OnResize(u32 width, u32 height) = 0;

		virtual bool OnBuildPacket(void* data, RenderViewPacket* outPacket) const = 0;
		virtual bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const = 0;

		u16 id;
		const char* name;

		RenderViewKnownType type;
		DynamicArray<RenderPass*> passes;
	protected:
		u16 m_width;
		u16 m_height;

		char* m_customShaderName;

		LoggerInstance m_logger;
	};

	struct GeometryRenderData
	{
		mat4 model;
		Geometry* geometry;
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
		DynamicArray<Mesh> meshes;
	};
}