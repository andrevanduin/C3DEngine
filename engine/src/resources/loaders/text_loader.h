
#pragma once
#include "resource_loader.h"

namespace C3D
{
    struct TextResource final : Resource
    {
        String text;
    };

    template <>
    class C3D_API ResourceLoader<TextResource> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, TextResource& resource) const;
        static void Unload(TextResource& resource);
    };
}  // namespace C3D
