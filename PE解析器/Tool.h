#pragma once
#include <sstream>
#include <iomanip>

template<typename T>
std::string ToHex(T value, size_t width)
{
    std::stringstream ss;
    ss << "0x"
        << std::uppercase
        << std::hex
        << std::setw(width)
        << std::setfill('0')
        << (uint64_t)value;

    return ss.str();
}


