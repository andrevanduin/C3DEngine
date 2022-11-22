
#include "ui_text.h"

#include "shader.h"
#include "renderer/renderer_frontend.h"
#include "renderer/vertex.h"
#include "services/services.h"
#include "systems/font_system.h"
#include "systems/shader_system.h"
#include "core/identifier.h"

namespace C3D
{
	static constexpr u64 VERTICES_PER_QUAD = 4;
	static constexpr u64 INDICES_PER_QUAD = 6;

	UIText::UIText()
		: type(), data(nullptr), instanceId(INVALID_ID), frameNumber(INVALID_ID_U64), m_logger("UI_TEXT"), m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_text(nullptr)
	{}

	bool UIText::Create(const UITextType fontType, const char* fontName, const u16 fontSize, const char* textContent)
	{
		if (!fontName || !textContent)
		{
			m_logger.Error("Create() - Requires a valid font name and content");
			return false;
		}

		type = fontType;

		// Acquire our font and assign it's internal data
		if (!Fonts.Acquire(fontName, fontSize, this))
		{
			m_logger.Error("Create() - Unable to acquire font: '{}'.", fontName);
			return false;
		}

		m_text = textContent;
		instanceId = INVALID_ID;
		frameNumber = INVALID_ID_U64;

		// Acquire resources for font texture map
		Shader* uiShader = Shaders.Get("Shader.Builtin.UI"); // TODO: Switch this to a dedicated text shader
		TextureMap* fontMaps[1] = { &data->atlas };

		if (!Renderer.AcquireShaderInstanceResources(uiShader, fontMaps, &instanceId))
		{
			m_logger.Fatal("Create() - Unable to acquire shader resources for font texture map.");
			return false;
		}

		const auto textSize = m_text.Size();

		// Generate the vertex buffer
		static constexpr u64 quad_size = sizeof(Vertex2D) * VERTICES_PER_QUAD;

		m_vertexBuffer = Renderer.CreateRenderBuffer(RenderBufferType::Vertex, textSize * quad_size, false);
		if (!m_vertexBuffer)
		{
			m_logger.Error("Create() - Failed to create vertex buffer.");
			return false;
		}
		m_vertexBuffer->Bind(0);

		// Generate index buffer
		static constexpr u64 index_size = sizeof(u32) * INDICES_PER_QUAD;

		m_indexBuffer = Renderer.CreateRenderBuffer(RenderBufferType::Index, textSize * index_size, false);
		if (!m_indexBuffer)
		{
			m_logger.Error("Create() - Failed to create index buffer.");
			return false;
		}
		m_indexBuffer->Bind(0);

		// Verify that our atlas has all the required glyphs
		if (!Fonts.VerifyAtlas(data, m_text))
		{
			m_logger.Error("Create() - Font atlas verification failed.");
			return false;
		}

		RegenerateGeometry();

		uniqueId = Identifier::GetNewId(this);

		return true;
	}

	void UIText::Destroy()
	{
		Identifier::ReleaseId(uniqueId);

		// Destroy our string
		m_text.Destroy();

		// Destroy our temp data
		m_vertexData.Destroy();
		m_indexData.Destroy();

		// Destroy our buffers
		Renderer.DestroyRenderBuffer(m_vertexBuffer);
		Renderer.DestroyRenderBuffer(m_indexBuffer);

		// Release resources for font texture map
		Shader* uiShader = Shaders.Get("Shader.Builtin.UI"); // TODO: use a text shader
		if (!Renderer.ReleaseShaderInstanceResources(uiShader, instanceId))
		{
			m_logger.Fatal("Destroy() - Failed to release shader resources for font texture map.");
		}
	}

	void UIText::SetPosition(const vec3& pos)
	{
		transform.SetPosition(pos);
	}

	void UIText::SetText(const char* text)
	{
		// We need some valid text
		if (!text) 
			return;

		// If the new string matches we don't need to do anything
		if (m_text == text)
			return;

		m_text = text;

		// Ensure that our font atlas has all the glyphs required
		if (!Fonts.VerifyAtlas(data, m_text))
		{
			m_logger.Error("SetText() - Font atlas verification failed.");
			return;
		}

		RegenerateGeometry();
	}

	void UIText::Draw() const
	{
		// TODO: Utf8 length
		if (!m_vertexBuffer->Draw(0, static_cast<u32>(m_text.Size() * VERTICES_PER_QUAD), true))
		{
			m_logger.Error("Draw() - Failed to draw vertex buffer.");
		}

		if (!m_indexBuffer->Draw(0, static_cast<u32>(m_text.Size() * INDICES_PER_QUAD), false))
		{
			m_logger.Error("Draw() - Failed to draw index buffer.");
		}
	}

	void UIText::RegenerateGeometry()
	{
		const u64 utf8Size = m_text.SizeUtf8();
		const u64 size = m_text.Size();

		const u64 vertexCount = VERTICES_PER_QUAD * utf8Size;
		const u64 indexCount = INDICES_PER_QUAD * utf8Size;

		const u64 vertexBufferSize = sizeof(Vertex2D) * vertexCount;
		const u64 indexBufferSize = sizeof(u32) * indexCount;

		// Resize our buffers (only if the needed size is larger than what we currently have)
		if (vertexBufferSize > m_vertexBuffer->totalSize)
		{
			if (!m_vertexBuffer->Resize(vertexBufferSize))
			{
				m_logger.Error("RegenerateGeometry() - Failed to resize vertex buffer.");
				return;
			}
		}

		if (indexBufferSize > m_indexBuffer->totalSize)
		{
			if (!m_indexBuffer->Resize(indexBufferSize))
			{
				m_logger.Error("RegenerateGeometry() - Failed to resize index buffer.");
				return;
			}
		}

		// Generate new geometry for each character
		f32 x = 0, y = 0;

		// Make sure we have enough storage for our temp data
		m_vertexData.Reserve(vertexCount);
		m_indexData.Reserve(indexCount);

		u64 advance = 1;
		u32 utf8CharCount = 0;
		for (u64 index = 0; index < size; index += advance)
		{
			auto codepoint = m_text.ToCodepoint(index, advance);
			if (codepoint == '\n')
			{
				// If we find a newline we increment the y position and reset the x position
				x = 0;
				y += static_cast<f32>(data->lineHeight);
				continue;
			}

			if (codepoint == '\t')
			{
				// If we find a tab character we increment x by our tabAdvance
				x += data->tabXAdvance;
				continue;
			}

			FontGlyph glyph = {};
			auto found = false;
			for (const auto& g : data->glyphs)
			{
				if (g.codepoint == codepoint)
				{
					glyph = g;
					found = true;
					break;
				}
			}

			if (!found)
			{
				// If we can't find the glyph we look for codepoint = -1
				codepoint = -1;
				for (const auto& g : data->glyphs)
				{
					if (g.codepoint == codepoint)
					{
						glyph = g;
						found = true;
						break;
					}
				}
			}

			if (!found)
			{
				m_logger.Error("RegenerateGeometry() - Unable to find glyph. Unknown codepoint.");
				continue;
			}

			const f32 minX = x + static_cast<f32>(glyph.xOffset);
			const f32 minY = y + static_cast<f32>(glyph.yOffset);

			const f32 maxX = minX + static_cast<f32>(glyph.width);
			const f32 maxY = minY + static_cast<f32>(glyph.height);

			const auto fAtlasSizeX = static_cast<f32>(data->atlasSizeX);
			const auto fAtlasSizeY = static_cast<f32>(data->atlasSizeY);

			const f32 tMinX = static_cast<f32>(glyph.x) / fAtlasSizeX;
			f32 tMinY = static_cast<f32>(glyph.y) / fAtlasSizeY;

			const f32 tMaxX = static_cast<f32>(glyph.x + glyph.width) / fAtlasSizeX;
			f32 tMaxY = static_cast<f32>(glyph.y + glyph.height) / fAtlasSizeY;

			// Flip the y-axis for system text
			if (type == UITextType::System)
			{
				tMinY = 1.0f - tMinY;
				tMaxY = 1.0f - tMaxY;
			}

			m_vertexData.EmplaceBack(vec2(minX, minY), vec2(tMinX, tMinY));
			m_vertexData.EmplaceBack(vec2(maxX, maxY), vec2(tMaxX, tMaxY));
			m_vertexData.EmplaceBack(vec2(minX, maxY), vec2(tMinX, tMaxY));
			m_vertexData.EmplaceBack(vec2(maxX, minY), vec2(tMaxX, tMinY));

			i32 kerning = 0;

			const u64 nextIndex = index + advance;
			if (nextIndex < utf8Size)
			{
				u64 nextAdvance = 0;
				const i32 nextCodepoint = m_text.ToCodepoint(nextIndex, nextAdvance);

				for (const auto& k : data->kernings)
				{
					if (k.codepoint0 == codepoint && k.codepoint1 == nextCodepoint)
					{
						kerning = k.amount;
						break;
					}
				}
			}

			x += static_cast<f32>(glyph.xAdvance) + static_cast<f32>(kerning);

			m_indexData.EmplaceBack(utf8CharCount * 4 + 2);
			m_indexData.EmplaceBack(utf8CharCount * 4 + 1);
			m_indexData.EmplaceBack(utf8CharCount * 4 + 0);
			m_indexData.EmplaceBack(utf8CharCount * 4 + 3);
			m_indexData.EmplaceBack(utf8CharCount * 4 + 0);
			m_indexData.EmplaceBack(utf8CharCount * 4 + 1);

			utf8CharCount++;
		}

		// Load up our vertex and index buffer data
		if (!m_vertexBuffer->LoadRange(0, vertexBufferSize, m_vertexData.GetData()))
		{
			m_logger.Error("RegenerateGeometry() - Failed to LoadRange() for vertex buffer.");
		}
		if (!m_indexBuffer->LoadRange(0, indexBufferSize, m_indexData.GetData()))
		{
			m_logger.Error("RegenerateGeometry() - Failed to LoadRange() for index buffer.");
		}

		// Clear our temp data
		m_vertexData.Clear();
		m_indexData.Clear();
	}
}
