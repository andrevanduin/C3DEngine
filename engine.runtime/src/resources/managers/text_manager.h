
#pragma once
#include "resource_manager.h"

namespace C3D
{
    struct TextResource final : public IResource
    {
        TextResource() : IResource(ResourceType::Text) {}

        String text;
    };

    template <>
    class C3D_API ResourceManager<TextResource> final : public IResourceManager
    {
    public:
        ResourceManager();

        bool Read(const String& name, TextResource& resource) const;
        void Cleanup(TextResource& resource) const;
    };
}  // namespace C3D
