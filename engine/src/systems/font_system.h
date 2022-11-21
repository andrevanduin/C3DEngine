
#pragma once
#include "system.h"
#include "containers/hash_map.h"
#include "containers/hash_table.h"
#include "core/defines.h"
#include "containers/string.h"
#include "resources/loaders/bitmap_font_loader.h"

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
		u8 defaultSystemFontCount;
		SystemFontConfig* systemFontConfigs;
		u8 defaultBitmapFontCount;
		BitmapFontConfig* bitmapFontConfigs;
		u8 maxSystemFontCount;
		u8 maxBitmapFontCount;
		bool autoRelease;
	};

	struct BitmapFontLookup
	{
		u16 referenceCount = 0;
		BitmapFontResource resource;
	};

	class UIText;
	struct FontData;

	class C3D_API FontSystem final : public System<FontSystemConfig>
	{
	public:
		FontSystem();

		bool Init(const FontSystemConfig& config) override;
		void Shutdown() override;

		[[nodiscard]] bool LoadSystemFont(const FontSystemConfig& config) const;
		bool LoadBitmapFont(const BitmapFontConfig& config);

		bool Acquire(const char* fontName, u16 fontSize, UIText* text);
		bool Release(UIText* text);

		bool VerifyAtlas(const FontData* font, const String& text) const;

	private:
		bool SetupFontData(FontData& font) const;
		static void CleanupFontData(FontData& font);

		HashMap<const char*, BitmapFontLookup> m_bitmapFonts;
	};
}
