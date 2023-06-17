
#include "dynamic_library.h"

#include "platform/platform.h"

namespace C3D
{
	DynamicLibrary::DynamicLibrary()
		: m_name(""), m_fileName(""), m_dataSize(0), m_data(nullptr), m_logger("DYNAMIC_LIBRARY")
	{}

	DynamicLibrary::DynamicLibrary(const CString<16>& name)
		: m_name(""), m_fileName(""), m_dataSize(0), m_data(nullptr), m_logger(name)
	{}

	DynamicLibrary::~DynamicLibrary()
	{
		if (m_data && !m_name.Empty()) Unload();
	}

	bool DynamicLibrary::Load(const char* name)
	{
		if (!Platform::LoadDynamicLibrary(name, &m_data, m_dataSize))
		{
			Logger::Error("[DYNAMIC_LIBRARY] - Load() failed for: '{}'", name);
			return false;
		}

		m_name = name;
		m_fileName = m_name;
		m_fileName += Platform::GetDynamicLibraryExtension();

		Logger::Info("[DYNAMIC_LIBRARY] - '{}' was loaded successfully", name);
		return true;
	}

	bool DynamicLibrary::Unload()
	{
		if (!Platform::UnloadDynamicLibrary(m_data))
		{
			Logger::Error("[DYNAMIC_LIBRARY] - Unload() failed for: '{}'", m_name);
			return false;
		}

		Logger::Info("[DYNAMIC_LIBRARY] - '{}' was unloaded successfully", m_name);

		m_name = "";
		m_fileName = "";
		m_data = nullptr;
		m_dataSize = 0;

		return true;
	}
}
