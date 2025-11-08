#pragma once

#include <stdio.h>
#include <string>

#if defined(__clang__)
__attribute__((format(printf, 1, 2)))
#endif
static std::string format(char const* format, ...)
{
    va_list args;
    va_start(args, format);
    int length = vsnprintf(nullptr, 0, format, args) + 1;
    std::string output;
    output.resize(length);
    vsnprintf(output.data(), length, format, args);
    output.pop_back();
    va_end(args);
    return output;
}
