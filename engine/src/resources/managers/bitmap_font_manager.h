
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "resource_manager.h"
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

    struct BitmapFontResource : public IResource
    {
        BitmapFontResource() : IResource(ResourceType::BitmapFont) {}

        FontData data;
        DynamicArray<BitmapFontPage> pages;
    };

    template <>
    class ResourceManager<BitmapFontResource> final : public IResourceManager
    {
    public:
        ResourceManager();

        bool Read(const String& name, BitmapFontResource& resource) const;
        void Cleanup(BitmapFontResource& resource) const;

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
