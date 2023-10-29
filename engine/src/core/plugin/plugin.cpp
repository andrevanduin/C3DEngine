
#include "plugin.h"

namespace C3D
{
    Plugin::Plugin() : DynamicLibrary("Plugin") {}

    Plugin::Plugin(const String& name) : DynamicLibrary(name) {}
}  // namespace C3D