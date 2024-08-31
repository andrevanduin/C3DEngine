
#include "cstring_tests.h"

#include <string/cstring.h>

#include "../expect.h"

TEST(CStringShouldCreateEmptyWithEmptyCtor)
{
    C3D::CString<128> str;

    ExpectEqual(0, str.Size());
    ExpectEqual('\0', str[0]);
}

void CString::RegisterTests(TestManager& manager)
{
    manager.StartType("CString");
    REGISTER_TEST(CStringShouldCreateEmptyWithEmptyCtor, "Strings should be created empty with default CTOR.");
}