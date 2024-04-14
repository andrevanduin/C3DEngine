
#include "cstring_tests.h"

#include <iostream>

#include "../expect.h"
#include "../util.h"
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
    REGISTER_TEST(CStringShouldCreateEmptyWithEmptyCtor, "Strings should be created empty with default CTOR.");
}