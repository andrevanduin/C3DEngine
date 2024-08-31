
#pragma once
#include "resource_manager.h"
#include "resources/font.h"
#include "resources/resource_types.h"

namespace C3D
{
    enum class SystemFontFileType
    {
        NotFound,
        CSF,
        FontConfig,
    };

    struct SupportedSystemFontFileType
    {
        const char* extension;
        SystemFontFileType type;
        bool isBinary;
    };

    struct SystemFontResource : IResource
    {
        DynamicArray<SystemFontFace> fonts;
        u64 binarySize;
        void* fontBinary;
    };

    template <>
    class ResourceManager<SystemFontResource> final : public IResourceManager
    {
    public:
        ResourceManager();

        bool Read(const String& name, SystemFontResource& outResource) const;
        void Cleanup(SystemFontResource& resource) const;
    };
}  // namespace C3D