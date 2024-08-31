
#include "system_font_manager.h"

namespace C3D
{
    bool ResourceManager<SystemFontResource>::Read(const String& name, SystemFontResource& outResource) const
    {
        C3D_NOT_IMPLEMENTED();
        return false;
    }
    void ResourceManager<SystemFontResource>::Cleanup(SystemFontResource& resource) const { C3D_NOT_IMPLEMENTED(); }
}  // namespace C3D