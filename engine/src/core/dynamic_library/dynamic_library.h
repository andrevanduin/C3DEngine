
#pragma once
#include "containers/cstring.h"
#include "dynamic_libary_function.h"
#include "memory/allocators/malloc_allocator.h"
#include "platform/platform.h"

namespace C3D
{
	class C3D_API DynamicLibrary
	{
	public:
		DynamicLibrary();

		DynamicLibrary(const DynamicLibrary&) = delete;
		DynamicLibrary(DynamicLibrary&&) = delete;

		DynamicLibrary& operator=(const DynamicLibrary&) = delete;
		DynamicLibrary& operator=(DynamicLibrary&&) = delete;

		~DynamicLibrary();

		bool Load(const char* name);
		bool Unload();

		template <typename Signature>
		Signature LoadFunction(const char* name)
		{
			return Platform::LoadDynamicLibraryFunction<Signature>(name, m_data);
		}

	protected:
		explicit DynamicLibrary(const CString<16>& name);

		DynamicArray<DynamicLibraryFunction, MallocAllocator> m_functions;

		CString<64> m_name;
		CString<64> m_fileName;
		u64 m_dataSize;
		void* m_data;

		LoggerInstance<16> m_logger;
	};
}
