
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "resource_loader.h"
#include "resources/font.h"
#include "resources/resource_types.h"

namespace C3D
{
    class File;

    enum class BitmapFontFileType
    {
        NotFound,
        CBF,
        FNT,
    };

    struct SupportedBitmapFontFileType
    {
        const char* extension;
        BitmapFontFileType type;
        bool isBinary;
    };

    struct BitmapFontResource : Resource
    {
        FontData data;
        DynamicArray<BitmapFontPage> pages;
    };

    template <>
    class ResourceLoader<BitmapFontResource> final : public IResourceLoader
    {
    public:
        ResourceLoader();

        bool Load(const char* name, BitmapFontResource& resource) const;
        static void Unload(BitmapFontResource& resource);

    private:
        bool ImportFntFile(File& file, const String& outCbfFilename, BitmapFontResource& data) const;

        static bool ParseInfoLine(const String& line, BitmapFontResource& res);
        bool ParseCommonLine(const String& line, BitmapFontResource& res) const;
        bool ParseCharsLine(const String& line, BitmapFontResource& res) const;
        static bool ParseCharLine(const String& line, BitmapFontResource& res);
        static bool ParsePageLine(const String& line, BitmapFontResource& res);
        bool ParseKerningsLine(const String& line, BitmapFontResource& res) const;
        static bool ParseKerningLine(const String& line, BitmapFontResource& res);

        bool ReadCbfFile(File& file, BitmapFontResource& res) const;
        [[nodiscard]] bool WriteCbfFile(const String& path, const BitmapFontResource& res) const;
    };

}  // namespace C3D
