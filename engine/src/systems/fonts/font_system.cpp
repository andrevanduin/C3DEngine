
#include "font_system.h"

#include "core/engine.h"
#include "math/c3d_math.h"
#include "renderer/renderer_frontend.h"
#include "systems/resources/resource_system.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "FONT_SYSTEM";

    bool FontSystem::OnInit(const FontSystemConfig& config)
    {
        if (config.maxBitmapFontCount == 0 || config.maxSystemFontCount == 0)
        {
            ERROR_LOG("Config.maxBitmapFontCount and Config.maxSystemFontCount must be > 0.");
            return false;
        }

        m_config = config;

        m_bitmapFonts.Create(config.maxBitmapFontCount);
        m_bitmapNameLookup.Create(config.maxBitmapFontCount);

        // Load all our bitmap fonts
        for (auto& font : m_config.bitmapFontConfigs)
        {
            if (!LoadBitmapFont(font))
            {
                ERROR_LOG("Failed to load bitmap font: '{}'.", font.name);
            }
        }

        return true;
    }

    void FontSystem::OnShutdown()
    {
        INFO_LOG("Cleaning font data for all registered fonts.");

        // Cleanup all our bitmap fonts
        for (auto& font : m_bitmapFonts)
        {
            CleanupFontData(font.resource.data);
        }

        m_bitmapFonts.Destroy();
        m_bitmapNameLookup.Destroy();
    }

    bool FontSystem::LoadSystemFont(const FontSystemConfig& config) const
    {
        ERROR_LOG("This functionality is not yet supported.");
        return false;
    }

    bool FontSystem::LoadBitmapFont(const BitmapFontConfig& config)
    {
        if (m_bitmapNameLookup.Has(config.name.Data()))
        {
            WARN_LOG("A font named: '{}' already exists and won't be loaded again.", config.name);
            return true;
        }

        // Load our font resource
        BitmapFontLookup lookup;
        if (!Resources.Load(config.resourceName, lookup.resource))
        {
            ERROR_LOG("Failed to load bitmap font resource.");
            return false;
        }

        // Acquire the texture
        lookup.resource.data.atlas.texture = Textures.Acquire(lookup.resource.pages[0].file.Data(), true);

        const bool result = SetupFontData(lookup.resource.data);

        // Generate a new Handle (UUID)
        auto handle = FontHandle::Create();
        // Store our new lookup at the generated handle
        m_bitmapFonts.Set(handle, lookup);
        // Also store the combination of name and handle so we can do lookups by name
        m_bitmapNameLookup.Set(config.name.Data(), handle);

        return result;
    }

    FontHandle FontSystem::Acquire(const char* fontName, FontType type, u16 fontSize)
    {
        if (type == FontType::Bitmap)
        {
            if (!m_bitmapNameLookup.Has(fontName))
            {
                ERROR_LOG("Bitmap font named: '{}' was not found.", fontName);
                return FontHandle::Invalid();
            }

            // Get our handle by name
            auto handle = m_bitmapNameLookup[fontName];
            // Now get our actual bitmap data to increment reference count
            auto& data = m_bitmapFonts[handle];
            data.referenceCount++;

            return handle;
        }

        if (type == FontType::System)
        {
            ERROR_LOG("System fonts are not yet supported.");
            return FontHandle::Invalid();
        }

        ERROR_LOG("Unknown font type.");
        return FontHandle::Invalid();
    }

    FontHandle FontSystem::Acquire(const String& name, FontType type, u16 fontSize) { return Acquire(name.Data(), type, fontSize); }

    void FontSystem::Release(FontHandle handle)
    {
        if (m_bitmapFonts.Has(handle))
        {
            // Decrement our reference count
            auto& lookup = m_bitmapFonts[handle];
            lookup.referenceCount--;
        }
    }

    bool FontSystem::VerifyAtlas(FontHandle handle, const String& text) const
    {
        auto& font = m_bitmapFonts[handle].resource.data;
        if (font.type == FontType::Bitmap)
        {
            // Bitmaps don't need verification since they are already generated.
            return true;
        }

        ERROR_LOG("Failed to verify atlas, unknown font type.");
        return false;
    }

    FontGlyph* FontSystem::GetFontGlyph(const FontData& data, const i32 codepoint) const
    {
        for (auto& g : data.glyphs)
        {
            if (g.codepoint == codepoint)
            {
                return &g;
            }
        }
        return nullptr;
    }

    f32 FontSystem::GetFontKerningAmount(const FontData& data, const String& text, const i32 codepoint, const u32 offset,
                                         const u64 utf8Size) const
    {
        if (offset > utf8Size - 1) return 0;

        u8 advanceNext          = 0;
        const i32 nextCodepoint = text.ToCodepoint(offset, advanceNext);
        if (nextCodepoint == -1) return 0;

        for (const auto& k : data.kernings)
        {
            if (k.codepoint0 == codepoint && k.codepoint1 == nextCodepoint)
            {
                return k.amount;
            }
        }
        return 0;
    }

    vec2 FontSystem::MeasureString(FontHandle handle, const String& text, u64 size)
    {
        vec2 extents;

        u32 charLength = C3D::Clamp(size, (u64)0, text.Size());
        u32 utf8Length = text.SizeUtf8();

        f32 x = 0;
        f32 y = 0;

        auto& fontData = GetFontData(handle);

        // Take the length in chars and get the correct codepoint from it.
        for (u32 c = 0; c < charLength; ++c)
        {
            i32 codepoint = text[c];

            // Continue to the next line for newlines
            if (codepoint == '\n')
            {
                x = 0;
                y += static_cast<f32>(fontData.lineHeight);
                continue;
            }

            if (codepoint == '\t')
            {
                x += fontData.tabXAdvance;
                continue;
            }

            u8 advance = 0;
            codepoint  = text.ToCodepoint(c, advance);

            const FontGlyph* glyph = GetFontGlyph(fontData, codepoint);
            if (!glyph)
            {
                // If we don't have a valid glyph for the codepoint we simply revert to the codepoint = 1 glyph
                // (fallback glyph)
                glyph = GetFontGlyph(fontData, -1);
            }

            if (glyph)
            {
                x += glyph->xAdvance + GetFontKerningAmount(fontData, text, codepoint, c + advance, utf8Length);
            }
            else
            {
                ERROR_LOG("Failed find codepoint. Skipping this glyph.");
                continue;
            }

            // Increment our character index (subtracting 1 since our loop will increment every iteration by 1)
            c += advance - 1;
        }

        // One last check in case of no more newlines.
        if (x > extents.x)
        {
            extents.x = x;
        }

        // Since y starts 0-based, we need to add one more to make it 1-line based.
        y += fontData.lineHeight;
        extents.y = y;

        return extents;
    }

    FontData& FontSystem::GetFontData(FontHandle handle) { return m_bitmapFonts[handle].resource.data; }

    bool FontSystem::SetupFontData(FontData& font) const
    {
        // Create our TextureMap resources
        font.atlas.repeatU = font.atlas.repeatV = font.atlas.repeatW = TextureRepeat::ClampToEdge;

        if (!Renderer.AcquireTextureMapResources(font.atlas))
        {
            ERROR_LOG("Unable to acquire resources for font atlas.");
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
                    found            = true;
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
                        found            = true;
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

    void FontSystem::CleanupFontData(FontData& font) const
    {
        // Release our texture map resources
        Renderer.ReleaseTextureMapResources(font.atlas);
        // If it's a bitmap font, we release the reference to it's texture
        if (font.type == FontType::Bitmap && font.atlas.texture)
        {
            Textures.Release(font.atlas.texture->name.Data());
        }
        font.atlas.texture = nullptr;
    }
}  // namespace C3D
