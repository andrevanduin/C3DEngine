
#pragma once
#include "core/defines.h"
#include "containers/cstring.h"

/**
 * @brief All the platform stuff that is generic for all platforms
 * 
 */
namespace C3D {
	using DynamicLibraryPrefix = CString<8>;
	using DynamicLibraryExtension = CString<8>;
	using FileWatchId = u32;

	enum class CopyFileStatus : u8
	{
		Success,
		NotFound,
		Locked,
		Unknown,
	};
}