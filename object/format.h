#pragma once

#include <stdio.h>
#include <string>

template<typename... Args>
static std::string format(char const* format, Args&&... args)
{
    std::string output;
    size_t length = snprintf(nullptr, 0, format, args...) + 1;
    output.resize(length);
    snprintf(output.data(), length, format, args...);
    output.pop_back();
    return output;
}
