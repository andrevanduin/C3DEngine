
#pragma once
#include "containers/string.h"

namespace C3D
{
	using FuncPtr = void* (*)();

	struct DynamicLibraryFunction
	{
		String name;
		FuncPtr functionPtr;
	};
}