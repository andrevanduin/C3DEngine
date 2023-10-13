
#include "cstring_tests.h"
#include "../expect.h"
#include "../util.h"

#include <iostream>

#include "containers/cstring.h"

u8 CStringShouldCreateEmptyWithEmptyCtor()
{
	C3D::CString<128> str;

	ExpectShouldBe(0, str.Size());
	ExpectShouldBe('\0', str[0]);

	return true;
}

void CString::RegisterTests(TestManager& manager)
{
	manager.StartType("CString");
	manager.Register(CStringShouldCreateEmptyWithEmptyCtor, "Strings should be created empty with default CTOR.");
}