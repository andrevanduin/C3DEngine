
#include "font_system.h"

#include "resource_system.h"
#include "texture_system.h"
#include "renderer/renderer_frontend.h"
#include "resources/ui_text.h"
#include "math/c3d_math.h"

namespace C3D
{
	FontSystem::FontSystem()
		: System("FONT_SYSTEM")
	{}

	bool FontSystem::Init(const FontSystemConfig& config)
	{
		if (config.maxBitmapFontCount == 0 || config.maxSystemFontCount == 0)
		{
			m_logger.Error("Init() - Config.maxBitmapFontCount and Config.maxSystemFontCount must be > 0.");
			return false;
		}

		m_config = config;

		m_bitmapFonts.Create(config.maxBitmapFontCount);

		// Load all our bitmap fonts
		for (u32 i = 0; i < m_config.defaultBitmapFontCount; i++)
		{
			if (!LoadBitmapFont(m_config.bitmapFontConfigs[i]))
			{
				m_logger.Error("Init() - Failed to load bitmap font: '{}'", m_config.bitmapFontConfigs[i].name);
			}
		}

		return true;
	}

	void FontSystem::Shutdown()
	{
		// Cleanup all our bitmap fonts
		for (auto& font : m_bitmapFonts)
		{
			CleanupFontData(font.resource.data);
		}

		m_bitmapFonts.Destroy();
	}

	bool FontSystem::LoadSystemFont(const FontSystemConfig& config) const
	{
		m_logger.Error("LoadSystemFont() - This functionality is not yet supported.");
		return false;
	}

	bool FontSystem::LoadBitmapFont(const BitmapFontConfig& config)
	{
		if (m_bitmapFonts.Has(config.name.Data()))
		{
			m_logger.Warn("LoadBitmapFont() - A font named: '{}' already exists and won't be loaded again.", config.name);
			return true;
		}

		// Load our font resource
		BitmapFontLookup lookup;
		if (!Resources.Load(config.resourceName.Data(), &lookup.resource))
		{
			m_logger.Error("LoadBitmapFont() - Failed to load bitmap font resource.");
			return false;
		}

		// Acquire the texture
		// TODO: This only supports one page at the moment!
		lookup.resource.data.atlas.texture = Textures.Acquire(lookup.resource.pages[0].file, true);

		const bool result = SetupFontData(lookup.resource.data);

		// Store our new lookup
		m_bitmapFonts.Set(config.name.Data(), lookup);
		return result;
	}

	bool FontSystem::Acquire(const char* fontName, u16 fontSize, UIText* text)
	{
		if (text->type == UITextType::Bitmap)
		{
			if (!m_bitmapFonts.Has(fontName))
			{
				m_logger.Error("Acquire() - Bitmap font named: '{}' was not found.", fontName);
				return false;
			}

			// Get our lookup and increment the reference count
			auto& lookup = m_bitmapFonts[fontName];
			lookup.referenceCount++;

			// Set the UIText's fontData
			text->data = &lookup.resource.data;

			return true;
		}

		if (text->type == UITextType::System)
		{
			m_logger.Error("Acquire() - System fonts are not yet supported.");
			return false;
		}

		m_logger.Error("Acquire() - Unknown font type: {}.", ToUnderlying(text->type));
		return false;
	}

	bool FontSystem::Release(UIText* text)
	{
		// TODO: Undo our acquire (and potentially auto release)
		return true;
	}

	bool FontSystem::VerifyAtlas(const FontData* font, const String& text) const
	{
		if (font->type == FontType::Bitmap)
		{
			// Bitmaps don't need verification since they are already generated.
			return true;
		}

		m_logger.Error("VerifyAtlas() - Failed to verify atlas, unknown font type");
		return false;
	}

	bool FontSystem::SetupFontData(FontData& font) const
	{
		// Create our TextureMap resources
		font.atlas.magnifyFilter = font.atlas.minifyFilter = TextureFilter::ModeLinear;
		font.atlas.repeatU = font.atlas.repeatV = font.atlas.repeatW = TextureRepeat::ClampToEdge;
		font.atlas.use = TextureUse::Diffuse;

		if (!Renderer.AcquireTextureMapResources(&font.atlas))
		{
			m_logger.Error("SetupFontData() - Unable to acquire resources for font atlas");
			return false;
		}

		// Check for the tab glyph. If it is found we simply use it's xAdvance
		// if it is not found we create one based on 4x space.
		if (EpsilonEqual(font.tabXAdvance, 0.0f))
		{
			bool found = false;

			for (const auto& glyph : font.glyphs)
			{
				if (glyph.codepoint == '\t')
				{
					// If we find the tab character we just use it's xAdvance.
					font.tabXAdvance = glyph.xAdvance;
					found = true;
					break;
				}
			}

			if (!found)
			{
				// If we still haven't found it
				for (const auto& glyph : font.glyphs)
				{
					if (glyph.codepoint == ' ')
					{
						// We set it to 4 x xAdvance of space character.
						font.tabXAdvance = static_cast<f32>(glyph.xAdvance) * 4.0f;
						found = true;
						break;
					}
				}
			}

			if (!found)
			{
				// Still not found since there is no space character either, so we just hard-code it.
				font.tabXAdvance = static_cast<f32>(font.size) * 4.0f;
			}
		}

		return true;
	}

	void FontSystem::CleanupFontData(FontData& font)
	{
		// Release our texture map resources
		Renderer.ReleaseTextureMapResources(&font.atlas);
		// If it's a bitmap font, we release the reference to it's texture
		if (font.type == FontType::Bitmap && font.atlas.texture)
		{
			Textures.Release(font.atlas.texture->name);
		}
		font.atlas.texture = nullptr;
	}
}
