
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

namespace C3D
{
	struct RenderTarget;

	struct RenderPassConfig
	{
		const char* name;
		const char* previousName;
		const char* nextName;

		vec4 renderArea;
		vec4 clearColor;

		u8 clearFlags;
	};

	enum RenderPassClearFlags : u8
	{
		ClearNone = 0x0,
		ClearColor = 0x1,
		ClearDepth = 0x2,
		ClearStencil = 0x4
	};

	class RenderPass
	{
	public:
		RenderPass()
			: id(INVALID_ID_U16), renderArea(0), renderTargetCount(0), targets(nullptr),
			  m_hasPrevPass(false), m_hasNextPass(false), m_clearFlags(0), m_clearColor(0)
		{}

		explicit RenderPass(const u16 id, const RenderPassConfig& config)
			: id(id), renderArea(config.renderArea), renderTargetCount(0), targets(nullptr),
			  m_hasPrevPass(config.previousName != nullptr), m_hasNextPass(config.nextName != nullptr),
		      m_clearFlags(config.clearFlags), m_clearColor(config.clearColor)
		{}

		virtual void Create(f32 depth, u32 stencil) = 0;
		virtual void Destroy() = 0;

		u16 id;

		ivec4 renderArea;
		u8 renderTargetCount;
		RenderTarget* targets;

	protected:
		bool m_hasPrevPass, m_hasNextPass;

		u8 m_clearFlags;
		vec4 m_clearColor;
	};
}