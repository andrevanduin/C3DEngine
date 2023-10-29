
#pragma once
#include "core/dynamic_library/dynamic_library.h"

namespace C3D
{
    class C3D_API Plugin final : public DynamicLibrary
    {
    public:
        Plugin();
        Plugin(const String& name);

        template <typename Plugin, typename... Args>
        Plugin* Create(Args&&... args)
        {
            const auto createPluginFunc = LoadFunction<Plugin* (*)(Args...)>("CreatePlugin");
            if (!createPluginFunc)
            {
                INSTANCE_ERROR_LOG("PLUGIN", "Failed to load create function for: '{}'.", m_name);
                return nullptr;
            }
            return createPluginFunc(args...);
        }
    };
}  // namespace C3D
