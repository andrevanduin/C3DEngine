
#include "dynamic_library.h"

#include "platform/platform.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "DYNAMIC_LIBRARY";

    DynamicLibrary::DynamicLibrary(const String& name) : m_name(name) {}

    DynamicLibrary::~DynamicLibrary()
    {
        if (m_data && !m_name.Empty()) Unload();
    }

    bool DynamicLibrary::Load(const char* name)
    {
        if (!Platform::LoadDynamicLibrary(name, &m_data, m_dataSize))
        {
            ERROR_LOG("Failed for: '{}'.", name);
            return false;
        }

        m_name     = name;
        m_fileName = m_name;
        m_fileName += Platform::GetDynamicLibraryExtension();

        INFO_LOG("'{}' was loaded successfully.", name);
        return true;
    }

    bool DynamicLibrary::Unload()
    {
        if (!Platform::UnloadDynamicLibrary(m_data))
        {
            ERROR_LOG("Failed for: '{}'.", m_name);
            return false;
        }

        INFO_LOG("'{}' was unloaded successfully.", m_name);

        m_name.Destroy();
        m_fileName.Destroy();

        m_data     = nullptr;
        m_dataSize = 0;

        return true;
    }
}  // namespace C3D
