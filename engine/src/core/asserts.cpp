
#include "asserts.h"

#include <iostream>

void ReportAssertionFailure(const char* expression, const char* message, const char* file, i32 line)
{
    std::cout << "Assertion failed: " << expression << ", message: " << message << ", in file: " << file
              << ", on line: " << line << std::endl;
}
