
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "renderer/render_buffer.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"

namespace C3D
{
	struct FontData;

	enum class UITextType
	{
		Bitmap,
		System,
	};

	class C3D_API UIText
	{
	public:
		UIText();

		UIText(const UIText&) = delete;
		UIText(UIText&&) = delete;

		UIText& operator=(const UIText&) = delete;
		UIText& operator=(UIText&&) = delete;

		~UIText();

		bool Create(UITextType fontType, const char* fontName, u16 fontSize, const char* textContent);
		void Destroy();

		void SetPosition(const vec3& pos);
		void SetText(const char* text);

		void Draw() const;

		u32 uniqueId;

		UITextType type;
		FontData* data;

		u32 instanceId;
		u64 frameNumber;

		Transform transform;

	private:
		void RegenerateGeometry();

		LoggerInstance<16> m_logger;

		RenderBuffer* m_vertexBuffer;
		RenderBuffer* m_indexBuffer;

		DynamicArray<Vertex2D> m_vertexData;
		DynamicArray<u32> m_indexData;

		String m_text;
	};
}
