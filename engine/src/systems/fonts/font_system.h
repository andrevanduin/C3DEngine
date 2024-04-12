
#pragma once
#include "containers/hash_map.h"
#include "containers/string.h"
#include "core/defines.h"
#include "core/uuid.h"
#include "resources/font.h"
#include "resources/loaders/bitmap_font_loader.h"
#include "systems/system.h"

namespace C3D
{
    struct SystemFontConfig
    {
        String name;
        u16 defaultSize;
        String resourceName;
    };

    struct BitmapFontConfig
    {
        String name;
        u16 size;
        String resourceName;
    };

    struct FontSystemConfig
    {
        DynamicArray<SystemFontConfig> systemFontConfigs;
        DynamicArray<BitmapFontConfig> bitmapFontConfigs;

        u8 maxSystemFontCount;
        u8 maxBitmapFontCount;
        bool autoRelease;
    };

    struct BitmapFontLookup
    {
        u16 referenceCount = 0;
        BitmapFontResource resource;
    };

    struct FontData;

    class C3D_API FontSystem final : public SystemWithConfig<FontSystemConfig>
    {
    public:
        explicit FontSystem(const SystemManager* pSystemsManager);

        bool OnInit(const FontSystemConfig& config) override;
        void OnShutdown() override;

        bool LoadSystemFont(const FontSystemConfig& config) const;
        bool LoadBitmapFont(const BitmapFontConfig& config);

        FontHandle Acquire(const char* name, FontType type, u16 fontSize);
        FontHandle Acquire(const String& name, FontType type, u16 fontSize);

        void Release(FontHandle handle);

        bool VerifyAtlas(FontHandle handle, const String& text) const;
        vec2 MeasureString(FontHandle handle, const String& text, u32 size = 0) const;

        const FontData& GetFontData(FontHandle handle) const;

    private:
        bool SetupFontData(FontData& font) const;
        void CleanupFontData(FontData& font) const;

        FontGlyph* GetFontGlyph(const FontData& data, const i32 codepoint) const;
        f32 GetFontKerningAmount(const FontData& data, const String& text, const i32 codepoint, const u32 offset, const u64 utf8Size) const;

        HashMap<FontHandle, BitmapFontLookup> m_bitmapFonts;
        HashMap<const char*, FontHandle> m_bitmapNameLookup;
    };
}  // namespace C3D
