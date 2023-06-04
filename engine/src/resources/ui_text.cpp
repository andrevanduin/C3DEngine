
#include "ui_text.h"

#include "shader.h"
#include "renderer/renderer_frontend.h"
#include "renderer/vertex.h"
#include "systems/system_manager.h"
#include "systems/font_system.h"
#include "systems/shader_system.h"
#include "core/identifier.h"

namespace C3D
{
	static constexpr u64 VERTICES_PER_QUAD = 4;
	static constexpr u64 INDICES_PER_QUAD = 6;

	UIText::UIText()
		: uniqueId(INVALID_ID), type(), data(nullptr), instanceId(INVALID_ID), frameNumber(INVALID_ID_U64), m_logger("UI_TEXT"),
		  m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_text(nullptr)
	{
	}

	UIText::~UIText()
	{
		if (uniqueId != INVALID_ID)
		{
			Destroy();
		}
	}

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

		auto textSize = m_text.SizeUtf8();
		// Ensure that text size is larger than 0 since we can't create an empty buffer
		if (textSize < 1) textSize = 0;

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
		if (!text) return;

		// If the new string matches we don't need to do anything
		if (m_text == text) return;

		m_text = text;

		// Ensure that our font atlas has all the glyphs required
		if (!Fonts.VerifyAtlas(data, m_text))
		{
			m_logger.Error("SetText() - Font atlas verification failed.");
			return;
		}

		RegenerateGeometry();
	}

	u32 UIText::GetMaxX() const
	{
		return static_cast<u32>(m_maxX);
	}

	u32 UIText::GetMaxY() const
	{
		return static_cast<u32>(m_maxY);
	}

	void UIText::Draw() const
	{
		if (!m_vertexBuffer->Draw(0, static_cast<u32>(m_vertexData.Size()), true))
		{
			m_logger.Error("Draw() - Failed to draw vertex buffer.");
		}

		if (!m_indexBuffer->Draw(0, static_cast<u32>(m_indexData.Size()), false))
		{
			m_logger.Error("Draw() - Failed to draw index buffer.");
		}
	}

	void UIText::RegenerateGeometry()
	{
		const u64 utf8Size = m_text.SizeUtf8();

		m_maxX = 0;
		m_maxY = 0;

		// No need to regenerate anything since we don't have any text
		if (utf8Size < 1) return;

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

		f32 x = 0, y = 0;

		// Clear our temp data
		m_vertexData.Clear();
		m_indexData.Clear();

		// Make sure we have enough storage for our temp data
		m_vertexData.Reserve(vertexCount);
		m_indexData.Reserve(indexCount);

		for (u32 c = 0, uc = 0; c < m_text.Size(); ++c)
		{
			i32 codepoint = m_text[c];

			// Continue to the next line for newlines
			if (codepoint == '\n')
			{
				x = 0;
				y += static_cast<f32>(data->lineHeight);
				continue;
			}

			if (codepoint == '\t')
			{
				x += data->tabXAdvance;
				continue;
			}

			u8 advance = 0;
			codepoint = m_text.ToCodepoint(c, advance);

			const FontGlyph* glyph = GetFontGlyph(codepoint);
			if (!glyph)
			{
				// If we don't have a valid glyph for the codepoint we simply revert to the codepoint = 1 glyph (fallback glyph)
				glyph = GetFontGlyph(-1);
			}

			if (glyph)
			{
				const f32 minX = x + static_cast<f32>(glyph->xOffset);
				const f32 minY = y + static_cast<f32>(glyph->yOffset);
				const f32 maxX = minX + static_cast<f32>(glyph->width);
				const f32 maxY = minY + static_cast<f32>(glyph->height);

				if (maxX > m_maxX) m_maxX = maxX;
				if (maxY > m_maxY) m_maxY = maxY;

				const f32 atlasSizeX = static_cast<f32>(data->atlasSizeX);
				const f32 atlasSizeY = static_cast<f32>(data->atlasSizeY);

				const f32 tMinX = static_cast<f32>(glyph->x) / atlasSizeX;
				f32 tMinY = static_cast<f32>(glyph->y) / atlasSizeY;
				const f32 tMaxX = static_cast<f32>(glyph->x + glyph->width) / atlasSizeX;
				f32 tMaxY = static_cast<f32>(glyph->y + glyph->height) / atlasSizeY;

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

				// Increment our x by the xAdvance plus any potential kerning
				x += static_cast<f32>(glyph->xAdvance) + GetFontKerningAmount(codepoint, c + advance, utf8Size);
			}
			else
			{
				m_logger.Error("RegenerateGeometry() - Failed find codepoint. Skipping this glyph.");
				continue;
			}

			m_indexData.EmplaceBack(uc * 4 + 2);
			m_indexData.EmplaceBack(uc * 4 + 1);
			m_indexData.EmplaceBack(uc * 4 + 0);
			m_indexData.EmplaceBack(uc * 4 + 3);
			m_indexData.EmplaceBack(uc * 4 + 0);
			m_indexData.EmplaceBack(uc * 4 + 1);

			// Increment our character index (subtracting 1 since our loop will increment every iteration by 1)
			c += advance - 1;
			uc++;
		}

		// Load up our vertex and index buffer data
		if (!m_vertexBuffer->LoadRange(0, m_vertexData.Size() * sizeof(Vertex2D), m_vertexData.GetData()))
		{
			m_logger.Error("RegenerateGeometry() - Failed to LoadRange() for vertex buffer.");
		}
		if (!m_indexBuffer->LoadRange(0, m_indexData.Size() * sizeof(u32), m_indexData.GetData()))
		{
			m_logger.Error("RegenerateGeometry() - Failed to LoadRange() for index buffer.");
		}
	}

	FontGlyph* UIText::GetFontGlyph(const i32 codepoint) const
	{
		for (auto& g : data->glyphs)
		{
			if (g.codepoint == codepoint)
			{
				return &g;
			}
		}
		return nullptr;
	}

	f32 UIText::GetFontKerningAmount(const i32 codepoint, const u32 offset, const u64 utf8Size) const
	{
		if (offset > utf8Size - 1) return 0;

		u8 advanceNext = 0;
		const i32 nextCodepoint = m_text.ToCodepoint(offset, advanceNext);
		if (nextCodepoint == -1) return 0;

		for (const auto& k : data->kernings)
		{
			if (k.codepoint0 == codepoint && k.codepoint1 == nextCodepoint)
			{
				return k.amount;
			}
		}
		return 0;
	}
}
